# kirin-ops

Private ops workspace for the Kirin/HarmonyOS CANN spike.

This repository is intentionally scoped to portable scripts, setup notes, checkpoints, and a vendored source snapshot of `cann-recipes-harmony-infer`. It does not include local SDK/tool downloads, generated build artifacts, or downloaded CANN packages.

## Current Layout

- `docs/`: setup notes, DevEco guide, Docker notes, and checkpoints.
- `scripts/`: local helper scripts for DevEco/Harmony and remote CANN containers.
- `cann-recipes-harmony-infer/`: vendored source snapshot used for the Kirin/HarmonyOS spike.

## Current Target

Run the first real-device spike on a HarmonyOS phone with a Kirin chip:

1. Current Kirin9030 build path: use the x86 mobile-station CANN 9.0.0 container on `npu-group-3` to build SobelCustom and convert it to `.omc`.
2. Package the generated `.omc + input + golden` as a naked OMC bundle.
3. Run the bundle on the `aarch64` HarmonyOS target through `/data/local/tmp/model_run_tool`.
4. Validate NPU inference and collect device evidence.

The previous HAP install + `aa start` workflow is not the current operator/kernel test path. It was removed from the active scripts after the real HarmonyOS PC history showed that local testing uses `model_run_tool` directly.

## Known Boundary

The public standard `aarch64` CANN toolkit installs, but it does not compile the current Kirin9030 Sobel custom operator path. The known-good compile/conversion host is the x86 mobile-station toolkit in `z84378291-kirin-mobile-x86`.

See `docs/checkpoints/2026-08-03-kirin-mobile-spike-env-checkpoint.md` for the current state and evidence.
See `docs/kirin9030-sobel-mobile-station-build-plan.md` for the Kirin9030 Sobel build plan, compatibility patch, and evidence.
See `docs/sobel-910b2-validation-toolchain.md` for the 910B2 implementation-level Sobel validation chain.

For the naked `.omc` `model_run_tool` path, see `docs/kirin-naked-omc-test-playbook.md`.

Build a Kirin9030 SobelCustom OMC from the mobile-station host:

```bash
scripts/build-kirin9030-sobel-mobile-station.sh \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/kirin9030-sobel-2026-08-04
```

Validate the SobelCustom implementation on Ascend 910B2:

```bash
scripts/build-sobel-910b2-verify.sh \
  --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
  --pull-to-dir artifacts/remote-pulled/sobel910b2-toolchain-smoke-20260804 \
  --device-id 1
```

Scripted naked OMC `model_run_tool` wrapper:

```bash
scripts/test-naked-omc-vetest.sh --bundle-dir /path/to/kirin9030-gelu-fp16-2026-08-04
```

For Sobel bundles with `x.bin/y.bin`, the test and profiling wrappers use
`scripts/compare-sobel-output.py` to validate the pulled tensor with
Python/numpy after the real-device run.

Package a generic bundle once an `.omc` and its inputs are available:

```bash
scripts/package-naked-omc-bundle.sh --name kirin9030-gelu-fp16-2026-08-04 --omc ./gelu_fp16.omc --input ./gelu_fp16_input.bin --target-soc kirin9030
```
