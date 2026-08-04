# Kirin HarmonyOS Real-Device Test Playbook

This playbook is for the first physical-device spike: install a HarmonyOS HAP that contains a prebuilt `SobelCustom.omc`, start the app, run NPU inference, and collect evidence from `hilog`.

It verifies the app/package/model-load/runtime path. It does not verify Linux CANN custom-op compilation.

## Scope

Target app:

```text
bundleName: com.example.hdc_sobel_demo
module: entry
ability: EntryAbility
```

Expected packaged model:

```text
resources/rawfile/SobelCustom.omc
resources/rawfile/SobelCustom.om
```

Current local prebuilt demo workspace:

```text
artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo
```

Current command-line build output:

```text
artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo/entry/build/default/outputs/default/entry-default-unsigned.hap
```

Published prerelease artifact:

```text
release: https://github.com/RockmanZheng/kirin-ops/releases/tag/kirin-sobel-hap-2026-08-03
tag: kirin-sobel-hap-2026-08-03
target commit: a1ab36aeb9cf2f1fa85218365fd62846ff99554f
asset: entry-default-unsigned.hap
sha256: 5705dc6f945f2bbc72af72f7fafa6553c7bd3ffe8b84e2de87cc15dc27d179ba
```

Important: `entry-default-unsigned.hap` is unsigned. A real HarmonyOS device will usually reject it. For device installation, prefer a DevEco-built debug/signed HAP, or configure `signingConfigs` and generate a signed HAP.

## Preconditions

- A Huawei HarmonyOS phone, tablet, or 2in1/PC with Kirin/NPU support.
- HarmonyOS 5.0.5 Release or later is the upstream demo's documented baseline.
- Developer mode / HDC debugging is enabled on the target device.
- The host machine running these commands has `hdc`.
- The HAP is signed for the target device, unless the target test system explicitly allows unsigned packages.
- Optional but useful: `gh` is authenticated to GitHub for downloading private release assets.

Canonical topology:

```text
host with hdc + HAP  ->  HarmonyOS target device
```

If the Kirin HarmonyOS PC is itself the target, the same validation still needs an HDC-capable shell/host path or an equivalent DevEco/App Installer path. The commands below assume an HDC host controlling a target device.

## Get Scripts And HAP On A Fresh Host

The helper scripts and playbook are uploaded to the private repo on `main`. Clone the ops repo first:

```bash
git clone git@github.com-kirin-ops:RockmanZheng/kirin-ops.git
cd kirin-ops
git checkout main
```

If you do not use the repo-specific deploy-key SSH alias, clone with any authenticated GitHub method that works on that host:

```bash
git clone https://github.com/RockmanZheng/kirin-ops.git
cd kirin-ops
```

Confirm the expected scripts are present:

```bash
ls -l scripts/test-harmonyos-pc.sh scripts/local-macos-env.sh scripts/remote-cann-shell.sh scripts/remote-cann-exec.sh
```

Download the uploaded HAP and checksum from the prerelease:

```bash
gh auth status -h github.com
mkdir -p artifacts/release-downloads/kirin-sobel-hap-2026-08-03
gh release download kirin-sobel-hap-2026-08-03 \
  --repo RockmanZheng/kirin-ops \
  --pattern 'entry-default-unsigned.hap*' \
  --dir artifacts/release-downloads/kirin-sobel-hap-2026-08-03
```

Because `RockmanZheng/kirin-ops` is private, direct release asset downloads require GitHub authentication through `gh`, a browser session, or an API token. A deploy key is enough for `git clone` over SSH, but it does not authenticate HTTPS release asset downloads.

Verify the downloaded HAP:

```bash
cd artifacts/release-downloads/kirin-sobel-hap-2026-08-03
if command -v shasum >/dev/null 2>&1; then
  shasum -a 256 -c entry-default-unsigned.hap.sha256
else
  sha256sum -c entry-default-unsigned.hap.sha256
fi
export HAP="$PWD/entry-default-unsigned.hap"
cd -
```

Expected checksum result:

```text
entry-default-unsigned.hap: OK
```

Use this released unsigned HAP for transfer, package inspection, or targets that allow unsigned installation. If the target rejects it, rebuild/sign in DevEco and set `HAP` to the signed HAP path instead.

## Prepare Variables

Set these on the host that will install and verify the app:

```bash
export HAP="/absolute/path/to/entry-default-signed.hap"
export BUNDLE="com.example.hdc_sobel_demo"
export ABILITY="EntryAbility"
export LOG_RE="HiAIFoundationDemo|CANNKitDemo|LoadModelFromBuffer|InitIOTensors|OH_NNExecutor_RunSync|GetResult|HIAI|NN|compibility|compatibility|failed"
```

