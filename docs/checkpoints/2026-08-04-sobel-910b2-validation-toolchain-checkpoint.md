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

## Accuracy Hardening Loop

Date: 2026-08-04

Root-cause direction:

- The output format is confirmed as raw contiguous `uint8`.
- The packaged `y.bin` golden is confirmed sane against the numpy Sobel
  reference with `max_abs_diff=1`.
- The 910B2 runtime mismatch is inside the scratch-patched tiled vector kernel,
  not the output parser or golden path.
- `SobelCustom::CeilDiv` is a special helper for input-dimension-to-output-tile
  count. The previous 910B2 script patch incorrectly used
  `CeilDiv(H - 2, h - 2)` and `CeilDiv(W - 2, w - 2)`, which over-counts tiles
  and can cause out-of-bounds read/write.
- The same special helper was also used for 32-byte `DataCopyPad` stride gaps;
  tail tiles need a real `ceil(bytes / 32)` helper instead.
- The highest-confidence first-row defect was `Transpose4DImpl` scratch
  overlap: the old scratch buffer began at `calcBuf` offset 0 while
  `tempTensor0` began at byte offset `tileLength * 1 = 6912`; CANN90 910B2
  transpose scratch can use beyond that boundary, corrupting the first row of
  the transposed tile.

Hardening plan:

1. Preserve the vendored vector kernel as evidence.
2. Make `scripts/build-sobel-910b2-verify.sh` default to `vector-fixed`, which
   keeps the original `CeilDiv(H, h)` / `CeilDiv(W, w)` tile count, adds a real
   `Ceil32Blocks(bytes)` helper, reserves dedicated transpose scratch, and
   keeps the existing 910B2 compatibility patches for transpose, index casts,
   chained comparison, offset init, and uint8 clamp.
3. Keep `scalar-correctness` as a diagnostic fallback only, using integer
   fixed-point grayscale to satisfy AscendC scalar-cast restrictions.
4. Rebuild the custom OPP package, convert the ONNX model to
   `SobelCustom_Ascend910B2.om`, run it on ACL device 1, and compare with
   numpy using `max_abs_diff <= 1`.

Acceptance command:

```bash
scripts/build-sobel-910b2-verify.sh \
  --build-name sobel910b2_vector_fix_20260804_01 \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/sobel910b2-vector-fix-20260804 \
  --device-id 1
```

Acceptance criteria:

```text
build_status=0
baseline_status=0
atc_status=0
run_status=0
compare_status=0
kernel_mode=vector-fixed
candidate.raw_uint8_full.max_abs_diff<=1
```

Loop notes:

- `sobel910b2_scalar_fix_20260804_01` failed at kernel compile with
  `build_status=2` because AscendC AICore functions reject scalar casts between
  unsigned integers and floats. The scalar fallback template was converted to
  fixed-point integer math but is not the primary path.
- `sobel910b2_scalar_fixedint_compile_20260804_01` validated that the fixed-
  point scalar diagnostic mode compiles and converts with `build_status=0` and
  `atc_status=0`; ACL execution was intentionally skipped with `--no-run`.

Final 910B2 result:

```text
remote_artifact=/data1/z84378291/artifacts/sobel910b2_vector_fix_20260804_01
local_evidence=artifacts/remote-pulled/sobel910b2-vector-fix-20260804
build_status=0
baseline_status=0
atc_status=0
run_status=0
compare_status=0
kernel_mode=vector-fixed
```

Accuracy evidence:

```text
sobel_output_contract.dtype=uint8
sobel_output_contract.shape=1,1,761,1022
files.output_size_bytes=777742
files.golden_size_bytes=777742
candidate.raw_uint8_full.passes_threshold=true
candidate.raw_uint8_full.max_abs_diff=1
candidate.raw_uint8_full.mean_abs_diff=0.072360757
best_candidate=raw_uint8_full
reference.npu_half_clipped.sha256=9ce63e7376d8977d9bd448f5a130f913b76aed9d31b4ce4679f61331b68b7035
files.output_sha256=9ce63e7376d8977d9bd448f5a130f913b76aed9d31b4ce4679f61331b68b7035
```

Patched remote source evidence:

```text
TRANSPOSE_TMP_BYTES=8192
cntH = SobelCustom::CeilDiv(this->H, h)
cntW = SobelCustom::CeilDiv(this->W, w)
CopyIn dstStride uses SobelCustom::Ceil32Blocks(...)
CopyOut last-column srcStride uses SobelCustom::Ceil32Blocks(...)
```

## Kirin9030 Build Sync

Date: 2026-08-04

After the 910B2 accuracy fix landed, `scripts/build-kirin9030-sobel-mobile-station.sh`
was checked and still had the bad `CeilDiv(H - 2, h - 2)` /
`CeilDiv(W - 2, w - 2)` scratch patch. The Kirin9030 build path was synced to
the same vector fix:

```text
TRANSPOSE_TMP_BYTES=8192
cntH = SobelCustom::CeilDiv(this->H, h)
cntW = SobelCustom::CeilDiv(this->W, w)
CopyIn dstStride uses SobelCustom::Ceil32Blocks(...)
CopyOut last-column srcStride uses SobelCustom::Ceil32Blocks(...)
```

Build command:

```bash
scripts/build-kirin9030-sobel-mobile-station.sh \
  --build-name kirin9030_sobel_vector_fix_20260804_01 \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/kirin9030-sobel-vector-fix-20260804 \
  --jobs 8
```

Build result:

```text
remote_artifact=/data1/z84378291/artifacts/kirin9030_sobel_vector_fix_20260804_01
local_model_files=artifacts/remote-pulled/kirin9030-sobel-vector-fix-20260804
build_status=0
atc_status=0
SobelCustom_kirin9030.omc bytes=31886
omc_sha256=7f93d3c17806d04d0893583beff65274acb2ce318e3da6d64ddd80c5f29461c6
x_sha256=9f2f4d02d225403d3f480b9e88bb7b2b362f1f8a1751c6e34041d38b0935f03b
y_sha256=4f2655e865146d12cfc0513fca54040746c830f23120905079bdec24d3d0e8b1
```

Packaged release:

```text
tag=kirin9030-sobel-custom-vector-fix-2026-08-04
url=https://github.com/RockmanZheng/kirin-ops/releases/tag/kirin9030-sobel-custom-vector-fix-2026-08-04
zip=artifacts/releases/kirin9030-sobel-custom-vector-fix-2026-08-04.zip
zip_sha256=0094517a8e25f65578b68cc2f4bc9562c35dcf2624100f92a3b24197c1a47058
```
