#!/usr/bin/env python3
"""Validate SobelCustom golden data and tiled kernel arithmetic."""

from __future__ import annotations

import argparse
import hashlib
import math
import sys
from pathlib import Path

try:
    import numpy as np
except ModuleNotFoundError as exc:
    raise SystemExit("ERROR: scripts/validate-sobel-baseline.py requires numpy.") from exc


DEFAULT_INPUT_SHAPE = (1, 763, 1024, 3)
DEFAULT_OUTPUT_SHAPE = (1, 1, 761, 1022)
DEFAULT_TILE_H = 9
DEFAULT_TILE_W = 256


def parse_shape(value: str) -> tuple[int, ...]:
    try:
        shape = tuple(int(part) for part in value.replace("x", ",").split(",") if part)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid shape: {value}") from exc
    if not shape or any(dim <= 0 for dim in shape):
        raise argparse.ArgumentTypeError(f"invalid shape: {value}")
    return shape


def sha256_bytes(values: np.ndarray) -> str:
    return hashlib.sha256(values.reshape(-1).tobytes()).hexdigest()


def half_add(left: np.ndarray, right: np.ndarray) -> np.ndarray:
    return (left.astype(np.float16) + right.astype(np.float16)).astype(np.float16)


def half_mul(values: np.ndarray, scalar: float) -> np.ndarray:
    return (values.astype(np.float16) * np.float16(scalar)).astype(np.float16)


def sobel_contract_reference(input_nhwc: np.ndarray) -> np.ndarray:
    input_nchw = np.transpose(input_nhwc, (0, 3, 1, 2)).astype(np.uint8).astype(np.float16)
    gray = (
        0.299 * input_nchw[:, 0, :, :]
        + 0.587 * input_nchw[:, 1, :, :]
        + 0.114 * input_nchw[:, 2, :, :]
    )
    gray32 = np.squeeze(gray).astype(np.float32)

    dx32 = (
        -gray32[:-2, :-2]
        + gray32[:-2, 2:]
        - 2 * gray32[1:-1, :-2]
        + 2 * gray32[1:-1, 2:]
        - gray32[2:, :-2]
        + gray32[2:, 2:]
    )
    dy32 = (
        -gray32[:-2, :-2]
        - 2 * gray32[:-2, 1:-1]
        - gray32[:-2, 2:]
        + gray32[2:, :-2]
        + 2 * gray32[2:, 1:-1]
        + gray32[2:, 2:]
    )
    sobel = np.abs(dx32.astype(np.float16)).astype(np.float16) + np.abs(dy32.astype(np.float16)).astype(np.float16)
    return np.ceil(np.clip(sobel, 0, 255)).astype(np.uint8)


def npu_half_full_reference(input_nhwc: np.ndarray) -> np.ndarray:
    input_nchw = np.transpose(input_nhwc, (0, 3, 1, 2)).astype(np.float16)
    red = half_mul(input_nchw[:, 0, :, :], 0.299)
    green = half_mul(input_nchw[:, 1, :, :], 0.587)
    blue = half_mul(input_nchw[:, 2, :, :], 0.114)
    gray = half_add(half_add(red, green), blue).reshape(input_nhwc.shape[1], input_nhwc.shape[2])

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
    return np.ceil(np.clip(sobel, 0, 255)).astype(np.uint8)


def compute_kernel_tile(tile_nhwc: np.ndarray, tile_h: int, tile_w: int) -> np.ndarray:
    tile_nchw = np.transpose(tile_nhwc.reshape((1, tile_h, tile_w, 3)), (0, 3, 1, 2)).astype(np.float16)
    red = half_mul(tile_nchw[:, 0, :, :], 0.299)
    green = half_mul(tile_nchw[:, 1, :, :], 0.587)
    blue = half_mul(tile_nchw[:, 2, :, :], 0.114)
    gray = half_add(half_add(red, green), blue).reshape(tile_h, tile_w)

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
    return np.ceil(np.clip(sobel, 0, 255)).astype(np.uint8)


