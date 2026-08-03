# Kirin Mobile Spike Environment Checkpoint

Date: 2026-08-03
Workspace: `/Users/zhenglewen/projects/kirin-ops`
Goal: run the first spike on a real physical HarmonyOS mobile phone with a Kirin chip.

## Executive State

We have completed the local workspace setup and created isolated remote CANN Docker environments under work ID `z84378291`.

DevEco Studio can open the existing nested HarmonyOS sample project at `cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble`. This was confirmed from the desktop GUI after opening the project.

The Linux Sobel custom-op to `.omc` spike has been attempted on the available `aarch64` remote containers. ONNX generation works, but both available CANN/Kirin-capable containers fail at the AscendC device-kernel compilation phase before a custom-op package can be installed. No `SobelCustom.omc` has been produced.

The repo-documented `x86_64` Kirin `mobile-station` toolkit and Bisheng compiler packages have been downloaded and extracted under `/data1/z84378291`. They are confirmed to contain KirinX90 subpackages, but they are not runnable on the current `aarch64` host without an `x86_64` Linux environment or binfmt/qemu emulation.

The repo quick-install flow was also tried in a dedicated Ubuntu 22.04 `aarch64` container. The exact `aarch64 mobile-station` package is still unavailable to us, but the public standard `Ascend-cann-toolkit_9.0.0_linux-aarch64.run` installs successfully and exposes `atc`, `ccec`, and `bisheng`. That standard toolkit still fails to compile this repo's Sobel `kirinx90` AscendC device kernel, so it does not unblock `.omc` generation.

It is not yet proven that the generated model will run on a physical Kirin phone. That remains the final target.

## Completed

- Created top-level workspace artifacts outside the nested `cann-recipes-harmony-infer` repo.
- Saved local macOS plan at `docs/local-macos-harmony-setup-plan.md`.
- Saved remote Docker environment record at `docs/npu-group-3-kirin-cann-docker-env.md`.
- Added local helper scripts:
  - `scripts/local-macos-env.sh`
  - `scripts/open-deveco-soble.sh`
  - `scripts/remote-cann-shell.sh`
  - `scripts/remote-cann-exec.sh`
- Added DevEco project guide:
  - `docs/deveco-project-open-guide.md`
- Installed and validated local macOS prerequisites:
  - CMake 4.4.2
  - Homebrew OpenJDK 17.0.20 via repo-local env script
- Installed and validated local DevEco/Harmony tooling:
  - DevEco Studio 6.1.1 at `/Applications/DevEco-Studio.app`
  - `ohpm 6.1.2.268`
  - `hvigorw 6.24.2`
  - `hdc Ver: 3.2.0d`
- Confirmed the repo already contains a DevEco/Harmony sample project:
  - `cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble`
- Confirmed DevEco Studio successfully opens that nested `Soble` project.
- Kept nested source repo clean through CLI setup; after DevEco project open/sync, local generated IDE/dependency changes appeared and remain uncommitted.
- Created isolated remote workspace:
  - `/data1/z84378291/kirin-ops`
  - `/data1/z84378291/artifacts`
  - `/data1/z84378291/cache`
- Created isolated remote Docker resources:
  - network: `z84378291-kirin-cann-net`
  - container: `z84378291-kirin-cann91`
  - image: `cann-9.1.0:v2`
- Created a second isolated probe container for CANN 9.0 comparison:
  - container: `z84378291-kirin-cann90`
  - image: `cann90-ascendc-image:snapshot_20260526_112929`
- Synced the workspace source to:
  - `/data1/z84378291/kirin-ops/cann-recipes-harmony-infer`
- Created Python venv for repo ONNX/data scripts:
  - `/data1/z84378291/cache/py311-cann-harmony`
- Added CANN Python runtime dependencies to the dedicated venv after ATC reported missing imports:
  - `decorator`
  - `attrs`
  - `psutil`
  - `scipy`
  - `sympy`
  - `absl-py`
- Ran the Sobel custom-op build and ATC spike in scratch copies under `/data1/z84378291/artifacts`, not in the source checkout.
- Downloaded the repo-documented Kirin `mobile-station` toolkit and standalone Bisheng compiler package to:
  - `/data1/z84378291/kirin-ops/downloads/Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run`
  - `/data1/z84378291/kirin-ops/downloads/cann-bisheng-compiler_9.0.0_linux-x86_64.run`
