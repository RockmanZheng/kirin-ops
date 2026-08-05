# 2026-08-05 Sobel vetest Python compare checkpoint

## Context

Issue #4 real Kirin9030 spike showed that `model_run_tool` can load and run
`SobelCustom_kirin9030.omc`, but `scripts/test-naked-omc-vetest.sh` still
reported compare failure.

The pulled `output_0` size in the latest prod comment was `3,110,968` bytes,
exactly `4 * 777,742`. Sobel's output tensor contract is uint8
`[1, 1, 761, 1022]`, so the effective tensor is `777,742` bytes. A whole-file
`cmp` can therefore misclassify a valid tensor when the runner writes a larger
dump with trailing bytes.

## Change Plan

- Keep generic OMC bundle behavior backward compatible.
- Retire byte-for-byte golden comparison from the host vetest wrappers.
- Treat `COMPARE=1` as "run the Python precision validator".
- Use `COMPARE_SCRIPT` when a bundle needs a specific validator script.
- Auto-select `scripts/compare-sobel-output.py` for Sobel/Soble bundles.
- Preserve `COMPARE=0` as output-pulled-only confirmation.
- Let bundle manifests optionally specify `COMPARE_SCRIPT`.
- Keep prod-side validation on the host that runs `hdc`; the Python script
  validates the output pulled back from the real device.

Naming correction history:

- `sobel` is not a comparison mode.
- `tensor` / `validator` / `contract` also over-specified the shell wrapper.
- The precision contract belongs inside the Python script.
- The shell wrapper now only chooses a Python script, runs it, and records its
  result.

## Current State

- `scripts/test-naked-omc-vetest.sh` now runs
  `scripts/compare-sobel-output.py` for Sobel bundles, or a bundle-provided
  `COMPARE_SCRIPT`.
- `scripts/profile-naked-omc-vetest.sh` uses the same Python compare behavior
  and fails if compare was requested but the pulled output is missing or empty.
- `scripts/package-naked-omc-bundle.sh` can write `COMPARE_SCRIPT`.
- `docs/kirin-naked-omc-test-playbook.md` documents Python/numpy Sobel compare.
- Existing Kirin9030 Sobel vector-fix bundles still work through auto detection
  because their names include `sobel`.

## Prod Retest Command

```bash
cd /root/z84378291/kirin-ops
git pull --ff-only

REMOTE_DIR=/data/local/tmp/z84378291

scripts/test-naked-omc-vetest.sh \
  --target SH258HS0727 \
  --bundle-dir "/root/z84378291/kirin9030-sobel-custom-vector-fix-2026-08-04" \
  --device-dir "$REMOTE_DIR" \
  --model-run-tool "$REMOTE_DIR/model_run_tool" \
  --no-clear-logs
```

Expected success signal:

```text
[kirin-naked-omc] checking output with Python precision validator: ...
[kirin-naked-omc] golden compare passed
[kirin-naked-omc] PASS: output was pulled and passed golden comparison.
```

The evidence `compare.log` should include `decision=PASS_ACCURACY_THRESHOLD`.

## PASS status and console color follow-up

Issue #4 latest real-device run on `2026-08-05T07:36:16Z` passed the Python precision validator:

```text
[kirin-naked-omc] checking output with Python precision validator: /root/z84378291/kirin-ops/scripts/compare-sobel-output.py
[kirin-naked-omc] golden compare passed
```

The prior final status was `PASS_CANDIDATE`; that is now promoted to `PASS` because the byte-compare path has been retired and Python precision validation is the precision contract.

Console color policy:

- Interactive terminals, including MobaXterm, auto-render ANSI colors.
- `FORCE_COLOR=1` or `CLICOLOR_FORCE=1` forces color.
- `NO_COLOR=1` disables auto-color for copy/paste logs and CI unless color is explicitly forced.

## Profiling wrapper review follow-up

Review finding: `scripts/profile-naked-omc-vetest.sh` already selected
`scripts/compare-sobel-output.py` for Sobel/Soble bundles, but its inline compare
block was weaker than `scripts/test-naked-omc-vetest.sh`:

- It did not preflight-check `python3`.
- It did not preflight-check `numpy`.
- It skipped compare when output pull failed, then could still exit success.

Fix: replace the inline block with the same Python precision validator flow used
by the test wrapper. A profiling run with `COMPARE=1` now only reports `PASS`
when `compare-sobel-output.py` returns success; missing/empty pulled output is a
compare failure.

## Profiling evidence helper follow-up

Issue #17 showed that asking for many manual `sed`/`find` commands is fragile.
Add `scripts/collect-profiling-evidence.sh` so the prod host can collect one
copy/paste diagnostics report from the latest `artifacts/profiling/profile_*`
run, or from a selected `--run-id` / `--run-dir`.

The default report is compact and focuses on the lines needed to debug the
profiling failure. Use `--full` only when the compact report is insufficient.

The helper reports:

- host-side `summary.txt`, `manifest.env`, `host-manifest.env`, `target-script.log`, `pull.log`, and `compare.log`;
- key fields such as `PROFILING_ARG`, `ADD_TIMES`, `DATA_PROC_STATUS`, `DATA_PROC_RESULT_PATH`, and `PROFILE_CANDIDATES_FILE`;
- target-side `command.txt`, tool help logs, run logs, profile candidates, and file listings;
- archive contents, including reading target-side files directly from a pulled `.tgz`/`.tar` when `target-run/` was not expanded locally.

Prod command:

```bash
cd /root/z84378291/kirin-ops
git pull --ff-only
scripts/collect-profiling-evidence.sh
```

Full fallback:

```bash
scripts/collect-profiling-evidence.sh --full
```
