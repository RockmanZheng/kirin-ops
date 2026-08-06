# Kirin9030 macOS Phone HDC Environment Hardening Plan

Date: 2026-08-06

Status: host-side hardening is prepared. The environment is still not
end-to-end ready until the phone is connected, authorized, and proves
Kirin9030 / Changsha chip identity.

## Scope

Use this Apple Silicon macOS desktop as the host for a physical HarmonyOS phone
with Kirin9030 / Changsha silicon, then run the existing naked OMC
bundle flow:

```bash
scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/kirin9030-sobel-custom-vector-fix-2026-08-04" \
  --device-dir "/data/local/tmp/z84378291" \
  --model-run-tool "/data/local/tmp/z84378291/model_run_tool" \
  --no-clear-logs
```

This plan is for host/device setup and acceptance hardening. It does not cover
rebuilding the Kirin9030 OMC.

## Current Readiness

Host facts verified on this Mac:

- Host OS: macOS 26.4, arm64.
- DevEco Studio exists at `/Applications/DevEco-Studio.app`.
- Huawei SDK exists at `/Users/zhenglewen/Library/Huawei/Sdk`.
- `hdc` is installed in DevEco SDK:
  `/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/toolchains/hdc`.
- `hdc` is also made available by `scripts/local-macos-env.sh` from the local
  command-line tools bundle.
- `hdc` version is `3.2.0d`.
- `hdc list targets -v` currently returns `[Empty]`.
- `system_profiler SPUSBDataType` did not show a connected Huawei/Harmony phone.
- The latest Kirin9030 Sobel vector-fix release zip carries the metadata-driven
  contract:
  `OUTPUT_TYPE="UINT8"`, `TARGET_SOC="kirin9030"`, `COMPARE="1"`, and
  `COMPARE_SCRIPT="compare-sobel-output.py"`.

Host Python facts:

- `/opt/homebrew/bin/python3` is Python `3.14.6` and currently has no `numpy`.
- `/usr/bin/python3` is Python `3.9.6` and has `numpy 2.0.2`.
- The wrappers currently call `python3` for host-side comparison. After
  `scripts/local-macos-env.sh`, Homebrew Python can precede `/usr/bin/python3`,
  so a Sobel compare can fail on this Mac even though a numpy-capable Python is
  present.

Conclusion: the Mac is close, but not plug-and-run ready.

## External Tooling Basis

Official HarmonyOS/HDC guidance used for this plan:

- Huawei debugging-device guidance says to enable USB debugging, use the HDC
  directory from the SDK/toolchains, run `hdc list targets`, and use
  `hdc -t <target> shell ...` for target-specific commands.
- Huawei HarmonyOS FAQ covers the common `hdc: command not found` / macOS HDC
  recognition class of problems.
- OpenHarmony `developtools_hdc` command help documents the commands used by
  this repo flow: `hdc list targets`, `hdc file send`, and `hdc file recv`.

Primary references:

- https://developer.huawei.com/consumer/en/doc/app/agc-help-add-device-0000001946142249
- https://developer.huawei.com/consumer/en/doc/harmonyos-faqs/faqs-ability-104
- https://developer.huawei.com/consumer/en/doc/harmonyos-faqs-V5/faqs-ability-112-V5
- https://gitee.com/openharmony/developtools_hdc

## Blocking Gaps

1. No physical target is connected.

   Evidence: `hdc list targets -v` returns `[Empty]`, and macOS USB profiler did
   not show a Huawei/Harmony phone.

2. We do not yet have chip evidence from the new phone.

   The prod-machine failure was caused by a swapped chip, so do not run or
   interpret OMC results until the new target proves its chip identity.

   Minimum command:

   ```bash
   hdc -t "$SN" shell param get ohos.boot.chiptype
   ```

   Acceptance: this must prove Kirin9030 or an equivalent Changsha
   device identity. If the target reports Kirin9020, stop.

3. The target-side `model_run_tool` is not guaranteed to exist on a retail or
   fresh phone.

   The repo flow depends on a device-local executable such as:

   ```text
   /data/local/tmp/z84378291/model_run_tool
   ```

   A new phone may need this binary pushed from an internal known-good source.
   It must be compatible with the phone OS/ABI and executable from
   `/data/local/tmp/z84378291`.