- Extracted both packages without executing installer payloads to:
  - `/data1/z84378291/kirin-ops/mobile-station/extracted-x86_64`
- Downloaded and installed the public standard `aarch64` CANN toolkit in a dedicated container:
  - package: `/data1/z84378291/kirin-ops/downloads/Ascend-cann-toolkit_9.0.0_linux-aarch64.run`
  - container: `z84378291-cann-quickinstall-aarch64`
  - install path inside container: `/opt/ascend-standard-aarch64`

## Verified Evidence

Remote container status:

```text
z84378291-kirin-cann91 cann-9.1.0:v2 Up
z84378291-kirin-cann90 cann90-ascendc-image:snapshot_20260526_112929 Up
```

Container CANN/toolchain:

```text
workdir: /data1/z84378291/kirin-ops
ASCEND_HOME_PATH=/usr/local/Ascend/cann-9.1.0
atc=/usr/local/Ascend/cann-9.1.0/bin/atc
ccec=/usr/local/Ascend/cann-9.1.0/bin/ccec
cmake version 4.3.0
```

CANN 9.0 probe container:

```text
container: z84378291-kirin-cann90
image: cann90-ascendc-image:snapshot_20260526_112929
arch: aarch64
ASCEND_HOME_PATH=/usr/local/Ascend/cann-9.0.0-beta.2
atc=/usr/local/Ascend/cann-9.0.0-beta.2/bin/atc
ccec=/usr/local/Ascend/cann-9.0.0-beta.2/bin/ccec
```

Container NPU visibility:

```text
npu-smi sees 8x Ascend 910B2, all Health=OK
```

Dedicated Python venv:

```text
onnx 1.22.0
opencv-python-headless/cv2 5.0.0
```

Local DevEco/Harmony desktop tooling:

```text
DevEco Studio 6.1.1
ohpm 6.1.2.268
hvigorw 6.24.2
hdc Ver: 3.2.0d
```

Existing DevEco project root:

```text
cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble
```

DevEco project open:

```text
User confirmed DevEco Studio opened the nested Soble project successfully on the desktop.
```

Sobel ONNX generation smoke:

```text
ops/ascendc/src/sobel_custom/test/create_onnx.py generated SobelCustom.onnx successfully
```

## Linux Sobel Kernel To OMC Spike

Date: 2026-08-03

Goal:

```text
Sobel custom-op source -> AscendC kernel binary/package -> ATC -> SobelCustom.omc
```

Execution boundary:

- Used scratch copies under `/data1/z84378291/artifacts`.
- Did not run the repo's installer against the source checkout.
- Did not install any toolkit into the system CANN paths.
- Did not modify other users' Docker containers, images, or workspaces.

CANN 9.1 attempt:

```text
container: z84378291-kirin-cann91
spike dir: /data1/z84378291/artifacts/sobel_omc_spike_20260803_070959
configure: passed
host/tiling build: passed
kernel binary build: failed
status: build_binary_status=2
```

First kernel errors:

```text
/usr/local/Ascend/cann-9.1.0/aarch64-linux/asc/impl/basic_api/utils/kernel_utils_macros.h:404:8: error: redefinition of 'int4x2_t'
/usr/local/Ascend/cann-9.1.0/aarch64-linux/asc/impl/.../kernel_operator_common_impl.h:1225:45: error: use of undeclared identifier 'ROUND_H'
```

ONNX generation and ATC:

```text
SobelCustom.onnx generated successfully
atc --soc_version=KirinX90 status: atc_status=255
ATC final error: No operator plugin is registered for Op: sobelCustom, optype: ai.onnx::11::SobelCustom.
```

Interpretation: ATC reached model parsing, but custom-op plugin registration was unavailable because the custom-op package was not built/installed.

CANN 9.0 probe attempt:

```text
container: z84378291-kirin-cann90
spike dir: /data1/z84378291/artifacts/sobel_omc_spike_cann90_20260803_151518
configure: passed
host/tiling build: passed
kernel binary build: failed
status: build_binary_status=2
```

First kernel errors:

