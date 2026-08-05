# Kirin9030 Sobel Mobile-Station Checkpoint

Date: 2026-08-04
Workspace: `/Users/zhenglewen/projects/kirin-ops`
Remote host: `npu-group-3`
Work ID: `z84378291`

## Goal

Build a Kirin9030-targeted Sobel custom operator model bundle:

```text
Sobel AscendC source -> custom-op package -> model conversion -> .omc + input/golden bundle -> real-device model_run_tool test
```

## Current State

This checkpoint records the active Kirin9030 build-path execution. It complements the generic naked `.omc` runner work already on `main`.

## Work Progress Log

### 2026-08-04 Remote Execution

- Confirmed user-provided device fact is part of the plan: the HarmonyOS target device reports `uname -m` as `aarch64`.
- Installed x86 mobile-station CANN 9.0.0 inside `z84378291-kirin-mobile-x86` under `/opt/Ascend/cann-9.0.0`; host tools `atc`, `ccec`, `bisheng`, `cmake`, `g++`, and `make` are available there.
- The first install attempt under `/data1/z84378291/mobile-station/Ascend` failed because the CANN installer rejected the shared parent directory permissions. The shared workspace was not chmodded; `/opt/Ascend` was used instead to keep the remote workspace policy intact.
- Started x86 mobile-station Sobel scratch build at:

```text
/data1/z84378291/artifacts/kirin9030_sobel_mobile_x86_20260804_092010
```

- Scratch copy edits only:
  - `CMakePresets.json`: `ASCEND_COMPUTE_UNIT=kirin9030`, `ASCEND_CANN_PACKAGE_PATH=/opt/Ascend/cann-9.0.0`
  - `op_host/sobel_custom.cpp`: `AddConfig("kirin9030", aicore_config)`
- Live process evidence shows BiSheng compiling the generated Sobel kernel with:

```text
--compute-unit=kirin9030
--cce-aicore-arch=dav-l311
```

- Python ONNX/data-generation dependency install is still running under x86 emulation. Custom-op binary build does not depend on those Python modules; ONNX/data generation and ATC conversion will wait for this dependency step or use a narrower fallback.

### 2026-08-04 Final Build Result

- The slow x86 pip install was stopped; it was not needed after reusing existing Sobel fixtures.
- ATC/TBE runtime Python dependencies were installed with apt inside `z84378291-kirin-mobile-x86`:

```text
python3-numpy python3-decorator python3-sympy python3-scipy python3-psutil python3-absl python3-attr
```

- Raw x86 mobile-station build failed without the Sobel compatibility patch:

```text
/data1/z84378291/artifacts/kirin9030_sobel_mobile_x86_20260804_092010
status=2
TransposeUB2UBImpl undeclared
```

- Hardened x86 mobile-station scratch build succeeded:

```text
/data1/z84378291/artifacts/kirin9030_sobel_mobile_x86_hardened_20260804_092334
build_status=0
custom_opp_ubuntu_x86_64.run produced and installed
```

- The successful scratch copy used these source hardening changes:
  - `ASCEND_COMPUTE_UNIT=kirin9030`
  - `AddConfig("kirin9030", aicore_config)`
  - fixed `0 < j < cntW - 1` to `0 < j && j < cntW - 1`
  - initialized `offset`
  - used `AscendC::Transpose4DImpl` directly for `__NPU_ARCH__ == 3113`

- ATC conversion succeeded:

```text
atc --model=./SobelCustom.onnx --framework=5 --output=./SobelCustom_kirin9030 --soc_version=Kirin9030
atc_status=0
SobelCustom_kirin9030.omc 31630 bytes
```

- The generated bundle was created locally:

```text
artifacts/naked-omc/kirin9030-sobel-custom-2026-08-04
artifacts/releases/kirin9030-sobel-custom-2026-08-04.zip
zip sha256: 894c0ace26802bb08720c645dc0b92e09545ac81ab97bc3b223a2d38857b9167
```

- Offline checks passed:
  - bundle `SHA256SUMS`
  - `scripts/test-naked-omc-vetest.sh --dry-run`
  - OMC strings include `soc_version`, `Kirin9030`, `SobelCustom`, and `Bisheng-Compiler`

- Scripted repeatability smoke passed:

```text
scripts/build-kirin9030-sobel-mobile-station.sh \
  --build-name kirin9030_sobel_script_smoke_20260804_0246 \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/kirin9030-sobel-script-smoke-2026-08-04 \
  --jobs 8

build_status=0
atc_status=0
remote omc: /data1/z84378291/artifacts/kirin9030_sobel_script_smoke_20260804_0246/model_conversion/SobelCustom_kirin9030.omc
```

