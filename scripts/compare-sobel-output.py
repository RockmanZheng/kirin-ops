#!/usr/bin/env python3
"""Verify SobelCustom model_run_tool output against y.bin."""

from __future__ import annotations

import argparse
import hashlib
import math
import sys
from pathlib import Path

try:
    import numpy as np
except ModuleNotFoundError as exc:
    raise SystemExit("ERROR: scripts/compare-sobel-output.py requires numpy on the host running the compare.") from exc


DEFAULT_SHAPE = (1, 1, 761, 1022)
DEFAULT_INPUT_SHAPE = (1, 763, 1024, 3)


def parse_shape(value: str) -> tuple[int, ...]:
    try:
        dims = tuple(int(part) for part in value.replace("x", ",").split(",") if part)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid shape: {value}") from exc
    if not dims or any(dim <= 0 for dim in dims):
        raise argparse.ArgumentTypeError(f"invalid shape: {value}")
    return dims


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def format_shape(shape: tuple[int, ...]) -> str:
    return ",".join(str(dim) for dim in shape)


def first_mismatches(diff: np.ndarray, output: np.ndarray, golden: np.ndarray, limit: int) -> list[str]:
    if limit <= 0:
        return []
    indexes = np.flatnonzero(diff)[:limit]
    return [
        f"  offset=0x{int(index):x} output=0x{int(output[index]):02x} "
        f"expected=0x{int(golden[index]):02x} delta={int(diff[index]):+d}"
        for index in indexes
    ]


def offset_index(index: int, shape: tuple[int, ...]) -> str:
    if math.prod(shape) != 0:
        return ",".join(str(part) for part in np.unravel_index(index, shape))
    return ""


def print_spatial_mismatch_summary(prefix: str, abs_diff: np.ndarray, shape: tuple[int, ...], top_limit: int) -> None:
    if top_limit <= 0 or len(shape) < 2 or abs_diff.size != math.prod(shape):
        return

    height = shape[-2]
    width = shape[-1]
    spatial = abs_diff.reshape((-1, height, width))
    row_counts = np.count_nonzero(spatial, axis=(0, 2))
    col_counts = np.count_nonzero(spatial, axis=(0, 1))
    if not row_counts.any() and not col_counts.any():
        return

    top_count = min(top_limit, 12)
    row_order = np.argsort(-row_counts, kind="stable")[:top_count]
    col_order = np.argsort(-col_counts, kind="stable")[:top_count]
    row_parts = [f"{int(index)}:{int(row_counts[index])}" for index in row_order if row_counts[index]]
    col_parts = [f"{int(index)}:{int(col_counts[index])}" for index in col_order if col_counts[index]]
    if row_parts:
        print(f"{prefix}.top_mismatch_rows=" + ",".join(row_parts))
    if col_parts:
        print(f"{prefix}.top_mismatch_cols=" + ",".join(col_parts))


def print_tile_mismatch_summary(
    prefix: str,
    abs_diff: np.ndarray,
    shape: tuple[int, ...],
    tile_output_height: int,
    tile_output_width: int,
) -> None:
    if len(shape) < 2 or abs_diff.size != math.prod(shape):
        return

    height = shape[-2]
    width = shape[-1]
    spatial = abs_diff.reshape((-1, height, width))
    mismatch = spatial != 0
    if tile_output_height > 1:
        row_mod_parts = [
            f"{mod}:{int(np.count_nonzero(mismatch[:, mod::tile_output_height, :]))}"
            for mod in range(tile_output_height)
        ]
        print(f"{prefix}.tile_row_mod_{tile_output_height}_mismatch_counts=" + ",".join(row_mod_parts))
    if tile_output_width > 1:
        col_block_parts = []
        for block_index, start in enumerate(range(0, width, tile_output_width)):
            end = min(start + tile_output_width, width)
            col_block_parts.append(f"{block_index}[{start}:{end}]={int(np.count_nonzero(mismatch[:, :, start:end]))}")
        print(f"{prefix}.tile_col_block_{tile_output_width}_mismatch_counts=" + ",".join(col_block_parts))


