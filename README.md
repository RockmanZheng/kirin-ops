# kirin-ops

Private ops workspace for the Kirin/HarmonyOS CANN spike.

This repository is intentionally scoped to portable scripts, setup notes, checkpoints, and a vendored source snapshot of `cann-recipes-harmony-infer`. It does not include local SDK/tool downloads, generated build artifacts, or downloaded CANN packages.

## Current Layout

- `docs/`: setup notes, DevEco guide, Docker notes, and checkpoints.
- `scripts/`: local helper scripts for DevEco/Harmony and remote CANN containers.
- `cann-recipes-harmony-infer/`: vendored source snapshot used for the Kirin/HarmonyOS spike.

## Current Target

Run the first real-device spike on a HarmonyOS phone with a Kirin chip:

1. Fast path: run the prebuilt `SobelCustom.omc` on a real device.
2. Full toolchain path: build the Sobel custom operator on Linux and convert it with ATC.
3. App path: package the HarmonyOS `Soble` app with DevEco when app UI validation is needed.
4. Validate NPU inference and collect device evidence.

## Known Boundary

The public standard `aarch64` CANN toolkit installs, but it has not compiled the repo's `kirinx90` Sobel custom operator. The repo-documented `mobile-station` toolkit is still the target environment for this spike.

See `docs/checkpoints/2026-08-03-kirin-mobile-spike-env-checkpoint.md` for the current state and evidence.

For physical-device validation commands, see `docs/kirin-real-device-test-playbook.md`.

For the naked `.omc` runner path, see `docs/kirin-naked-omc-test-playbook.md`.

Scripted HDC test wrapper:

```bash
scripts/test-harmonyos-pc.sh --hap /path/to/entry-default-signed.hap
```

Scripted naked OMC runner wrapper:

```bash
scripts/test-naked-omc-vetest.sh --bundle-dir /path/to/kirin-sobel-naked-omc-2026-08-03
```
