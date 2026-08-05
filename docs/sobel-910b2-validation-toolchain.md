# Sobel 910B2 Validation Toolchain

Date: 2026-08-04

## Objective

Build a runnable Ascend910B2 version of the vendored `SobelCustom` operator and
use it as an implementation-level accuracy gate before spending more time on
Kirin9030 real-device runs.

This path validates:

- custom operator package build;
- ONNX to `.om` conversion for `Ascend910B2`;
- ACL runtime execution on a 910B2 NPU;
- numpy comparison against the packaged `y.bin` golden and an independent
  half-precision Sobel reference.

## Environment

Default host/container:

```text
host:      npu-group-3
container: z84378291-kirin-cann90
CANN:      /usr/local/Ascend/cann-9.0.0-beta.2
device id: 1
```

The CANN build and conversion settings deliberately differ:

```text
CMake ASCEND_COMPUTE_UNIT: ascend910b
ATC --soc_version:         Ascend910B2
```

`ASCEND_COMPUTE_UNIT=ascend910b2` is not accepted by the CANN CMake support
list in this environment.

## Command

```bash
scripts/build-sobel-910b2-verify.sh \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/sobel910b2-toolchain-smoke-20260804 \
  --device-id 1
```

The script writes a timestamped remote artifact under:

```text
/data1/z84378291/artifacts/sobel910b2_verify_<timestamp>
```

and can pull the main evidence files locally:

```text
SobelCustom_Ascend910B2.om
SobelCustom.onnx
x.bin
y.bin
output_0.bin
manifest.env
build.status
baseline.status
atc.status
run.status
compare.status
*.log
```

## Current Result

Latest smoke artifact:

```text
remote: /data1/z84378291/artifacts/sobel910b2_toolchain_smoke_20260804_05
local:  artifacts/remote-pulled/sobel910b2-toolchain-smoke-20260804
```

Status:

```text
build_status=0
baseline_status=0
atc_status=0
run_status=0
compare_status=1
```

This means the 910B2 build/conversion/execution toolchain is working, but the
current SobelCustom implementation does not pass accuracy.

Accuracy summary from `compare.log`:

```text
best_candidate=raw_uint8_full
candidate.raw_uint8_full.max_abs_diff=250
candidate.raw_uint8_full.mean_abs_diff=4.683112137
candidate.raw_uint8_full.nonzero_diff_count=115078
candidate.raw_uint8_full.nonzero_diff_rate=0.147964235
decision=FAIL_ACCURACY_THRESHOLD
```

The golden baseline itself is not the problem:

```text
candidate.npu_half_clipped_vs_golden.max_abs_diff=1
candidate.npu_half_clipped_vs_golden.nonzero_diff_rate=0.072360757
```

The failure pattern is tile-shaped:

```text
diagnostic.raw_uint8_full_vs_golden.tile_row_mod_7_mismatch_counts=0:64434,1:8475,2:8359,3:8515,4:8531,5:8427,6:8337
diagnostic.raw_uint8_full_vs_golden.tile_col_block_254_mismatch_counts=0[0:254]=27712,1[254:508]=28165,2[508:762]=27965,3[762:1016]=27939,4[1016:1022]=3297
```

Compared with the packaged golden, the mismatches are concentrated on output
rows where `row % 7 == 0`; the last 6 output columns are also
disproportionately bad. The same row-mod pattern remains after comparing device
output with the numpy half-op reference, so the current evidence points at the
tiled kernel boundary path, not at the output file format or golden generation.

## Hardening Notes

`scripts/build-sobel-910b2-verify.sh` patches only a scratch copy of the vendored
Sobel source. It does not mutate the vendored source tree.

The scratch patch currently applies:

- `ASCEND_COMPUTE_UNIT=ascend910b`;
- `AddConfig("ascend910b", ...)`;
- output tile counts based on `H - 2` and `W - 2`;
- explicit clamp before final `uint8` cast;
- initialized `offset`;
- fixed chained comparison `0 < j && j < cntW - 1`;
- removed unsupported `Cast(newindex, index, ...)` index casts for 910B2;
- skipped global OPP install and uses the packaged custom OPP path via
  `ASCEND_CUSTOM_OPP_PATH`.

The next implementation fix should target the `SobelCustom` tiled data path,
especially tile first-row handling and the final column tile.