4. Host-side Python/numpy selection must be verified.

   The strict Sobel compare runs on the host after pulling `output_0`. This Mac
   has numpy under `/usr/bin/python3`, but not under the default Homebrew
   `python3` that can appear first in `PATH`. The wrappers now auto-select a
   numpy-capable Python or honor `--python-bin` / `PYTHON_BIN`.

5. Profiling wrapper macOS env sourcing is prepared.

   `scripts/test-naked-omc-vetest.sh` auto-sources `scripts/local-macos-env.sh`
   when `hdc` is not already in `PATH`. `scripts/profile-naked-omc-vetest.sh`
   now does the same.

6. SoC normalization handles Changsha.

   The test wrapper extracts and normalizes `kirinNNNN` tokens. It records
   `Changsha` in raw strings, and now normalizes that name to
   `kirin9030` for strict preflight. Bare `CHS` remains manual evidence unless
   it appears with a stronger Kirin9030/Changsha token.

## Hardening Plan

### Phase 0: Do Not Run Until Chip Proof Exists

After connecting the phone and accepting the device trust prompt, collect:

```bash
source scripts/local-macos-env.sh

scripts/check-kirin9030-phone-hdc-env.sh
```

If multiple targets are connected, pass the target explicitly:

```bash
SN="<target-id>"
scripts/check-kirin9030-phone-hdc-env.sh --target "$SN"
```

The helper writes a report under `artifacts/phone-env-checks/` and exits
nonzero until all readiness gates pass.

Manual minimum commands:

```bash
source scripts/local-macos-env.sh

hdc list targets -v
SN="<target-id>"

hdc -t "$SN" shell param get ohos.boot.chiptype
hdc -t "$SN" shell uname -m
hdc -t "$SN" shell 'param get const.product.model; param get const.product.name; param get const.product.software.version; param get const.ohos.apiversion'
```

Acceptance:

- `hdc list targets -v` shows exactly the intended phone and it is authorized.
- `uname -m` is `aarch64`.
- `ohos.boot.chiptype` or nearby device params prove Kirin9030/Changsha.

Do not use `--skip-soc-check` if the device reports Kirin9020. That is the same
class of failure as the swapped prod machine.

### Phase 1: Harden Host Toolchain Entry

Use this shell entrypoint before every local phone test:

```bash
cd /Users/zhenglewen/projects/kirin-ops
source scripts/local-macos-env.sh
hdc -v
hdc checkserver
hdc list targets -v
```

If `hdc` is still missing, call it directly:

```bash
/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/toolchains/hdc -v
```

Implemented code hardening:

- `scripts/profile-naked-omc-vetest.sh` auto-sources
  `scripts/local-macos-env.sh`, matching the test wrapper.
- `scripts/check-kirin9030-phone-hdc-env.sh` collects host HDC version, target
  list, chip params, writable/executable target directory, runner evidence,
  host Python/numpy, and bundle metadata in one compact report.
- The HDC target parser strips CR characters from macOS HDC output, so `[Empty]`
  is not misread as a target.

### Phase 2: Harden Host Python Selection

Short-term workaround for this Mac:

```bash
mkdir -p /tmp/kirin-python3-bin
ln -sf /usr/bin/python3 /tmp/kirin-python3-bin/python3

source scripts/local-macos-env.sh
PATH="/tmp/kirin-python3-bin:$PATH" python3 -c 'import numpy; print(numpy.__version__)'
```

Then run the wrapper from the same shell with `/tmp/kirin-python3-bin` first in
`PATH`.

Implemented code hardening:

- Both host wrappers honor `PYTHON_BIN` / `--python-bin`.
- If `PYTHON_BIN` is not set, they select the first Python interpreter that can
  import numpy from: `python3`, `/usr/bin/python3`, `python3.13`,
  `python3.12`, and `python3.11`.
- The selected Python path is recorded in compare logs, dry-run output, and
  summary evidence.

Acceptance:

```bash
python3 -c 'import numpy; print(numpy.__version__)'
```

must pass in the exact shell that runs `scripts/test-naked-omc-vetest.sh`.

### Phase 3: Bootstrap Target Runner

Use an isolated device directory:

```bash
REMOTE_DIR="/data/local/tmp/z84378291"
hdc -t "$SN" shell "mkdir -p $REMOTE_DIR && ls -ld $REMOTE_DIR"
```

