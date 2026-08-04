# Kirin9030 Sobel Mobile-Station Build Plan

Date: 2026-08-04

## Objective

Produce a Kirin9030-targeted SobelCustom naked OMC bundle that can be pushed to an
`aarch64` HarmonyOS device and run with `model_run_tool`.

The compiler host and the target device are different:

- Device target: HarmonyOS `aarch64`.
- Build/conversion host: x86_64 Linux mobile-station CANN toolkit.
- Device-side runner/tools must still be `aarch64`.

## Best Practice

Use the x86 mobile-station toolkit as a cross-generation host toolchain. The
standard `aarch64` CANN 9.1 container on `npu-group-3` is useful for CANN smoke
tests, but it fails the current Kirin mobile Sobel kernel build.

Keep the workflow isolated:

- Use `z84378291-kirin-mobile-x86` for mobile-station work.
- Install CANN under `/opt/Ascend/cann-9.0.0` inside the container.
- Do not chmod or repurpose the shared `/data1/z84378291` parent directory.
- Copy vendored Sobel source into a timestamped scratch artifact before patching.
- Keep generated `.omc`, input, golden, build logs, and zips under ignored
  `artifacts/`.

Apply the Kirin9030 Sobel compatibility patch only in the scratch copy:

- `CMakePresets.json`: `ASCEND_COMPUTE_UNIT=kirin9030`.
- `op_host/sobel_custom.cpp`: `AddConfig("kirin9030", aicore_config)`.
- `op_kernel/sobel_custom.cpp`:
  - fix the invalid C++ chained comparison `0 < j < cntW - 1`;
  - initialize `offset` defensively;
  - for `__NPU_ARCH__ == 3113` call `AscendC::Transpose4DImpl` directly.

Why the transpose patch is needed: Kirin9030 maps to `dav_l311`. The CANN 9.0.0
`dav_l311` implementation provides `Transpose4DImpl`, but the public 4D
`AscendC::Transpose` wrapper still compiles a `cSize == 1` branch that references
`TransposeUB2UBImpl`, which is not present for `dav_l311`.

## One-Time Environment

Remote host:

```text
npu-group-3
```

Container:

```text
z84378291-kirin-mobile-x86
platform: linux/amd64
CANN: /opt/Ascend/cann-9.0.0
```

Mobile-station package:

```text
/data1/z84378291/kirin-ops/downloads/Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run
sha256: 2433c209106723454fe4a09d6506e34925649cb08db0819eb360809a36f0c120
```

ATC/TBE Python runtime packages installed with apt:

```text
python3-numpy python3-decorator python3-sympy python3-scipy python3-psutil python3-absl python3-attr
```

## Reproducible Build

If the Sobel ONNX/input/golden fixtures already exist on the remote host:

```bash
scripts/build-kirin9030-sobel-mobile-station.sh \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/kirin9030-sobel-2026-08-04
```

Then package the pulled files:

```bash
scripts/package-naked-omc-bundle.sh \
  --name kirin9030-sobel-custom-2026-08-04 \
  --description "SobelCustom Kirin9030 OMC built with x86 mobile-station CANN 9.0.0 on npu-group-3" \
  --omc artifacts/remote-pulled/kirin9030-sobel-2026-08-04/SobelCustom_kirin9030.omc \
  --input artifacts/remote-pulled/kirin9030-sobel-2026-08-04/x.bin \
  --golden artifacts/remote-pulled/kirin9030-sobel-2026-08-04/y.bin \
  --target-soc kirin9030 \
  --compare 1 \
  --force
```

## Executed Evidence

aarch64 CANN 9.1 probe:

```text
/data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850
sobel_binary=2
sobel_atc_kirin9030=255
```

Representative failure:

```text
kernel_operator_vec_transpose_intf_impl.h:199:13: error: use of undeclared identifier 'TransposeUB2UBImpl'
```

x86 mobile-station raw scratch build:

```text
/data1/z84378291/artifacts/kirin9030_sobel_mobile_x86_20260804_092010
status=2
```

x86 mobile-station hardened scratch build:

```text
/data1/z84378291/artifacts/kirin9030_sobel_mobile_x86_hardened_20260804_092334
build_status=0
custom_opp_ubuntu_x86_64.run produced and installed
```

ATC conversion:

```text
atc --model=./SobelCustom.onnx --framework=5 --output=./SobelCustom_kirin9030 --soc_version=Kirin9030
atc_status=0
SobelCustom_kirin9030.omc 31630 bytes
```

Bundle:

```text
artifacts/naked-omc/kirin9030-sobel-custom-2026-08-04
artifacts/releases/kirin9030-sobel-custom-2026-08-04.zip
zip sha256: 894c0ace26802bb08720c645dc0b92e09545ac81ab97bc3b223a2d38857b9167
```

Offline checks:

```text
bundle SHA256SUMS: OK
runner dry-run: OK
strings: soc_version Kirin9030, SobelCustom, Bisheng-Compiler
```

## Device Test

Real-device execution is still pending from this environment because the local
OpenHarmony `hdc` returned no connected target:

```text
hdc list targets
[Empty]
```

When a device is attached:

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/artifacts/naked-omc/kirin9030-sobel-custom-2026-08-04" \
  --device-dir "/data/local/tmp/z84378291" \
  --model-run-tool "/data/local/tmp/z84378291/model_run_tool" \
  --no-clear-logs
```
