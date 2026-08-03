# DevEco Project Open Guide

## Answer

Do not open `cann-recipes-harmony-infer` itself in DevEco Studio. The repository root is not a HarmonyOS/OpenHarmony app project, so DevEco correctly rejects it as an unsupported project type.

Open this nested project instead:

```text
cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble
```

This is the existing HarmonyOS sample app project in the repo. We do not need a blank template for the first spike unless DevEco still refuses this nested project after SDK sync.

## Why This Directory Is The Project

The `Soble` directory has the expected DevEco/Harmony project markers:

- `build-profile.json5`
- `hvigorfile.ts`
- `oh-package.json5`
- `AppScope/app.json5`
- `entry/build-profile.json5`
- `entry/hvigorfile.ts`
- `entry/oh-package.json5`

Its product config targets HarmonyOS:

- `compatibleSdkVersion`: `5.0.1(13)`
- `targetSdkVersion`: `6.0.0(20)`
- `runtimeOS`: `HarmonyOS`
- native ABI filters: `arm64-v8a`, `x86_64`

## Open It

From the `kirin-ops` top-level workspace:

```bash
./scripts/open-deveco-soble.sh
```

Or manually in DevEco Studio:

1. Choose `Open`.
2. Select `cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble`.
3. Let DevEco sync/install required SDK and build tools.
4. Use the `entry` module as the app module.

## Local CLI Environment

From the `kirin-ops` top-level workspace:

```bash
source ./scripts/local-macos-env.sh
ohpm --version
hvigorw --version
hdc version
```

The environment script prefers the local `command-line-tools` bundle when present and falls back to the DevEco Studio app bundle for `ohpm`, `node`, and `hdc`.

## What To Expect

Opening and syncing the project is the local desktop goal. Running the native inference path on a phone still needs:

- A real HarmonyOS/Kirin phone attached and trusted over `hdc`.
- Correct app signing/debug profile in DevEco.
- A real ATC-generated `SobelCustom.omc` model placed under `entry/src/main/resources/rawfile/SobelCustom.omc`.
- Confirmation that the CANN/mobile toolchain can produce a Kirin-compatible custom-op/model artifact.

Do not add a fake `SobelCustom.omc`; that would only hide the real integration gap.
