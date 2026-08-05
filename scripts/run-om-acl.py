#!/usr/bin/env python3
"""Run an Ascend OM model with the Python ACL runtime and dump outputs."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

try:
    import acl
except ModuleNotFoundError as exc:
    raise SystemExit("ERROR: run-om-acl.py must run in a CANN Python environment with acl installed.") from exc


# Values are from aclrtMemcpyKind / aclrtMemMallocPolicy in acl/acl_rt.h.
ACL_MEM_MALLOC_HUGE_FIRST = 0
ACL_MEMCPY_HOST_TO_DEVICE = 1
ACL_MEMCPY_DEVICE_TO_HOST = 2


def check(value: object, step: str) -> object:
    ret = value[-1] if isinstance(value, tuple) else value
    if ret != 0:
        raise RuntimeError(f"{step} failed: ret={ret}, raw={value!r}")
    return value


def unpack(value: object, step: str) -> object:
    check(value, step)
    if not isinstance(value, tuple):
        raise RuntimeError(f"{step} returned non-tuple payload: {value!r}")
    payload = value[:-1]
    if len(payload) != 1:
        raise RuntimeError(f"{step} returned unexpected payload: {value!r}")
    return payload[0]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", required=True, type=Path, help="Input .om model.")
    parser.add_argument("--input", required=True, action="append", type=Path, help="Input tensor binary. Repeat per input.")
    parser.add_argument("--output-dir", required=True, type=Path, help="Directory for output_N.bin files.")
    parser.add_argument("--device-id", type=int, default=0, help="Ascend device id. Default: 0.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if not args.model.is_file():
        raise SystemExit(f"ERROR: model not found: {args.model}")
    for input_path in args.input:
        if not input_path.is_file():
            raise SystemExit(f"ERROR: input not found: {input_path}")

    args.output_dir.mkdir(parents=True, exist_ok=True)

    model_id = None
    model_desc = None
    input_dataset = None
    output_dataset = None
    data_buffers: list[int] = []
    device_ptrs: list[int] = []
    host_ptrs: list[int] = []

    check(acl.init(), "acl.init")
    try:
        check(acl.rt.set_device(args.device_id), "acl.rt.set_device")
        model_id = unpack(acl.mdl.load_from_file(str(args.model)), "acl.mdl.load_from_file")
        model_desc = acl.mdl.create_desc()
        check(acl.mdl.get_desc(model_desc, model_id), "acl.mdl.get_desc")

        input_count = acl.mdl.get_num_inputs(model_desc)
        output_count = acl.mdl.get_num_outputs(model_desc)
        print(f"model.inputs={input_count}")
        print(f"model.outputs={output_count}")
        if len(args.input) != input_count:
            raise RuntimeError(f"input count mismatch: provided={len(args.input)} model={input_count}")

        input_dataset = acl.mdl.create_dataset()
        output_dataset = acl.mdl.create_dataset()

        for index, input_path in enumerate(args.input):
            input_size = acl.mdl.get_input_size_by_index(model_desc, index)
            input_bytes = input_path.read_bytes()
            print(f"model.input{index}_size={input_size}")
            print(f"files.input{index}={input_path}")
            print(f"files.input{index}_size={len(input_bytes)}")
            if len(input_bytes) != input_size:
                raise RuntimeError(f"input {index} size mismatch: file={len(input_bytes)} model={input_size}")

            input_dev = unpack(acl.rt.malloc(input_size, ACL_MEM_MALLOC_HUGE_FIRST), f"acl.rt.malloc.input{index}")
            device_ptrs.append(input_dev)
            check(
                acl.rt.memcpy(
                    input_dev,
                    input_size,
                    acl.util.bytes_to_ptr(input_bytes),
                    input_size,
                    ACL_MEMCPY_HOST_TO_DEVICE,
                ),
                f"acl.rt.memcpy.input{index}_h2d",
            )
            input_buffer = acl.create_data_buffer(input_dev, input_size)
            data_buffers.append(input_buffer)
            check(acl.mdl.add_dataset_buffer(input_dataset, input_buffer), f"acl.mdl.add_dataset_buffer.input{index}")

        for index in range(output_count):
            output_size = acl.mdl.get_output_size_by_index(model_desc, index)
            print(f"model.output{index}_size={output_size}")
            output_dev = unpack(acl.rt.malloc(output_size, ACL_MEM_MALLOC_HUGE_FIRST), f"acl.rt.malloc.output{index}")
            device_ptrs.append(output_dev)
            output_buffer = acl.create_data_buffer(output_dev, output_size)
            data_buffers.append(output_buffer)
            check(acl.mdl.add_dataset_buffer(output_dataset, output_buffer), f"acl.mdl.add_dataset_buffer.output{index}")

        check(acl.mdl.execute(model_id, input_dataset, output_dataset), "acl.mdl.execute")

        for index in range(output_count):
            output_size = acl.mdl.get_output_size_by_index(model_desc, index)
            output_buffer = acl.mdl.get_dataset_buffer(output_dataset, index)
            output_dev = acl.get_data_buffer_addr(output_buffer)
            output_host = unpack(acl.rt.malloc_host(output_size), f"acl.rt.malloc_host.output{index}")
            host_ptrs.append(output_host)
            check(
                acl.rt.memcpy(output_host, output_size, output_dev, output_size, ACL_MEMCPY_DEVICE_TO_HOST),
                f"acl.rt.memcpy.output{index}_d2h",
            )
            output_path = args.output_dir / f"output_{index}.bin"
            output_path.write_bytes(acl.util.ptr_to_bytes(output_host, output_size))
            print(f"files.output{index}={output_path}")
            print(f"files.output{index}_size={output_path.stat().st_size}")
    finally:
        for data_buffer in data_buffers:
            try:
                acl.destroy_data_buffer(data_buffer)
            except Exception as exc:  # pragma: no cover - best-effort ACL cleanup
                print(f"cleanup.destroy_data_buffer.error={exc}", file=sys.stderr)
        if input_dataset is not None:
            try:
                acl.mdl.destroy_dataset(input_dataset)
            except Exception as exc:  # pragma: no cover
                print(f"cleanup.destroy_input_dataset.error={exc}", file=sys.stderr)
        if output_dataset is not None:
            try:
                acl.mdl.destroy_dataset(output_dataset)
            except Exception as exc:  # pragma: no cover
                print(f"cleanup.destroy_output_dataset.error={exc}", file=sys.stderr)
        for host_ptr in host_ptrs:
            try:
                acl.rt.free_host(host_ptr)
            except Exception as exc:  # pragma: no cover
                print(f"cleanup.free_host.error={exc}", file=sys.stderr)
        for device_ptr in device_ptrs:
            try:
                acl.rt.free(device_ptr)
            except Exception as exc:  # pragma: no cover
                print(f"cleanup.free.error={exc}", file=sys.stderr)
        if model_desc is not None:
            try:
                acl.mdl.destroy_desc(model_desc)
            except Exception as exc:  # pragma: no cover
                print(f"cleanup.destroy_desc.error={exc}", file=sys.stderr)
        if model_id is not None:
            try:
                acl.mdl.unload(model_id)
            except Exception as exc:  # pragma: no cover
                print(f"cleanup.unload.error={exc}", file=sys.stderr)
        try:
            acl.rt.reset_device(args.device_id)
        except Exception as exc:  # pragma: no cover
            print(f"cleanup.reset_device.error={exc}", file=sys.stderr)
        try:
            acl.finalize()
        except Exception as exc:  # pragma: no cover
            print(f"cleanup.finalize.error={exc}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
