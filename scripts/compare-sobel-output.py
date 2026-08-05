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
        f"golden=0x{int(golden[index]):02x} delta={int(diff[index]):+d}"
        for index in indexes
    ]


def candidate_stats(
    name: str,
    output: np.ndarray,
    golden: np.ndarray,
    note: str,
    sample_limit: int,
) -> dict[str, object]:
    diff = output.astype(np.int16) - golden.astype(np.int16)
    abs_diff = np.abs(diff)
    nonzero = int(np.count_nonzero(diff))
    return {
        "name": name,
        "available": True,
        "note": note,
        "max_abs_diff": int(abs_diff.max(initial=0)),
        "mean_abs_diff": float(abs_diff.mean()) if abs_diff.size else 0.0,
        "nonzero_diff_count": nonzero,
        "nonzero_diff_rate": nonzero / int(diff.size) if diff.size else 0.0,
        "first_mismatches": first_mismatches(diff, output, golden, sample_limit),
    }


def unavailable(name: str, reason: str) -> dict[str, object]:
    return {"name": name, "available": False, "reason": reason}


def print_candidate(candidate: dict[str, object], max_abs_diff: int, max_diff_rate: float | None) -> bool:
    name = str(candidate["name"])
    if not candidate["available"]:
        print(f"candidate.{name}.available=false")
        print(f"candidate.{name}.reason={candidate['reason']}")
        return False

    passes = int(candidate["max_abs_diff"]) <= max_abs_diff
    if max_diff_rate is not None:
        passes = passes and float(candidate["nonzero_diff_rate"]) <= max_diff_rate

    print(f"candidate.{name}.available=true")
    print(f"candidate.{name}.passes_threshold={str(passes).lower()}")
    print(f"candidate.{name}.note={candidate['note']}")
    print(f"candidate.{name}.max_abs_diff={candidate['max_abs_diff']}")
    print(f"candidate.{name}.mean_abs_diff={float(candidate['mean_abs_diff']):.9f}")
    print(f"candidate.{name}.nonzero_diff_count={candidate['nonzero_diff_count']}")
    print(f"candidate.{name}.nonzero_diff_rate={float(candidate['nonzero_diff_rate']):.9f}")
    mismatches = candidate["first_mismatches"]
    if mismatches:
        print(f"candidate.{name}.first_mismatches=")
        print("\n".join(str(line) for line in mismatches))
    return passes