```text
/usr/local/Ascend/cann-9.0.0-beta.2/aarch64-linux/asc/impl/basic_api/utils/kernel_utils_macros.h:410:8: error: redefinition of 'int4x2_t'
/usr/local/Ascend/cann-9.0.0-beta.2/aarch64-linux/asc/.../kernel_operator_sys_var_intf_impl.h:50:12: error: use of undeclared identifier 'GetBlockIdxImpl'
```

Current result:

```text
No custom-op `.run` package.
No installed Sobel custom-op plugin.
No `SobelCustom.omc`.
```

Likely classification:

```text
available aarch64 CANN server/probe images are not sufficient for this repo's Kirin Sobel device-kernel build
```

The repo documentation points to a `mobile-station` CANN toolkit flow for Kirin. The documented public package URL is x86_64. A guessed aarch64 `mobile-station` package URL returned access denied, so an aarch64 mobile-station toolkit is not currently confirmed available from that path.

## Kirin Mobile-Station Toolkit Download And Setup Attempt

Date: 2026-08-03

Repo-documented package:

```text
Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run
```

Repo-documented extra compiler package:

```text
cann-bisheng-compiler_9.0.0_linux-x86_64.run
```

Remote download location:

```text
/data1/z84378291/kirin-ops/downloads
```

Downloaded artifacts:

```text
Ascend-cann-toolkit_9.0.0_linux-x86_64-mobile-station.run
size: 901822286 bytes
sha256: 2433c209106723454fe4a09d6506e34925649cb08db0819eb360809a36f0c120

cann-bisheng-compiler_9.0.0_linux-x86_64.run
size: 179542048 bytes
sha256: 8b5984919eb2dec21a418cc4947f56c3b4091e8963b022f206a58b57ca74eb78
```

Extraction result:

```text
extracted root: /data1/z84378291/kirin-ops/mobile-station/extracted-x86_64
extracted size: about 1.5G
```

Important extracted Kirin subpackages:

```text
cann-kirinx90-ops-legacy_8.5.0_linux-x86_64.run
cann-kirinx90-ops-nn_9.0.0_linux-x86_64.run
cann-kirinx90-ops-transformer_9.0.0_linux-x86_64.run
cann-bisheng-compiler_9.0.0_linux-x86_64.run
cann-simulator_9.0.0_linux-x86_64.run
cann-ge-compiler_9.0.0_linux-x86_64.run
cann-ge-executor_9.0.0_linux-x86_64.run
cann-asc-devkit_9.0.0_linux-x86_64.run
cann-asc-tools_9.0.0_linux-x86_64.run
cann-tbe-tik_9.0.0_linux-x86_64.run
```

Compiler architecture check:

```text
extracted Bisheng binaries are x86-64 ELF executables
example: bisheng_compiler/bin/bisheng -> ELF 64-bit LSB executable, x86-64
example: hcc/bin/aarch64-target-linux-gnu-gcc -> ELF 64-bit LSB executable, x86-64
```

Host/container architecture check:

```text
npu-group-3 host arch: aarch64
local arm64 ubuntu image: docker.m.daocloud.io/library/ubuntu:22.04
local amd64 ubuntu image: swr.cn-north-4.myhuaweicloud.com/ddn-k8s/docker.io/library/ubuntu:22.04
binfmt status: enabled, but no qemu-x86_64 entry
amd64 container smoke: exec /usr/bin/uname: exec format error
```

Isolated install attempt:

```text
install path: /data1/z84378291/kirin-ops/mobile-station/install-x86_64
log: /data1/z84378291/kirin-ops/mobile-station/logs/toolkit-install-x86_64-on-aarch64.log
status: toolkit_install_status=4
first failure: CANN subinstaller rejected private 750 workspace permissions for root/all-user install
```

Important safety note:

```text
The failed installer touched /etc/Ascend/ascend_cann_install.info before failing.
The previous metadata was restored from /etc/Ascend/20260803_153110/ascend_cann_install.info.
Restored content:
Install_Path=/usr/local/Ascend
Toolkit_InstallPath=/usr/local/Ascend/cann-8.5.2
```

Cleaner container install probe:

```text
container image: swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-910b-ubuntu22.04-py3.11
container lifecycle: throwaway probe, removed after logs were copied
install path inside container: /tmp/ascend-mobile-station
wrapper log: /data1/z84378291/kirin-ops/mobile-station/logs/toolkit-install-x86_64-in-cann-arm64-container-keep.log
copied CANN logs: /data1/z84378291/kirin-ops/mobile-station/logs/ascend_seclog_cann_arm64_container
status: container_toolkit_install_status=4
```

