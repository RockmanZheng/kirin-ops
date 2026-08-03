# Local macOS HarmonyOS Setup Plan

Status: local DevEco/OpenHarmony desktop setup completed enough for project open/sync
Scope: this `kirin-ops` workspace and this desktop only. Linux CANN/ATC setup and physical phone validation are deferred.

## Goal

Prepare the macOS desktop to open, inspect, and build the HarmonyOS Next sample app at:

```text
cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble
```

This local setup does not generate the CANN `.omc` model and does not prove NPU execution. Those require a Linux CANN/ATC toolchain and later a supported HarmonyOS phone.

## Current Local State

- Host: macOS 26.4, Apple Silicon arm64.
- Disk: sufficient after cleanup, around 318 GiB free at plan creation.
- Installed: Homebrew, Python 3, Node/npm, Docker Desktop.
- Installed during setup: CMake 4.4.2, Homebrew OpenJDK 17.0.20, and DevEco Studio 6.1.1.
- Local Huawei command-line tools are available under `command-line-tools`.
- Added workspace-level shell helper: `scripts/local-macos-env.sh`.
- Added DevEco project opener: `scripts/open-deveco-soble.sh`.
- Added project-open guide: `docs/deveco-project-open-guide.md`.
- No files should be added to the nested `cann-recipes-harmony-infer` repo during this local setup phase.
- Available local commands:
  - `ohpm 6.1.2.268`
  - `hvigorw 6.24.2`
  - `hdc 3.2.0d`
- Nested CANN repo state should remain clean `master...origin/master`.

## Repo Requirements

- The app declares `compatibleSdkVersion: 5.0.1(13)` and `targetSdkVersion: 6.0.0(20)` in `cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble/build-profile.json5`.
- Native build is enabled through `entry/build-profile.json5` and `entry/src/main/cpp/CMakeLists.txt`.
- The native library links HarmonyOS/HMS NDK libraries: `hiai_foundation`, `libneural_network_core.so`, `libace_napi.z.so`, `libhilog_ndk.z.so`, and `librawfile.z.so`.
- The sample expects `entry/src/main/resources/rawfile/SobelCustom.omc`, but the file is intentionally absent until CANN/ATC conversion is done.

## Official Tooling Context

- DevEco Studio is Huawei's HarmonyOS IDE and includes HarmonyOS SDK management plus app build/debug workflows.
- Huawei's command-line app build docs use Hvigor for CI-style builds.
- Huawei's command-line tools docs describe downloading Command Line Tools separately; DevEco Studio also embeds SDK/tooling.
- The project README requires DevEco Studio 6.0.0 Release or later and HarmonyOS SDK 6.0.0 Release SDK or later.

Reference URLs:

- https://developer.huawei.com/consumer/en/deveco-studio/
- https://developer.huawei.com/consumer/en/doc/harmonyos-releases/deveco-studio-new-features-600
- https://developer.huawei.com/consumer/en/doc/harmonyos-guides/ide-commandline-get
- https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/ide-command-line-building-app
- https://developer.huawei.com/consumer/cn/download/command-line-tools-for-hmos

## Local macOS Setup Steps

1. Install non-Huawei local prerequisites.

   ```bash
   brew install cmake openjdk@17
   ```

   Completed locally.

   Notes:
   - DevEco Studio normally bundles a runtime, but a local JDK is useful for CLI validation and build tooling fallbacks.
   - CMake is useful for native build diagnostics, even if DevEco uses its own SDK/toolchain paths.

2. Configure local shell helpers.

   Add JDK 17 to the interactive shell only if needed:

   ```bash
   source scripts/local-macos-env.sh
   java -version
   cmake --version
   ```

   Completed locally through `scripts/local-macos-env.sh`.

3. Install DevEco Studio for macOS Apple Silicon from Huawei's official download page.

   Expected installation target:

   ```text
   /Applications/DevEco-Studio.app
   ```

   First-launch actions:
   - Install HarmonyOS SDK `6.0.0(20)` or newer.
   - Install Native SDK/NDK components.
   - Install HMS/native components if offered.
   - Skip emulator images for now; the target is build readiness without a phone.

   Completed locally.