- Real-device execution is still pending from this workspace because the local OpenHarmony `hdc` target list returned:

```text
[Empty]
```

### 2026-08-04 Release Upload

Created GitHub prerelease:

```text
tag: kirin9030-sobel-custom-2026-08-04
url: https://github.com/RockmanZheng/kirin-ops/releases/tag/kirin9030-sobel-custom-2026-08-04
target: main
```

Uploaded assets:

```text
kirin9030-sobel-custom-2026-08-04.zip
  sha256: 894c0ace26802bb08720c645dc0b92e09545ac81ab97bc3b223a2d38857b9167
kirin9030-sobel-custom-2026-08-04.omc
  sha256: 8524eaae52bb251f4ce39688b098d9ef09588cb14ccfbedf811f376cb6f90ff8
kirin9030-sobel-custom-2026-08-04.zip.sha256
kirin9030-sobel-custom-2026-08-04.omc.sha256
```

Confirmed so far:

- The target HarmonyOS device reports `uname -m` as `aarch64`.
- Device-side tools and binaries must therefore be `aarch64`.
- The model-conversion host may still be `x86_64`; the x86 mobile-station toolkit is a host cross-generation toolchain and is not deployed to the device.
- The existing Sobel source is not target-parametric:
  - `CMakePresets.json` uses `ASCEND_COMPUTE_UNIT=kirinx90`.
  - `op_host/sobel_custom.cpp` registers `AddConfig("kirinx90", ...)`.
- A Kirin9030 attempt must update both fields together.

### 2026-08-04 Issue #4 Real-Device Accuracy Triage

- Real Kirin9030 device spike reached model execution and pulled `output_0`.
- The run failed the current strict accuracy check because `output_0` was larger than the packaged golden:

```text
output_0: 3110968 bytes
y.bin:     777742 bytes
ratio:     4x
```

- Latest issue comment included `xxd -g1` and `xxd -g4` for the first 128 bytes of `output_0`.
- Comparing those bytes against local bundle golden `artifacts/naked-omc/kirin9030-sobel-custom-2026-08-04/y.bin` showed the device output is already aligned with the `uint8` golden byte stream, not a clean `uint32` or `float32` representation.
- First-128-byte sample had 6 differences, all `abs(delta) == 1`, so the current likely failure mode is:
  - full-file `cmp` fails because `model_run_tool` or the runtime dump includes extra trailing bytes;
  - prefix compare may still need a bounded uint8 tolerance because NPU and CPU/OpenCV Sobel rounding are not byte-exact.
- Updated issue #4 marker comment:

```text
https://github.com/RockmanZheng/kirin-ops/issues/4#issuecomment-5186559434
```

- Next evidence requested on the machine that has the pulled output:
  - compare only the first `wc -c y.bin` bytes of `output_0`;
  - report `max_abs_diff`, `mean_abs_diff`, `nonzero_diff_count`, `nonzero_diff_rate`;
  - dump the first 128 trailing bytes after the valid prefix.
- Added a standalone verifier instead of changing the generic runner semantics:

```bash
scripts/compare-sobel-output.py \
  --output /root/z84378291/kirin-ops/artifacts/naked-omc-runs/20260804_180131/output_0 \
  --golden /root/z84378291/kirin9030-sobel-custom-2026-08-04/y.bin
```

- Local verifier smoke passed:
  - script syntax check with `python3 -m py_compile`;
  - exact `y.bin` vs `y.bin` under a temporary local numpy path reports `raw_uint8_full` and `decision=PASS_ACCURACY_THRESHOLD`;
  - synthetic 4x dump under a temporary local numpy path reports `best_candidate=raw_uint8_prefix`, `dump_size_status=TRAILING_BYTES_PRESENT`, and rejects stride-4/uint32/float32 candidates.
- `npu-group-3` probe confirms Python can import numpy 2.0.2; the real device-side machine is also reported to have numpy available.

## npu-group-3 Evidence

Existing isolated containers:

```text
z84378291-kirin-cann91  cann-9.1.0:v2
z84378291-kirin-cann90  cann90-ascendc-image:snapshot_20260526_112929
```

New x86 mobile-station container:

```text
container: z84378291-kirin-mobile-x86
base image: swr.cn-north-4.myhuaweicloud.com/ddn-k8s/docker.io/library/ubuntu:22.04
platform: linux/amd64
workspace mount: /data1/z84378291:/data1/z84378291
```