Clean architecture failure evidence:

```text
[AscTools] [2026-08-03 07:36:44] [ERROR]: ERR_NO:0x0001;ERR_DES: The architecture of the run package (x86_64) is inconsistent with that of the current environment (aarch64).
```

Standalone Bisheng compiler probe:

```text
container image: swr.cn-south-1.myhuaweicloud.com/ascendhub/cann:9.0.0-910b-ubuntu22.04-py3.11
package: cann-bisheng-compiler_9.0.0_linux-x86_64.run
install path inside container: /tmp/bisheng-install
wrapper log: /data1/z84378291/kirin-ops/mobile-station/logs/bisheng-install-x86_64-in-cann-arm64-container.log
copied CANN logs: /data1/z84378291/kirin-ops/mobile-station/logs/bisheng_ascend_seclog_cann_arm64_container
status: container_bisheng_install_status=1
error: The architecture of the run package (x86_64) is inconsistent with that of the current environment (aarch64).
```

Interpretation:

```text
The documented x86_64 mobile-station package cannot be set up on npu-group-3's aarch64 userspace.
Some earlier subpackages install because their wrapper/install scripts can run, but the full toolkit setup correctly fails once an architecture-checked package is reached.
The standalone x86_64 Bisheng compiler package also cannot be installed in the current aarch64 userspace.
```

Current result:

```text
The documented x86_64 mobile-station package is downloaded and extracted.
It is not installed or usable on npu-group-3 as of this checkpoint.
The current blocker is now confirmed as host/toolkit architecture mismatch:
we need a real x86_64 Linux environment for this package, or an accessible aarch64 mobile-station package.
```

## Quick Install Doc Trial

Date: 2026-08-03

Doc being exercised:

```text
cann-recipes-harmony-infer/docs/quick_install.md
```

Dedicated trial container:

```text
container: z84378291-cann-quickinstall-aarch64
image: docker.m.daocloud.io/library/ubuntu:22.04
arch: aarch64
OS: Ubuntu 22.04.5 LTS
glibc: 2.35
workspace mount: /data1/z84378291:/data1/z84378291
```

Step 1, `bash install_deps.sh`:

```text
log: /data1/z84378291/kirin-ops/quickinstall-aarch64/logs/install_deps.log
status: install_deps_status=1
```

The script successfully installed or validated:

```text
Python 3.10.12
pytest 9.1.1
pytest-cov 7.1.0
gcc/g++ 11.4.0
cmake 3.22.1
```

The script stopped at `ccache`:

```text
The ccache version provided by apt is too old.
```

Manual dependency completion in the same dedicated container:

```text
log: /data1/z84378291/kirin-ops/quickinstall-aarch64/logs/manual_deps.log
status: manual_deps_status=0
installed: gawk, dos2unix, pigz, lcov, ccache
note: Ubuntu 22.04 provides ccache 4.5.1, below the script's requested 4.8.2
```

Step 2, exact mobile-station package install:

```text
wanted: Ascend-cann-toolkit_9.0.0_linux-aarch64-mobile-station.run
status: not runnable because the package URL is still unavailable to us
direct URL status: 403 AccessDenied
```

Fallback step 2, standard public `aarch64` toolkit install:

```text
package: /data1/z84378291/kirin-ops/downloads/Ascend-cann-toolkit_9.0.0_linux-aarch64.run
sha256: c6e4c1332209fc04d92c7dd36acdb422536d15628c47b35cfc9338bf4f6eb0ae
install command: ./Ascend-cann-toolkit_9.0.0_linux-aarch64.run --install --quiet --force --install-path=/opt/ascend-standard-aarch64
log: /data1/z84378291/kirin-ops/quickinstall-aarch64/logs/install_standard_aarch64_toolkit.log
status: standard_aarch64_toolkit_install_status=0
installed size inside container: about 3.2G
```

Step 3, source environment:

```text
source /opt/ascend-standard-aarch64/cann/set_env.sh
ASCEND_HOME_PATH=/opt/ascend-standard-aarch64/cann-9.0.0
atc=/opt/ascend-standard-aarch64/cann-9.0.0/bin/atc
ccec=/opt/ascend-standard-aarch64/cann-9.0.0/bin/ccec
bisheng=/opt/ascend-standard-aarch64/cann-9.0.0/bin/bisheng
ccec/bisheng version: 2026-04-25T15:46:25+08:00 clang version 15.0.5
validation log: /data1/z84378291/kirin-ops/quickinstall-aarch64/logs/source_env_validate.log
```

Kirin support files present in the standard toolkit:

```text
/opt/ascend-standard-aarch64/cann-9.0.0/aarch64-linux/data/platform_config/KirinX90.ini
/opt/ascend-standard-aarch64/cann-9.0.0/aarch64-linux/data/platform_config/Kirin9030.ini
/opt/ascend-standard-aarch64/cann-9.0.0/aarch64-linux/simulator/KirinX90
/opt/ascend-standard-aarch64/cann-9.0.0/aarch64-linux/simulator/Kirin9030
```

Sobel build retry using this quick-install container:

```text
spike dir: /data1/z84378291/artifacts/sobel_omc_spike_standard_aarch64_20260803_160539
log: /data1/z84378291/artifacts/sobel_omc_spike_standard_aarch64_20260803_160539/build_binary.log
configure: passed
host/tiling build: passed
kernel binary build: failed
status: standard_aarch64_sobel_binary_status=2
```

First kernel errors:

```text
/opt/ascend-standard-aarch64/cann-9.0.0/aarch64-linux/asc/impl/basic_api/utils/kernel_utils_macros.h:409:8: error: redefinition of 'int4x2_t'
/opt/ascend-standard-aarch64/cann-9.0.0/aarch64-linux/asc/include/basic_api/../../impl/basic_api/kernel_operator_sys_var_intf_impl.h:50:12: error: use of undeclared identifier 'GetBlockIdxImpl'
```

Interpretation:

```text
The quick-install dependency/toolkit flow works for the public standard aarch64 CANN toolkit.
That standard toolkit is not equivalent to the repo-documented mobile-station toolkit.
It still does not produce the Sobel custom-op binary/package or SobelCustom.omc.
```

## Boundaries

- Do not modify other users' Docker containers, images, or workspaces.
- Do not use existing user paths such as `/data0/g00923713`, `/data0/m00953917`, `/data0/db00946854`, or `/data1/z60106022` for our work.
- Keep persistent work under `/data1/z84378291`.
- Keep source changes out of nested `cann-recipes-harmony-infer` until an explicit implementation step is approved.
- Do not add a fake `SobelCustom.omc`; the app must receive a real ATC-generated model.

## Current Local Source State

Opening/syncing the DevEco project created local generated state inside the nested repo. Current affected areas include:

- `harmony_infer/harmony_os_next/Soble/.idea/`
- `harmony_infer/harmony_os_next/Soble/.hvigor/`
- `harmony_infer/harmony_os_next/Soble/.clangd`
- `harmony_infer/harmony_os_next/Soble/.clang-tidy`
- `harmony_infer/harmony_os_next/Soble/oh_modules/`
- `harmony_infer/harmony_os_next/Soble/oh-package-lock.json5`
- `harmony_infer/harmony_os_next/Soble/entry/oh-package-lock.json5`

These are not yet classified as source changes to keep. Do not commit or discard them without a separate cleanup decision.

## Current Caveats

- The remote Docker environment is CANN 9.1 on a 910B server, not a Kirin mobile-station environment.
- It proves CANN/AscendC build tooling availability, not Kirin `.omc` compatibility.
- Available aarch64 CANN 9.1 and 9.0 probe environments contain KirinX90/Kirin9030 config files, but the Sobel device kernel still fails to compile.
- The repo's Sobel sample docs mention Kirin X90 and Kirin 9030, but the current Sobel preset and host registration are oriented to `kirinx90`.
- A physical HarmonyOS phone is not yet available, so real NPU execution on mobile is not verified.
- DevEco can open the nested `Soble` project, not the `cann-recipes-harmony-infer` repository root.
- The nested repo is currently dirty from DevEco-generated local project/dependency state.
- The DevEco `Soble` app supports phone `arm64-v8a`; it does not require x86_64. The x86_64 question applies to the Linux CANN development toolkit package documented by the repo, not to the phone app.
- The documented `x86_64` mobile-station toolkit contains KirinX90 subpackages, but its executable compiler/toolchain payload is `x86_64`.
- The current `aarch64` remote host does not have usable `x86_64` container emulation; an `amd64` Ubuntu container fails with `exec format error`.
- The public standard `aarch64` CANN toolkit installs and includes `cann-bisheng-compiler_9.0.0_linux-aarch64.run`, but it does not include the `cann-kirinx90-ops-*` subpackages found in the `x86_64 mobile-station` toolkit.
- The public standard `aarch64` CANN toolkit still fails the Sobel `kirinx90` device-kernel compile with the same class of AscendC/Bisheng header/compiler errors.