4. Install or locate command-line tools.

   Completed locally. The workspace has a local command-line tools bundle at:

   ```text
   command-line-tools
   ```

   DevEco Studio also embeds `ohpm`, `node`, and `hdc` under:

   ```text
   /Applications/DevEco-Studio.app
   ```

   Expected commands:

   ```bash
   ohpm --version
   hvigorw --version
   hdc version
   ```

   Verified locally:

   ```text
   ohpm 6.1.2.268
   hvigorw 6.24.2
   hdc Ver: 3.2.0d
   ```

5. Open/import the sample app.

   Open this directory in DevEco Studio:

   ```text
   cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble
   ```

   Convenience command:

   ```bash
   ./scripts/open-deveco-soble.sh
   ```

   Let DevEco sync dependencies. Do not change SDK versions unless the IDE explicitly requires a compatible migration.

6. Prepare the missing rawfile directory.

   ```bash
   mkdir -p cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble/entry/src/main/resources/rawfile
   ```

   Do not add a fake `SobelCustom.omc`. The final file must come from CANN/ATC.

   Deferred until we are ready to touch the nested CANN repo.

7. Validate local build readiness.

   Without `.omc`, the native/ArkTS project can still be checked for IDE/toolchain readiness, but runtime model load will fail. Build validation target:

   ```bash
   cd cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble
   ohpm install
   hvigorw --mode module -p module=entry assembleHap
   ```

   If CLI task names differ by DevEco/Hvigor version, use DevEco's generated task list or build from the IDE.

   Deferred until we are ready to allow generated package/build artifacts in the nested source repo.

## Deferred Linux CANN/ATC Setup

Use `ssh npu-group-3` for Linux-side Ascend/CANN work. This host is Huawei Cloud EulerOS aarch64 with 8 Ascend 910B2 NPUs and Docker.

Current dedicated environment record:

```text
docs/npu-group-3-kirin-cann-docker-env.md
```

Checked convention on `npu-group-3`:

- Host exposes Docker and NPU driver tooling (`docker`, `npu-smi`), but not `atc` directly on host PATH.
- Existing CANN work is containerized. Running examples include:
  - `dbcann90_docker` from `cann90-ascendc-image:snapshot_20260526_112929`
  - `mcd_simulator_cann91` from `cann-9.1.0:v2`
  - `gxj-q7-0731` from `cann90-ascendc-image:gxj-ovpn-20260615`
- Those containers mount host Ascend driver/add-ons/log paths into the container and use `/workspace` as workdir.
- Inside the containers, CANN tools exist:
  - `atc` under `/usr/local/Ascend/cann-9.1.0/bin/atc` or `/usr/local/Ascend/cann-9.1.0-beta.1/bin/atc`
  - `ccec` under the same CANN tree
  - CMake under `/usr/local/python3.11.14/bin/cmake`
- Nearby established workspaces include `/data1/z60106022/cann-bench`, `/data1/z60106022/OpenOps`, and `/data1/remote-ws/OPS-HQ.worktrees`.
- Caveat: `npu-group-3` is a 910B server environment. Its containers prove CANN/AscendC Docker convention, but do not yet prove Kirin/mobile-station `.omc` compatibility. `atc --help` exposes generic `--soc_version` but did not list Kirin targets in the checked help output.

Conclusion: follow the machine convention and build/test kernels inside the dedicated `z84378291-kirin-cann91` CANN Docker container, with host only providing NPU devices, driver mounts, and persistent workspace storage.

Deferred outputs:

- Build/install `ops/ascendc/src/sobel_custom`.
- Generate `SobelCustom.onnx`.
- Convert to `SobelCustom.omc` with ATC and target SoC, for example `KirinX90`.
- Copy the real `.omc` into `entry/src/main/resources/rawfile/SobelCustom.omc`.

## Deferred Phone Validation

After a HarmonyOS phone is available:

- Install the signed HAP.
- Start `com.example.hdc_sobel_demo`.
- Click `NPU推理`.
- Verify `hilog` contains `LoadModelFromBuffer success`, `InitIOTensors success`, and `OH_NNExecutor_RunSync success`.