If `model_run_tool` is not already present on the phone, push a known-good
HarmonyOS aarch64 build:

```bash
hdc -t "$SN" file send ./model_run_tool "$REMOTE_DIR/model_run_tool"
hdc -t "$SN" shell "chmod 755 $REMOTE_DIR/model_run_tool"
```

Verify:

```bash
hdc -t "$SN" shell "ls -l $REMOTE_DIR/model_run_tool; $REMOTE_DIR/model_run_tool --version 2>&1 || $REMOTE_DIR/model_run_tool --help 2>&1; echo runner_rc=\$?"
```

If `--help` or `--version` aborts but a previous runner is known to run models,
preserve the abort log as evidence and continue only with an explicit note. A
clean `--help` or usage output is preferred.

### Phase 4: Verify Bundle Locally

Download or use the latest release zip:

```bash
gh release download kirin9030-sobel-custom-vector-fix-2026-08-04 \
  --repo RockmanZheng/kirin-ops \
  --pattern 'kirin9030-sobel-custom-vector-fix-2026-08-04.zip'

shasum -a 256 kirin9030-sobel-custom-vector-fix-2026-08-04.zip
unzip -o kirin9030-sobel-custom-vector-fix-2026-08-04.zip

sed -n '1,80p' kirin9030-sobel-custom-vector-fix-2026-08-04/bundle.env
```

Required bundle metadata:

```text
OUTPUT_TYPE="UINT8"
TARGET_SOC="kirin9030"
COMPARE="1"
COMPARE_SCRIPT="compare-sobel-output.py"
```

### Phase 5: Dry-Run the Wrapper

Before touching the phone with files:

```bash
REMOTE_DIR="/data/local/tmp/z84378291"

scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/kirin9030-sobel-custom-vector-fix-2026-08-04" \
  --device-dir "$REMOTE_DIR" \
  --model-run-tool "$REMOTE_DIR/model_run_tool" \
  --no-clear-logs \
  --dry-run
```

Acceptance in dry-run output:

```text
OUTPUT_TYPE=UINT8
COMPARE=1
COMPARE_SCRIPT=<bundle>/compare-sobel-output.py
TARGET_SOC=kirin9030
```

### Phase 6: Run Accuracy Test

Run only after the above gates pass:

```bash
REMOTE_DIR="/data/local/tmp/z84378291"

scripts/test-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/kirin9030-sobel-custom-vector-fix-2026-08-04" \
  --device-dir "$REMOTE_DIR" \
  --model-run-tool "$REMOTE_DIR/model_run_tool" \
  --no-clear-logs
```

Acceptance:

- Console final status is `PASS`.
- Evidence `summary.txt` reports `result=PASS`.
- `compare.log` reports:

  ```text
  output_size_status=EXACT_TENSOR_SIZE
  comparison.output_vs_golden.passes_threshold=true
  decision=PASS_ACCURACY_THRESHOLD
  ```

- The runner command includes `--output_type=UINT8`.
- Evidence records the chip proof for Kirin9030/Changsha.

### Phase 7: Profiling Only After Accuracy PASS

Do not start profiling until Phase 6 passes.

Before profiling on macOS, either source the local env manually or land the
profile-wrapper auto-source hardening:

```bash
source scripts/local-macos-env.sh

scripts/profile-naked-omc-vetest.sh \
  --target "$SN" \
  --bundle-dir "$PWD/kirin9030-sobel-custom-vector-fix-2026-08-04" \
  --device-dir "$REMOTE_DIR" \
  --model-run-tool "$REMOTE_DIR/model_run_tool" \
  --target-soc Kirin9030 \
  --no-clear-logs
```

Profiling PASS does not replace accuracy PASS. It is a second evidence layer.

## Implemented Hardening

Prepared before the first serious macOS-phone run:

1. Added `PYTHON_BIN` / numpy-capable Python auto-selection to both host wrappers.
2. Auto-source `scripts/local-macos-env.sh` in the profiling wrapper.
3. Normalize `changsha` to `kirin9030` in SoC preflight.
4. Added `scripts/check-kirin9030-phone-hdc-env.sh` for one-command host/phone
   readiness evidence.
5. Hardened HDC `[Empty]` target parsing for macOS CR output.

The final claim must still include the exact chip proof, selected Python path,
and evidence archive.
