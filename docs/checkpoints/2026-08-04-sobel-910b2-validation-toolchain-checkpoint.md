# Sobel 910B2 Validation Toolchain Checkpoint

Date: 2026-08-04
Workspace: `/Users/zhenglewen/projects/kirin-ops`
Remote host: `npu-group-3`

## Goal

Create a repeatable 910B2 validation chain for the vendored Sobel custom
operator:

```text
Sobel Ascend C source -> custom OPP package -> Ascend910B2 .om -> ACL run -> numpy accuracy compare
```

This is intended to validate the operator implementation independently of
Kirin9030 real-device packaging and `model_run_tool` behavior.

## Toolchain Added

New host-side script:

```text
scripts/build-sobel-910b2-verify.sh
```

Helper runner:

```text
scripts/run-om-acl.py
```

Enhanced compare helper:

```text
scripts/compare-sobel-output.py
```

The compare helper now prints top mismatch rows/columns plus Sobel tile-oriented
diagnostics:

```text
tile_row_mod_7_mismatch_counts
tile_col_block_254_mismatch_counts
```

## Environment

Validated default environment:

```text
host: npu-group-3
container: z84378291-kirin-cann90
CANN: /usr/local/Ascend/cann-9.0.0-beta.2
ACL device id: 1
```

The 910B2 path uses:

```text
CMake ASCEND_COMPUTE_UNIT=ascend910b
ATC --soc_version=Ascend910B2
```

`ascend910b2` is not accepted as an `ASCEND_COMPUTE_UNIT` in this CANN CMake
environment.

## Execution

Command:

```bash
scripts/build-sobel-910b2-verify.sh \
  --build-name sobel910b2_toolchain_smoke_20260804_05 \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/sobel910b2-toolchain-smoke-20260804 \
  --device-id 1
```

Remote artifact:

```text
/data1/z84378291/artifacts/sobel910b2_toolchain_smoke_20260804_05
```

Local pulled evidence:

```text
artifacts/remote-pulled/sobel910b2-toolchain-smoke-20260804
```

Pulled evidence includes:

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
acl_run.log
build_and_install.log
compare.log
validate_baseline.log
```

## Result

The build/conversion/runtime chain succeeded:

```text
build_status=0
baseline_status=0
atc_status=0
run_status=0
```

Accuracy failed:

```text
compare_status=1
decision=FAIL_ACCURACY_THRESHOLD
```

Primary output contract:

```text
best_candidate=raw_uint8_full
```

Accuracy metrics against packaged `y.bin`:

```text
candidate.raw_uint8_full.max_abs_diff=250
candidate.raw_uint8_full.mean_abs_diff=4.683112137
candidate.raw_uint8_full.nonzero_diff_count=115078
candidate.raw_uint8_full.nonzero_diff_rate=0.147964235
```

The golden/reference path is sane:

```text
candidate.npu_half_clipped_vs_golden.max_abs_diff=1
candidate.npu_half_clipped_vs_golden.nonzero_diff_rate=0.072360757
```

Tile diagnostics:

```text
diagnostic.raw_uint8_full_vs_golden.top_mismatch_rows=602:651,7:630,756:630,161:628,217:623,322:619,462:619,546:619,392:617,721:617,665:616,406:614
diagnostic.raw_uint8_full_vs_golden.top_mismatch_cols=1018:560,1019:559,1020:556,1017:548,1021:547,1016:527,361:136,852:136,3:135,785:135,15:134,24:132
diagnostic.raw_uint8_full_vs_golden.tile_row_mod_7_mismatch_counts=0:64434,1:8475,2:8359,3:8515,4:8531,5:8427,6:8337
diagnostic.raw_uint8_full_vs_golden.tile_col_block_254_mismatch_counts=0[0:254]=27712,1[254:508]=28165,2[508:762]=27965,3[762:1016]=27939,4[1016:1022]=3297
```

After comparing device output to the numpy half-op reference, the remaining
large-error pattern is even clearer:

```text
candidate.raw_uint8_prefix_vs_npu_half_clipped.max_abs_diff=250
candidate.raw_uint8_prefix_vs_npu_half_clipped.nonzero_diff_count=67280
candidate.raw_uint8_prefix_vs_npu_half_clipped.nonzero_diff_rate=0.086506836
diagnostic.raw_uint8_prefix_vs_npu_half_clipped.tile_row_mod_7_mismatch_counts=0:64431,1:469,2:477,3:474,4:475,5:476,6:478
```

## Interpretation

Yes, the operator can be compiled, converted, and executed on 910B2.

No, the current SobelCustom implementation is not proven correct. The 910B2
validation chain shows a real implementation-level accuracy failure after the
golden/reference path passes.

The failure is not an output-format confusion:

- 910B2 ACL output is exactly the expected `uint8` tensor size.
- Kirin9030 `model_run_tool` output from Issue #4 was `uint8` prefix plus zero
  trailing dump bytes.
- The 910B2 failure remains when comparing raw `uint8` against both `y.bin` and
  the numpy half-op reference.

The next kernel fix should focus on tile-boundary handling:

- rows where `row % 7 == 0`;
- the last output column block `1016:1022`;
- `CopyIn`/`CopyOut` stride semantics and the first row of each `h - 2` output
  tile.