def finite_float_candidate(values: np.ndarray, mode: str) -> np.ndarray | None:
    if not np.isfinite(values).all():
        return None
    clipped = np.clip(values, 0, 255)
    if mode == "round":
        converted = np.rint(clipped)
    elif mode == "floor":
        converted = np.floor(clipped)
    elif mode == "ceil":
        converted = np.ceil(clipped)
    else:
        raise AssertionError(mode)
    return converted.astype(np.uint8)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path, help="Pulled model_run_tool output_0 file.")
    parser.add_argument("--golden", required=True, type=Path, help="Packaged y.bin golden file.")
    parser.add_argument("--shape", type=parse_shape, default=DEFAULT_SHAPE, help="Expected output shape. Default: 1,1,761,1022.")
    parser.add_argument("--max-abs-diff", type=int, default=1, help="Allowed absolute uint8 delta. Default: 1.")
    parser.add_argument("--max-diff-rate", type=float, default=None, help="Optional allowed mismatch rate, e.g. 0.01.")
    parser.add_argument("--sample-limit", type=int, default=40, help="Mismatch samples per candidate. Default: 40.")
    args = parser.parse_args()

    if args.max_abs_diff < 0:
        parser.error("--max-abs-diff must be non-negative")
    if args.max_diff_rate is not None and not 0 <= args.max_diff_rate <= 1:
        parser.error("--max-diff-rate must be between 0 and 1")
    if args.sample_limit < 0:
        parser.error("--sample-limit must be non-negative")

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
    print()

    if golden.size != expected_bytes:
        print("decision=FAIL_GOLDEN_SIZE_MISMATCH")
        print("reason=golden file size does not match Sobel uint8 output contract")
        return 2

    candidates: list[dict[str, object]] = []

    if output.size == expected_bytes:
        candidates.append(candidate_stats("raw_uint8_full", output, golden, "whole file is the uint8 tensor", args.sample_limit))
    else:
        candidates.append(unavailable("raw_uint8_full", "output size does not equal expected tensor bytes"))

    if output.size >= expected_bytes:
        candidates.append(
            candidate_stats(
                "raw_uint8_prefix",
                output[:expected_bytes],
                golden,
                "first expected_bytes are the uint8 tensor; remaining bytes are dump tail",
                args.sample_limit,
            )
        )
    else:
        candidates.append(unavailable("raw_uint8_prefix", "output is shorter than expected tensor bytes"))

    if output.size >= expected_bytes * 4:
        window = output[: expected_bytes * 4]
        lanes = window.reshape(expected_bytes, 4)
        for lane in range(4):
            candidates.append(
                candidate_stats(
                    f"stride4_lane{lane}_uint8",
                    lanes[:, lane],
                    golden,
                    f"every 4th byte lane {lane} is the uint8 tensor",
                    args.sample_limit,
                )
            )

        raw4 = window.tobytes()
        for endian, label in (("<", "le"), (">", "be")):
            values_u32 = np.frombuffer(raw4, dtype=f"{endian}u4", count=expected_bytes)
            if bool((values_u32 > 255).any()):
                candidates.append(unavailable(f"uint32_{label}", "at least one uint32 value is outside 0..255"))
            else:
                candidates.append(
                    candidate_stats(
                        f"uint32_{label}",
                        values_u32.astype(np.uint8),
                        golden,
                        f"{label} uint32 values cast to uint8",
                        args.sample_limit,
                    )
                )

            values_f32 = np.frombuffer(raw4, dtype=f"{endian}f4", count=expected_bytes)
            for mode in ("round", "floor", "ceil"):
                converted = finite_float_candidate(values_f32, mode)
                if converted is None:
                    candidates.append(unavailable(f"float32_{label}_{mode}", "non-finite float32 value found"))
                else:
                    candidates.append(
                        candidate_stats(
                            f"float32_{label}_{mode}",
                            converted,
                            golden,
                            f"{label} float32 values clipped to 0..255 and {mode} cast",
                            args.sample_limit,
                        )
                    )
    else:
        candidates.append(unavailable("stride4_or_32bit", "output is shorter than 4x expected tensor bytes"))

    available = [candidate for candidate in candidates if candidate["available"]]
    available.sort(
        key=lambda item: (
            int(item["max_abs_diff"]),
            float(item["mean_abs_diff"]),
            int(item["nonzero_diff_count"]),
        )
    )

    for candidate in candidates:
        print_candidate(candidate, args.max_abs_diff, args.max_diff_rate)
        print()

    if not available:
        print("decision=FAIL_NO_AVAILABLE_FORMAT_CANDIDATE")
        return 1

    best = available[0]
    tail_size = max(0, output.size - expected_bytes)
    print(f"best_candidate={best['name']}")
    print(f"best_candidate.max_abs_diff={best['max_abs_diff']}")
    print(f"best_candidate.mean_abs_diff={float(best['mean_abs_diff']):.9f}")
    print(f"best_candidate.nonzero_diff_rate={float(best['nonzero_diff_rate']):.9f}")
    print(f"tail_size_bytes_after_raw_uint8_prefix={tail_size}")
    if tail_size:
        tail = output[expected_bytes:]
        print(f"tail_sha256_after_raw_uint8_prefix={hashlib.sha256(tail.tobytes()).hexdigest()}")
        print(f"tail_all_zero_after_raw_uint8_prefix={str(bool((tail == 0).all())).lower()}")
        print(f"tail_first_128_hex_after_raw_uint8_prefix={tail[:128].tobytes().hex()}")

    passes = int(best["max_abs_diff"]) <= args.max_abs_diff
    if args.max_diff_rate is not None:
        passes = passes and float(best["nonzero_diff_rate"]) <= args.max_diff_rate

    if best["name"] == "raw_uint8_prefix" and tail_size:
        print("dump_size_status=TRAILING_BYTES_PRESENT")
    elif best["name"] == "raw_uint8_full":
        print("dump_size_status=EXACT_TENSOR_SIZE")
    else:
        print("dump_size_status=NON_RAW_UINT8_FORMAT_CANDIDATE")

    if passes:
        print("decision=PASS_ACCURACY_THRESHOLD")
        return 0

    print("decision=FAIL_ACCURACY_THRESHOLD")
    return 1


if __name__ == "__main__":
    sys.exit(main())