def print_diff_diagnostics(
    prefix: str,
    output: np.ndarray,
    expected: np.ndarray,
    shape: tuple[int, ...],
    top_limit: int,
    tile_output_height: int,
    tile_output_width: int,
) -> None:
    diff = output.astype(np.int16) - expected.astype(np.int16)
    abs_diff = np.abs(diff)
    print(f"{prefix}.abs_diff_histogram.0={int(np.count_nonzero(abs_diff == 0))}")
    print(f"{prefix}.abs_diff_histogram.1={int(np.count_nonzero(abs_diff == 1))}")
    print(f"{prefix}.abs_diff_histogram.2_5={int(np.count_nonzero((2 <= abs_diff) & (abs_diff <= 5)))}")
    print(f"{prefix}.abs_diff_histogram.6_15={int(np.count_nonzero((6 <= abs_diff) & (abs_diff <= 15)))}")
    print(f"{prefix}.abs_diff_histogram.16_63={int(np.count_nonzero((16 <= abs_diff) & (abs_diff <= 63)))}")
    print(f"{prefix}.abs_diff_histogram.64_127={int(np.count_nonzero((64 <= abs_diff) & (abs_diff <= 127)))}")
    print(f"{prefix}.abs_diff_histogram.128_255={int(np.count_nonzero(abs_diff >= 128))}")
    print_spatial_mismatch_summary(prefix, abs_diff, shape, top_limit)
    print_tile_mismatch_summary(prefix, abs_diff, shape, tile_output_height, tile_output_width)

    if top_limit <= 0:
        return
    mismatch_indexes = np.flatnonzero(abs_diff)
    if not mismatch_indexes.size:
        return
    order = np.argsort(-abs_diff[mismatch_indexes], kind="stable")[:top_limit]
    print(f"{prefix}.largest_mismatches=")
    for index in mismatch_indexes[order]:
        index_int = int(index)
        coord = offset_index(index_int, shape)
        coord_part = f" index={coord}" if coord else ""
        print(
            f"  offset=0x{index_int:x}{coord_part} output=0x{int(output[index_int]):02x} "
            f"expected=0x{int(expected[index_int]):02x} delta={int(diff[index_int]):+d}"
        )


def half_add(left: np.ndarray, right: np.ndarray) -> np.ndarray:
    return (left.astype(np.float16) + right.astype(np.float16)).astype(np.float16)


def half_mul(values: np.ndarray, scalar: float) -> np.ndarray:
    return (values.astype(np.float16) * np.float16(scalar)).astype(np.float16)


def npu_half_sobel_reference(input_path: Path, input_shape: tuple[int, ...], output_shape: tuple[int, ...]) -> np.ndarray:
    if len(input_shape) != 4 or input_shape[0] != 1 or input_shape[3] != 3:
        raise ValueError("input reference currently expects NHWC shape 1,H,W,3")
    expected_input_bytes = math.prod(input_shape)
    input_values = np.fromfile(input_path, dtype=np.uint8)
    if input_values.size != expected_input_bytes:
        raise ValueError(f"input size {input_values.size} does not match input shape bytes {expected_input_bytes}")

    input_nhwc = input_values.reshape(input_shape)
    input_nchw = np.transpose(input_nhwc, (0, 3, 1, 2)).astype(np.float16)

    red = half_mul(input_nchw[:, 0, :, :], 0.299)
    green = half_mul(input_nchw[:, 1, :, :], 0.587)
    blue = half_mul(input_nchw[:, 2, :, :], 0.114)
    gray = half_add(half_add(red, green), blue).reshape(input_shape[1], input_shape[2])

    dx = half_mul(gray[:-2, :-2], -1)
    dx = half_add(dx, half_mul(gray[1:-1, :-2], -2))
    dx = half_add(dx, half_mul(gray[2:, :-2], -1))
    dx = half_add(dx, gray[:-2, 2:])
    dx = half_add(dx, half_mul(gray[1:-1, 2:], 2))
    dx = half_add(dx, gray[2:, 2:])

    dy = half_mul(gray[:-2, :-2], -1)
    dy = half_add(dy, half_mul(gray[2:, :-2], 1))
    dy = half_add(dy, half_mul(gray[:-2, 1:-1], -2))
    dy = half_add(dy, half_mul(gray[2:, 1:-1], 2))
    dy = half_add(dy, half_mul(gray[:-2, 2:], -1))
    dy = half_add(dy, gray[2:, 2:])

    sobel = half_add(np.abs(dx).astype(np.float16), np.abs(dy).astype(np.float16))
    reference = np.ceil(np.clip(sobel, 0, 255)).astype(np.uint8).reshape(-1)
    if reference.size != math.prod(output_shape):
        raise ValueError(f"reference size {reference.size} does not match output shape bytes {math.prod(output_shape)}")
    return reference