The x86 container starts successfully and reports:

```text
uname -m: x86_64
Ubuntu 22.04.4 LTS
```

## aarch64 CANN 9.1 Probe

Artifact:

```text
/data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850
```

Status summary:

```text
quant_matmul_configure=0
quant_matmul_binary=2
sobel_configure=0
sobel_binary=2
sobel_create_onnx=0
sobel_gen_data=0
sobel_atc_kirin9030=255
```

Interpretation:

- `kirin9030` is accepted by CMake/OPC as a target string.
- Both QuantMatmul, which is already authored for Kirin9030, and the patched Sobel scratch copy fail during kernel binary compilation in the current `aarch64` CANN 9.1 container.
- This points to an environment/toolchain mismatch for Kirin mobile compilation, not only a Sobel target-name issue.

Sobel scratch patch evidence:

```text
CMakePresets.json: "value": "kirin9030"
op_host/sobel_custom.cpp: AddConfig("kirin9030", aicore_config)
```

Representative first Sobel kernel errors:

```text
kernel_utils_macros.h:404:8: error: redefinition of 'int4x2_t'
kernel_operator_common_impl.h:283:45: error: use of undeclared identifier 'ROUND_H'
kernel_operator_vec_transpose_intf_impl.h:199:13: error: use of undeclared identifier 'TransposeUB2UBImpl'
```

ATC cannot parse `SobelCustom` because the custom-op plugin was never built and installed:

```text
No operator plugin is registered for Op: sobelCustom, optype: ai.onnx::11::SobelCustom.
```

Generated but not sufficient:

```text
SobelCustom.onnx
x.bin
y.bin
```

No Kirin9030 `.om` or `.omc` was produced by this probe.

## Mobile-Station Package Evidence

Downloaded package:

```text
/data1/z84378291/kirin-ops/downloads/Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run
sha256: 2433c209106723454fe4a09d6506e34925649cb08db0819eb360809a36f0c120
```

Extracted package:

```text
/data1/z84378291/artifacts/mobile_station_extract_20260804_082147
```

Contained subpackages include:

```text
cann-bisheng-compiler_9.0.0_linux-x86_64.run
cann-ge-compiler_9.0.0_linux-x86_64.run
cann-ge-executor_9.0.0_linux-x86_64.run
cann-tbe-tik_9.0.0_linux-x86_64.run
cann-simulator_9.0.0_linux-x86_64.run
cann-kirinx90-ops-legacy_8.5.0_linux-x86_64.run
cann-kirinx90-ops-nn_9.0.0_linux-x86_64.run
cann-kirinx90-ops-transformer_9.0.0_linux-x86_64.run
```

Installer architecture check:

```text
Package arch x86_64 must match current environment architecture x86_64.
```

Therefore the x86 mobile-station package should be installed and executed inside the new `linux/amd64` container, not in the `aarch64` CANN container.

## Completed Steps

1. Installed and validated the x86 mobile-station CANN 9.0.0 toolkit.
2. Proved the unpatched Sobel source fails on Kirin9030/dav_l311.
3. Proved the scratch-only Sobel compatibility patch builds the custom-op package.
4. Installed the custom-op package inside the x86 mobile-station container.
5. Converted `SobelCustom.onnx` to `SobelCustom_kirin9030.omc`.
6. Packaged `.omc + x.bin + y.bin` into the generic naked OMC bundle.
7. Added scripted repeatability with `scripts/build-kirin9030-sobel-mobile-station.sh`.

## Remaining Step

Run the bundle on a connected `aarch64` HarmonyOS device and compare `output_0`
with `y.bin`.

## Acceptance Gates

Do not claim success until each gate is individually true:

- `custom_opp_*.run` exists for the Sobel custom op.
- `SobelCustom` is registered so model conversion no longer fails with `No operator plugin is registered`.
- A Kirin9030-targeted model file is produced.
- A bundle includes model, `x.bin`, `y.bin`, `bundle.env`, and checksums.
- The aarch64 HarmonyOS target loads the model with `model_run_tool`.
- Device output is pulled and compared with `y.bin`, or explicitly marked `PASS_OUTPUT_PULLED_NO_COMPARE` if no golden is available.

## Current Risk

The compile and conversion path is now proven, but runtime compatibility is not
proven until the `.omc` is loaded on the real `aarch64` HarmonyOS target with
`model_run_tool`.