def tiled_kernel_reference(
    input_nhwc: np.ndarray,
    output_shape: tuple[int, int],
    tile_h: int,
    tile_w: int,
    source_count_mode: bool,
) -> tuple[np.ndarray, np.ndarray, int, int]:
    input_h, input_w = input_nhwc.shape[1], input_nhwc.shape[2]
    output_h, output_w = output_shape
    out_stride_h = tile_h - 2
    out_stride_w = tile_w - 2
    if source_count_mode:
        count_h = math.ceil(input_h / tile_h)
        count_w = math.ceil(input_w / tile_w)
    else:
        count_h = math.ceil(output_h / out_stride_h)
        count_w = math.ceil(output_w / out_stride_w)

    output = np.zeros((output_h, output_w), dtype=np.uint8)
    written = np.zeros((output_h, output_w), dtype=bool)
    for tile_i in range(count_h):
        for tile_j in range(count_w):
            input_row = tile_i * out_stride_h
            input_col = tile_j * out_stride_w
            tile = np.zeros((tile_h, tile_w, 3), dtype=np.uint8)
            rows = min(input_h - input_row, tile_h)
            cols = min(input_w - input_col, tile_w)
            if rows > 0 and cols > 0:
                tile[:rows, :cols, :] = input_nhwc[0, input_row : input_row + rows, input_col : input_col + cols, :]

            tile_out = compute_kernel_tile(tile, tile_h, tile_w)
            output_row = tile_i * out_stride_h
            output_col = tile_j * out_stride_w
            output_rows = min(output_h - output_row, out_stride_h)
            output_cols = min(output_w - output_col, out_stride_w)
            if output_rows > 0 and output_cols > 0:
                output[output_row : output_row + output_rows, output_col : output_col + output_cols] = tile_out[
                    :output_rows, :output_cols
                ]
                written[output_row : output_row + output_rows, output_col : output_col + output_cols] = True

    return output, written, count_h, count_w


def print_compare(name: str, actual: np.ndarray, expected: np.ndarray, sample_limit: int) -> tuple[int, float, int]:
    actual_flat = actual.reshape(-1)
    expected_flat = expected.reshape(-1)
    diff = actual_flat.astype(np.int16) - expected_flat.astype(np.int16)
    abs_diff = np.abs(diff)
    nonzero = int(np.count_nonzero(diff))
    max_abs = int(abs_diff.max(initial=0))
    mean_abs = float(abs_diff.mean()) if abs_diff.size else 0.0
    print(f"{name}.sha256={sha256_bytes(actual)}")
    print(f"{name}.max_abs_diff={max_abs}")
    print(f"{name}.mean_abs_diff={mean_abs:.9f}")
    print(f"{name}.nonzero_diff_count={nonzero}")
    print(f"{name}.nonzero_diff_rate={nonzero / diff.size:.9f}")
    print(f"{name}.abs_diff_histogram.0={int(np.count_nonzero(abs_diff == 0))}")
    print(f"{name}.abs_diff_histogram.1={int(np.count_nonzero(abs_diff == 1))}")
    print(f"{name}.abs_diff_histogram.2_5={int(np.count_nonzero((2 <= abs_diff) & (abs_diff <= 5)))}")
    print(f"{name}.abs_diff_histogram.gt5={int(np.count_nonzero(abs_diff > 5))}")
    indexes = np.flatnonzero(abs_diff)[:sample_limit]
    if indexes.size:
        print(f"{name}.first_mismatches=")
        width = expected.shape[-1]
        for index in indexes:
            row, col = divmod(int(index), width)
            print(
                f"  offset=0x{int(index):x} row={row} col={col} "
                f"actual=0x{int(actual_flat[index]):02x} expected=0x{int(expected_flat[index]):02x} "
                f"delta={int(diff[index]):+d}"
            )
    return max_abs, mean_abs, nonzero