For the unsigned local artifact on this Mac:

```bash
export HAP="/Users/zhenglewen/projects/kirin-ops/artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo/entry/build/default/outputs/default/entry-default-unsigned.hap"
```

For the uploaded release artifact after running the download commands above:

```bash
export HAP="/absolute/path/to/kirin-ops/artifacts/release-downloads/kirin-sobel-hap-2026-08-03/entry-default-unsigned.hap"
```

Use that unsigned artifact only for package inspection or for systems that permit unsigned HAP installation.

## Scripted Run

The repo includes an executable wrapper for the install/start/log workflow. It is committed and pushed on `main`:

```bash
cd /absolute/path/to/kirin-ops
scripts/test-harmonyos-pc.sh --hap /absolute/path/to/entry-default-signed.hap
```

If more than one HDC target is connected:

```bash
scripts/test-harmonyos-pc.sh \
  --target <target-id-from-hdc-list-targets> \
  --hap /absolute/path/to/entry-default-signed.hap
```

The script writes evidence under:

```text
artifacts/real-device-runs/<timestamp>/
```

During the log capture window, tap `NPU推理` in the app UI. The script defaults to strict mode: it exits nonzero unless these log markers are found:

```text
LoadModelFromBuffer success
InitIOTensors success
OH_NNExecutor_RunSync success
GetResult success
```

For a package/install smoke that should not fail on missing runtime markers:

```bash
scripts/test-harmonyos-pc.sh --hap /absolute/path/to/entry-default-signed.hap --no-strict
```

For the downloaded prerelease HAP:

```bash
scripts/test-harmonyos-pc.sh \
  --hap artifacts/release-downloads/kirin-sobel-hap-2026-08-03/entry-default-unsigned.hap \
  --no-strict
```

Remove `--no-strict` only when the target can install the package and you are ready to tap `NPU推理` during the log capture window.

If tracing shows the script stopped at `hdc -t <target> hilog -r`, skip the optional log-clear step:

```bash
scripts/test-harmonyos-pc.sh \
  --target <target-id-from-hdc-list-targets> \
  --hap /absolute/path/to/entry-default-signed-or-accepted.hap \
  --no-clear-logs \
  --no-strict
```

`--no-clear-logs` still captures `hilog`; it only avoids clearing old logs before the capture window.

## Verify HAP Contents Before Transfer

Run this before copying the HAP to another machine:

```bash
unzip -l "$HAP" | grep -E 'resources/rawfile/SobelCustom\.(omc|om)|libs/arm64-v8a/libentry\.so|module\.json'
```

Expected evidence:

```text
resources/rawfile/SobelCustom.omc
resources/rawfile/SobelCustom.om
libs/arm64-v8a/libentry.so
module.json
```

If `SobelCustom.omc` is missing, do not continue with device testing. The app will not load the target model.

## Connect Device

List targets:

```bash
hdc list targets
```

If one device is connected, commands may work without `-t`. For reproducibility, set the target explicitly:

```bash
export TARGET="<target-id-from-hdc-list-targets>"
```

For TCP/IP HDC:

```bash
hdc tconn <device-ip>:<port>
hdc list targets
export TARGET="<device-ip>:<port>"
```

Record basic target info:

```bash
hdc -t "$TARGET" shell param get const.product.model
hdc -t "$TARGET" shell param get const.product.name
hdc -t "$TARGET" shell param get const.product.software.version
hdc -t "$TARGET" shell param get const.ohos.apiversion
hdc -t "$TARGET" shell uname -a
```

Some commercial HarmonyOS builds may not expose every `param` key. Record the output and any missing-key errors.

## Install

Optional cleanup:

```bash
hdc -t "$TARGET" uninstall "$BUNDLE" || true
```

Install or replace:

```bash
hdc -t "$TARGET" install -r "$HAP"
```

Expected result: install succeeds without signature, profile, SDK, or compatibility errors.

If installation fails with signing/profile text, regenerate a signed/debug HAP in DevEco and retry. That failure is about package trust, not proof that the `.omc` is bad.

## Confirm Install

Check that the bundle is registered:

```bash
hdc -t "$TARGET" shell bm dump -a | grep "$BUNDLE"
```

If `bm dump -a` is too noisy or unavailable on that build, use the device UI to confirm the app icon appears, then continue to the launch step.

## Start App

Start by bundle and ability:

```bash
hdc -t "$TARGET" shell aa start -a "$ABILITY" -b "$BUNDLE"
```

Expected result: the app opens to the Sobel demo UI.

If `aa start` fails but the app is installed, launch it from the device home screen and keep collecting logs.

## Capture Runtime Logs

