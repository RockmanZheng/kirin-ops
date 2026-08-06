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
  --pull-to-dir artifacts/remote-pulled/sobel910b2-vector-fix-20260804 \
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

Latest passing artifact:

```text
remote: /data1/z84378291/artifacts/sobel910b2_vector_fix_20260804_01
local:  artifacts/remote-pulled/sobel910b2-vector-fix-20260804
```

Status:

```text
build_status=0
baseline_status=0
atc_status=0
run_status=0
compare_status=0
kernel_mode=vector-fixed
```

This means the 910B2 build/conversion/execution/accuracy toolchain is passing.

Accuracy summary from `compare.log`:

```text
output_size_status=EXACT_TENSOR_SIZE
comparison.output_vs_golden.passes_threshold=true
comparison.output_vs_golden.max_abs_diff=1
comparison.output_vs_golden.mean_abs_diff=0.072360757
comparison.output_vs_golden.nonzero_diff_count=56278
comparison.output_vs_golden.nonzero_diff_rate=0.072360757
```

The device output now matches the numpy half-op reference exactly:

```text
files.output_sha256=9ce63e7376d8977d9bd448f5a130f913b76aed9d31b4ce4679f61331b68b7035
reference.npu_half_clipped.sha256=9ce63e7376d8977d9bd448f5a130f913b76aed9d31b4ce4679f61331b68b7035
```

The remaining `max_abs_diff=1` mismatch versus OpenCV `y.bin` is the known
half-precision arithmetic difference:

```text
comparison.npu_half_clipped_vs_golden.max_abs_diff=1
comparison.npu_half_clipped_vs_golden.nonzero_diff_rate=0.072360757
```

The earlier failing smoke artifact was:

```text
remote: /data1/z84378291/artifacts/sobel910b2_toolchain_smoke_20260804_05
local:  artifacts/remote-pulled/sobel910b2-toolchain-smoke-20260804
compare_status=1
comparison.output_vs_golden.max_abs_diff=250
```

Its row-mod pattern pointed at the tiled kernel path, not at the output file
format or golden generation.

## Hardening Notes

`scripts/build-sobel-910b2-verify.sh` patches only a scratch copy of the vendored
Sobel source. It does not mutate the vendored source tree.

The scratch patch currently applies:

- `ASCEND_COMPUTE_UNIT=ascend910b`;
- `AddConfig("ascend910b", ...)`;
- default `--kernel-mode vector-fixed`;
- original output tile counts through `CeilDiv(H, h)` and `CeilDiv(W, w)`;
- dedicated 8KB transpose scratch before the compute tensors so
  `Transpose4DImpl` does not overlap `tempTensor0`;
- true 32-byte ceil helper for `DataCopyPad` local stride gaps;
- explicit clamp before final `uint8` cast;
- initialized `offset`;
- fixed chained comparison `0 < j && j < cntW - 1`;
- removed unsupported `Cast(newindex, index, ...)` index casts for 910B2;
- skipped global OPP install and uses the packaged custom OPP path via
  `ASCEND_CUSTOM_OPP_PATH`.

`--kernel-mode scalar-correctness` is available only as a diagnostic fallback.
It uses direct scalar GM reads/writes and fixed-point integer grayscale math to
avoid AscendC scalar float-cast restrictions.