def comparison_stats(
    output: np.ndarray,
    expected: np.ndarray,
    sample_limit: int,
) -> dict[str, object]:
    diff = output.astype(np.int16) - expected.astype(np.int16)
    abs_diff = np.abs(diff)
    nonzero = int(np.count_nonzero(diff))
    return {
        "max_abs_diff": int(abs_diff.max(initial=0)),
        "mean_abs_diff": float(abs_diff.mean()) if abs_diff.size else 0.0,
        "nonzero_diff_count": nonzero,
        "nonzero_diff_rate": nonzero / int(diff.size) if diff.size else 0.0,
        "first_mismatches": first_mismatches(diff, output, expected, sample_limit),
    }


def print_comparison(name: str, stats: dict[str, object], max_abs_diff: int, max_diff_rate: float | None) -> bool:
    passes = int(stats["max_abs_diff"]) <= max_abs_diff
    if max_diff_rate is not None:
        passes = passes and float(stats["nonzero_diff_rate"]) <= max_diff_rate

    print(f"comparison.{name}.available=true")
    print(f"comparison.{name}.passes_threshold={str(passes).lower()}")
    print(f"comparison.{name}.max_abs_diff={stats['max_abs_diff']}")
    print(f"comparison.{name}.mean_abs_diff={float(stats['mean_abs_diff']):.9f}")
    print(f"comparison.{name}.nonzero_diff_count={stats['nonzero_diff_count']}")
    print(f"comparison.{name}.nonzero_diff_rate={float(stats['nonzero_diff_rate']):.9f}")
    mismatches = stats["first_mismatches"]
    if mismatches:
        print(f"comparison.{name}.first_mismatches=")
        print("\n".join(str(line) for line in mismatches))
    return passes


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path, help="Pulled model_run_tool output_0 file.")
    parser.add_argument("--golden", required=True, type=Path, help="Packaged y.bin golden file.")
    parser.add_argument("--input", type=Path, help="Optional packaged x.bin input; enables NPU half-op reference diagnostics.")
    parser.add_argument("--shape", type=parse_shape, default=DEFAULT_SHAPE, help="Expected output shape. Default: 1,1,761,1022.")
    parser.add_argument("--input-shape", type=parse_shape, default=DEFAULT_INPUT_SHAPE, help="Expected input shape. Default: 1,763,1024,3.")
    parser.add_argument("--max-abs-diff", type=int, default=1, help="Allowed absolute uint8 delta. Default: 1.")
    parser.add_argument("--max-diff-rate", type=float, default=None, help="Optional allowed mismatch rate, e.g. 0.01.")
    parser.add_argument("--sample-limit", type=int, default=40, help="Mismatch samples per comparison. Default: 40.")
    parser.add_argument("--top-mismatches", type=int, default=20, help="Largest mismatch samples for diagnostics. Default: 20.")
    parser.add_argument("--tile-output-height", type=int, default=7, help="Sobel tile output height for diagnostics. Default: 7.")
    parser.add_argument("--tile-output-width", type=int, default=254, help="Sobel tile output width for diagnostics. Default: 254.")
    args = parser.parse_args()

    if args.max_abs_diff < 0:
        parser.error("--max-abs-diff must be non-negative")
    if args.max_diff_rate is not None and not 0 <= args.max_diff_rate <= 1:
        parser.error("--max-diff-rate must be between 0 and 1")
    if args.sample_limit < 0:
        parser.error("--sample-limit must be non-negative")
    if args.top_mismatches < 0:
        parser.error("--top-mismatches must be non-negative")
    if args.tile_output_height < 0:
        parser.error("--tile-output-height must be non-negative")
    if args.tile_output_width < 0:
        parser.error("--tile-output-width must be non-negative")

    expected_bytes = math.prod(args.shape)
    output = np.fromfile(args.output, dtype=np.uint8)
    golden = np.fromfile(args.golden, dtype=np.uint8)

    print("sobel_output_contract.dtype=uint8")
    print(f"sobel_output_contract.shape={format_shape(args.shape)}")
    print(f"sobel_output_contract.expected_bytes={expected_bytes}")
    print(f"files.output={args.output}")
    print(f"files.golden={args.golden}")
    print(f"files.output_size_bytes={output.size}")
    print(f"files.golden_size_bytes={golden.size}")
    print(f"files.output_sha256={sha256_file(args.output)}")
    print(f"files.golden_sha256={sha256_file(args.golden)}")
    print(f"files.output_expected_size_ratio={output.size / expected_bytes:.6f}")
    print(f"output_size_status={'EXACT_TENSOR_SIZE' if output.size == expected_bytes else 'SIZE_MISMATCH'}")
    print()

    if golden.size != expected_bytes:
        print("decision=FAIL_GOLDEN_SIZE_MISMATCH")
        print("reason=golden file size does not match Sobel uint8 output contract")
        return 2

    if output.size != expected_bytes:
        if output.size > expected_bytes:
            extra = output[expected_bytes:]
            print(f"extra_bytes_after_expected_tensor={extra.size}")
            print(f"files.output_first_expected_bytes_sha256={hashlib.sha256(output[:expected_bytes].tobytes()).hexdigest()}")
            print(f"files.output_extra_sha256={hashlib.sha256(extra.tobytes()).hexdigest()}")
            print(f"files.output_extra_all_zero={str(bool((extra == 0).all())).lower()}")
            print(f"files.output_extra_first_128_hex={extra[:128].tobytes().hex()}")
        else:
            print(f"missing_bytes_before_expected_tensor={expected_bytes - output.size}")
        print("decision=FAIL_OUTPUT_SIZE_MISMATCH")
        print("reason=output file size must exactly match Sobel uint8 tensor contract; rerun model_run_tool with --output_type=UINT8")
        return 1

    output_vs_golden = comparison_stats(output, golden, args.sample_limit)
    passes = print_comparison("output_vs_golden", output_vs_golden, args.max_abs_diff, args.max_diff_rate)
    print_diff_diagnostics(
        "diagnostic.output_vs_golden",
        output,
        golden,
        args.shape,
        args.top_mismatches,
        args.tile_output_height,
        args.tile_output_width,
    )

    if args.input:
        print()
        try:
            npu_reference = npu_half_sobel_reference(args.input, args.input_shape, args.shape)
        except ValueError as exc:
            print("reference.npu_half_clipped.available=false")
            print(f"reference.npu_half_clipped.reason={exc}")
        else:
            print("reference.npu_half_clipped.available=true")
            print("reference.npu_half_clipped.note=numpy simulation of the Ascend C half-precision Sobel arithmetic path, clipped before uint8 cast")
            print(f"reference.npu_half_clipped.sha256={hashlib.sha256(npu_reference.tobytes()).hexdigest()}")

            reference_vs_golden = comparison_stats(
                npu_reference,
                golden,
                args.sample_limit,
            )
            print_comparison("npu_half_clipped_vs_golden", reference_vs_golden, args.max_abs_diff, args.max_diff_rate)
            print_diff_diagnostics(
                "diagnostic.npu_half_clipped_vs_golden",
                npu_reference,
                golden,
                args.shape,
                args.top_mismatches,
                args.tile_output_height,
                args.tile_output_width,
            )

            output_vs_reference = comparison_stats(output, npu_reference, args.sample_limit)
            print_comparison("output_vs_npu_half_clipped", output_vs_reference, args.max_abs_diff, args.max_diff_rate)
            print_diff_diagnostics(
                "diagnostic.output_vs_npu_half_clipped",
                output,
                npu_reference,
                args.shape,
                args.top_mismatches,
                args.tile_output_height,
                args.tile_output_width,
            )

    if passes:
        print("decision=PASS_ACCURACY_THRESHOLD")
        return 0

    print("decision=FAIL_ACCURACY_THRESHOLD")
    return 1


if __name__ == "__main__":
    sys.exit(main())