Use one terminal for logs:

```bash
hdc -t "$TARGET" hilog | grep -E "$LOG_RE"
```

If log clearing is supported on the target, clear first:

```bash
hdc -t "$TARGET" hilog -r || true
```

Some HarmonyOS PC / HDC combinations hang on `hdc hilog -r`. If that happens, do not block the run on log clearing. Use:

```bash
scripts/test-harmonyos-pc.sh --target "$TARGET" --hap "$HAP" --no-clear-logs --no-strict
```

Then start the app again and watch the filtered stream.

## Run NPU Inference

In the app UI:

1. Confirm the app shows `CANN SobelFilter Demo`.
2. Tap `NPU推理`.
3. Wait for the processed edge-detection image.
4. Record the displayed `NPU运行时间`.
5. Optionally tap `CPU推理` for a comparison path.

Expected UI evidence:

```text
CANN SobelFilter Demo
NPU运行时间 ： <number>ms
processed Sobel edge image is displayed
```

Expected log evidence:

```text
LoadModelFromBuffer success
InitIOTensors success
OH_NNExecutor_RunSync success
GetResult success
```

The code also logs model compatibility as `model compibility is <value>`. Record that value exactly.

## Acceptance Criteria

Treat the spike as passed only if all of these are true:

- The HAP installed on the target device.
- The app launched as `com.example.hdc_sobel_demo`.
- The packaged `SobelCustom.omc` was present in the HAP.
- `LoadModelFromBuffer success` appeared in `hilog`.
- Tapping `NPU推理` produced `OH_NNExecutor_RunSync success`.
- `GetResult success` appeared or the UI displayed the processed Sobel image.
- The UI showed `NPU运行时间`.

Do not mark the real-device spike as passed from installation alone, app launch alone, or CPU inference alone.

## Failure Triage

Installation fails with signature/profile errors:

```text
Likely cause: unsigned HAP or wrong signing profile.
Action: open the project in DevEco, configure debug signing, Build/Run, or export a signed HAP.
```

Observed example:

```text
error: failed to install bundle. code:9568320 error: no signature file.
```

This means the uploaded prerelease `entry-default-unsigned.hap` is not installable on that target as-is. It must be signed in DevEco or replaced with a target-accepted debug/signed HAP before app launch and NPU inference can be tested.

Installation fails with SDK/compatibility errors:

```text
Likely cause: target HarmonyOS version is below the demo baseline or target SDK mismatch.
Action: record OS/API version and try a HarmonyOS 5.0.5+ target.
```

App launches but model load fails:

```text
Likely causes:
- resources/rawfile/SobelCustom.omc missing from package
- .omc target SoC/runtime incompatible with device
- HiAI/CANN Kit runtime not available
Action:
- verify HAP contents with unzip
- capture hilog around LoadModelFromBuffer and compatibility check
```

NPU path fails but CPU path works:

```text
Likely cause: app/UI is fine, but NPU model/runtime path failed.
Action:
- capture OH_NNExecutor_RunSync failure logs
- record device model, HarmonyOS version, and model compatibility value
```

`hdc list targets` is empty:

```text
Likely causes:
- target not connected
- HDC debugging disabled
- wrong USB/TCP mode
Action:
- enable developer/HDC debugging on the device
- reconnect USB or run hdc tconn for TCP mode
```

Script trace stops at `hdc -t <target> hilog -r`:

```text
Likely cause: the target/HDC build hangs while clearing hilog.
Action:
- rerun with --no-clear-logs
- keep log capture enabled so hilog.raw.log and hilog.filtered.log are still saved
- inspect artifacts/real-device-runs/<timestamp>/hilog-clear.log if present
```

## Evidence Bundle To Save

For every run, save:

```text
device model
HarmonyOS version / API version
HAP filename and SHA256
install command output
aa start command output
hilog excerpt
photo/screenshot of UI after NPU推理
pass/fail conclusion
```

Useful host commands:

```bash
shasum -a 256 "$HAP"
hdc -t "$TARGET" list targets -v
hdc -t "$TARGET" bugreport "kirin-sobel-bugreport.txt"
```

If `hdc -t "$TARGET" list targets -v` is rejected by your HDC version, use:

```bash
hdc list targets -v
```

## Rebuild Unsigned HAP On Mac

From this repo:

```bash
cd /Users/zhenglewen/projects/kirin-ops
source scripts/local-macos-env.sh
cd artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo
hvigorw assembleHap --mode module -p module=entry@default --no-daemon --stacktrace
```

Output:

```text
entry/build/default/outputs/default/entry-default-unsigned.hap
```

Again, this unsigned HAP is for package inspection unless the target accepts unsigned packages.