## Next Spike

1. Resolve the CANN toolkit mismatch before further source-level debugging:

   ```bash
   Need one of:
   - an aarch64 Kirin mobile-station CANN toolkit that works on npu-group-3
   - an x86_64 Linux development host/container for the documented mobile-station toolkit
   - binfmt/qemu-x86_64 enablement on npu-group-3, if allowed by machine owners
   - internal guidance for making the existing aarch64 CANN images compile KirinX90 AscendC kernels
   ```

2. If a working mobile-station toolkit is obtained, rerun the same scratch-copy build:

   - configure `ops/ascendc/src/sobel_custom`
   - build target `binary`
   - build package target
   - install the generated custom-op `.run` into the same isolated CANN environment
   - generate `SobelCustom.onnx`
   - run `atc --model=SobelCustom.onnx --framework=5 --output=... --soc_version=KirinX90`

3. Only after a real `.omc` exists, copy it into the DevEco app rawfile path and build the HAP.

## Phone Spike Acceptance

The first physical-phone spike is successful only when all are true:

- We know the phone model, HarmonyOS version, and Kirin SoC target.
- `SobelCustom.omc` is generated for that target SoC.
- The `.omc` is packaged under:

  ```text
  cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble/entry/src/main/resources/rawfile/SobelCustom.omc
  ```

- DevEco or command-line tools build and sign the HAP.
- The HAP installs on the real phone.
- The app launches `com.example.hdc_sobel_demo`.
- Tapping `NPU推理` produces output image and NPU timing.
- `hilog` includes:

  ```text
  LoadModelFromBuffer success
  InitIOTensors success
  OH_NNExecutor_RunSync success
  ```

## Re-entry Commands

Open the HarmonyOS sample in DevEco Studio:

```bash
./scripts/open-deveco-soble.sh
```

Load the local macOS toolchain:

```bash
source ./scripts/local-macos-env.sh
```

Open an interactive CANN shell:

```bash
scripts/remote-cann-shell.sh
```

Run one command in the remote CANN container:

```bash
scripts/remote-cann-exec.sh 'pwd; command -v atc; command -v ccec'
```

Check container status:

```bash
ssh npu-group-3 'docker ps --filter "name=^/z84378291-kirin-cann91$" --format "{{.Names}} {{.Image}} {{.Status}}"'
```

Check both dedicated CANN containers:

```bash
ssh npu-group-3 'docker ps --filter "name=^/z84378291-kirin-cann" --format "{{.Names}} {{.Image}} {{.Status}}"'
```

Inspect downloaded mobile-station packages and logs:

```bash
ssh npu-group-3 'ls -lh /data1/z84378291/kirin-ops/downloads && grep -RIn "architecture of the run package" /data1/z84378291/kirin-ops/mobile-station/logs/ascend_seclog_cann_arm64_container'
```

Enter the quick-install standard aarch64 CANN container:

```bash
ssh npu-group-3 'docker exec -it z84378291-cann-quickinstall-aarch64 bash'
source /opt/ascend-standard-aarch64/cann/set_env.sh
```

Inspect latest Sobel spike artifacts:

```bash
ssh npu-group-3 'cat /data1/z84378291/artifacts/latest_sobel_omc_spike_path /data1/z84378291/artifacts/latest_sobel_omc_spike_cann90_path'
```

Inspect latest standard aarch64 Sobel spike:

```bash
ssh npu-group-3 'cat /data1/z84378291/artifacts/latest_sobel_omc_spike_standard_aarch64_path'
```
