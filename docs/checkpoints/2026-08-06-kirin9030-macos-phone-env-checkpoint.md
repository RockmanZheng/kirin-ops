# 2026-08-06 Kirin9030 macOS phone env checkpoint

## Context

The previous prod target was found to have a swapped chip. We need to move
physical Kirin9030 / Changsha validation to a HarmonyOS phone connected directly
to this macOS desktop.

## Local Audit

- Host: macOS 26.4, Apple Silicon arm64.
- DevEco Studio: `/Applications/DevEco-Studio.app`.
- DevEco HDC binary:
  `/Applications/DevEco-Studio.app/Contents/sdk/default/openharmony/toolchains/hdc`.
- Local command-line tools HDC after `source scripts/local-macos-env.sh`:
  `/Users/zhenglewen/projects/kirin-ops/command-line-tools/sdk/default/openharmony/toolchains/hdc`.
- HDC version: `3.2.0d`.
- Current target list: `[Empty]`.
- macOS USB scan did not show a Huawei/Harmony phone.
- Host Python:
  `/opt/homebrew/bin/python3` is `3.14.6` without numpy;
  `/usr/bin/python3` is `3.9.6` with `numpy 2.0.2`.
- Latest local Kirin9030 Sobel vector-fix bundle zip has:
  `OUTPUT_TYPE=UINT8`, `TARGET_SOC=kirin9030`, `COMPARE=1`, and
  `COMPARE_SCRIPT=compare-sobel-output.py`.

## Readiness Decision

Host-side preparation is hardened, but end-to-end readiness still waits for a
connected phone.

Ready-to-run requires:

1. A connected and authorized HDC target.
2. Explicit chip proof from the new phone:
   `hdc -t "$SN" shell param get ohos.boot.chiptype`.
3. Device-side runner present and executable under an isolated directory such
   as `/data/local/tmp/z84378291/model_run_tool`.
4. Host `python3` used by the wrapper can import numpy, or wrappers are hardened
   to choose a numpy-capable interpreter.
5. No use of `--skip-soc-check` if target params report Kirin9020.

## Hardening Implemented

- Added `scripts/check-kirin9030-phone-hdc-env.sh` as the first command to run
  when the phone arrives.
- Both host wrappers now honor `PYTHON_BIN` / `--python-bin` and auto-select a
  Python interpreter that can import numpy.
- `scripts/profile-naked-omc-vetest.sh` now auto-sources
  `scripts/local-macos-env.sh`, matching the test wrapper.
- SoC normalization maps `changsha` and `q709030` to `kirin9030`.
- HDC target parsing strips CR characters so macOS `[Empty]` output is not
  mistaken for a target.

Current no-phone helper result:

```text
python_bin=/usr/bin/python3
numpy_version=2.0.2
bundle metadata ok: OUTPUT_TYPE=UINT8, TARGET_SOC=kirin9030, COMPARE_SCRIPT=compare-sobel-output.py
result=NOT_READY
failure_count=2
```

The two expected failures are `no hdc target connected or authorized` and
`target checks skipped because no usable hdc target is selected`.

## Plan File

Detailed setup and hardening plan:

```text
docs/kirin9030-macos-phone-env-hardening-plan.md
```
