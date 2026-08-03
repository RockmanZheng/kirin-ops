# kirin-ops

Private ops workspace for the Kirin/HarmonyOS CANN spike.

This repository is intentionally scoped to portable scripts, setup notes, and checkpoints. It does not vendor the upstream `cann-recipes-harmony-infer` checkout or local SDK/tool downloads. Those stay local and should be cloned/downloaded separately on each machine.

## Current Layout

- `docs/`: setup notes, DevEco guide, Docker notes, and checkpoints.
- `scripts/`: local helper scripts for DevEco/Harmony and remote CANN containers.

## Current Target

Run the first real-device spike on a HarmonyOS phone with a Kirin chip:

1. Build the Sobel custom operator on Linux.
2. Convert the model with ATC into a real `SobelCustom.omc`.
3. Package the HarmonyOS `Soble` app with DevEco.
4. Install and validate NPU inference on the physical phone.

## Known Boundary

The public standard `aarch64` CANN toolkit installs, but it has not compiled the repo's `kirinx90` Sobel custom operator. The repo-documented `mobile-station` toolkit is still the target environment for this spike.

See `docs/checkpoints/2026-08-03-kirin-mobile-spike-env-checkpoint.md` for the current state and evidence.