def print_written(name: str, written: np.ndarray, count_h: int, count_w: int) -> int:
    unwritten = ~written
    unwritten_count = int(np.count_nonzero(unwritten))
    print(f"{name}.cntH={count_h}")
    print(f"{name}.cntW={count_w}")
    print(f"{name}.written_count={int(np.count_nonzero(written))}")
    print(f"{name}.unwritten_count={unwritten_count}")
    if unwritten_count:
        rows, cols = np.where(unwritten)
        print(f"{name}.unwritten_rows_minmax={int(rows.min())},{int(rows.max())}")
        print(f"{name}.unwritten_cols_minmax={int(cols.min())},{int(cols.max())}")
    return unwritten_count


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path, help="SobelCustom x.bin")
    parser.add_argument("--golden", required=True, type=Path, help="SobelCustom y.bin")
    parser.add_argument("--input-shape", default=DEFAULT_INPUT_SHAPE, type=parse_shape)
    parser.add_argument("--output-shape", default=DEFAULT_OUTPUT_SHAPE, type=parse_shape)
    parser.add_argument("--tile-h", default=DEFAULT_TILE_H, type=int)
    parser.add_argument("--tile-w", default=DEFAULT_TILE_W, type=int)
    parser.add_argument("--sample-limit", default=10, type=int)
    args = parser.parse_args()

    if len(args.input_shape) != 4 or args.input_shape[0] != 1 or args.input_shape[3] != 3:
        parser.error("--input-shape must be NHWC 1,H,W,3")
    if len(args.output_shape) != 4 or args.output_shape[0:2] != (1, 1):
        parser.error("--output-shape must be 1,1,H,W")
    if args.tile_h <= 2 or args.tile_w <= 2:
        parser.error("--tile-h and --tile-w must be greater than 2")
    if args.sample_limit < 0:
        parser.error("--sample-limit must be non-negative")

    input_values = np.fromfile(args.input, dtype=np.uint8)
    golden_values = np.fromfile(args.golden, dtype=np.uint8)
    expected_input = math.prod(args.input_shape)
    expected_output = math.prod(args.output_shape)
    print(f"files.input={args.input}")
    print(f"files.golden={args.golden}")
    print(f"files.input_size_bytes={input_values.size}")
    print(f"files.golden_size_bytes={golden_values.size}")
    print(f"files.input_sha256={hashlib.sha256(input_values.tobytes()).hexdigest()}")
    print(f"files.golden_sha256={hashlib.sha256(golden_values.tobytes()).hexdigest()}")
    print(f"shape.input={','.join(str(part) for part in args.input_shape)}")
    print(f"shape.output={','.join(str(part) for part in args.output_shape)}")
    print(f"tile.h={args.tile_h}")
    print(f"tile.w={args.tile_w}")
    print()

    if input_values.size != expected_input:
        print("decision=FAIL_INPUT_SIZE")
        return 2
    if golden_values.size != expected_output:
        print("decision=FAIL_GOLDEN_SIZE")
        return 2

    input_nhwc = input_values.reshape(args.input_shape)
    golden = golden_values.reshape(args.output_shape)[0, 0]
    output_hw = args.output_shape[2], args.output_shape[3]

    contract = sobel_contract_reference(input_nhwc)
    full_half = npu_half_full_reference(input_nhwc)
    source_tiled, source_written, source_count_h, source_count_w = tiled_kernel_reference(
        input_nhwc, output_hw, args.tile_h, args.tile_w, source_count_mode=True
    )
    corrected_tiled, corrected_written, corrected_count_h, corrected_count_w = tiled_kernel_reference(
        input_nhwc, output_hw, args.tile_h, args.tile_w, source_count_mode=False
    )

    contract_max, _, contract_nonzero = print_compare("contract_reference_vs_golden", contract, golden, args.sample_limit)
    print()
    full_half_max, _, _ = print_compare("npu_half_full_reference_vs_golden", full_half, golden, args.sample_limit)
    print()
    source_unwritten = print_written("source_tiling_counts", source_written, source_count_h, source_count_w)
    print_compare("source_tiled_reference_vs_full_half_reference", source_tiled, full_half, args.sample_limit)
    print()
    corrected_unwritten = print_written("corrected_tiling_counts", corrected_written, corrected_count_h, corrected_count_w)
    corrected_max, _, corrected_nonzero = print_compare(
        "corrected_tiled_reference_vs_full_half_reference", corrected_tiled, full_half, args.sample_limit
    )
    print()

    if contract_max != 0 or contract_nonzero != 0:
        print("decision=FAIL_GOLDEN_NOT_CONTRACT_REFERENCE")
        return 1
    if full_half_max > 1:
        print("decision=FAIL_HALF_REFERENCE_DRIFT_TOO_LARGE")
        return 1
    if source_unwritten:
        print("source_tiling_status=FAIL_UNWRITTEN_OUTPUT")
    else:
        print("source_tiling_status=PASS_FULL_COVERAGE")
    if corrected_unwritten or corrected_max != 0 or corrected_nonzero != 0:
        print("decision=FAIL_CORRECTED_TILING_NOT_FULL_REFERENCE")
        return 1

    print("decision=PASS_BASELINE_GOLDEN_AND_CORRECTED_TILING")
    return 0


if __name__ == "__main__":
    sys.exit(main())
