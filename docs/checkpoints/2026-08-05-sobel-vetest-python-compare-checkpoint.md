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
- Keep the shell wrappers operator-agnostic: do not select validators from
  bundle/OMC names.
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

- `scripts/test-naked-omc-vetest.sh` now runs a Python validator only when
  `COMPARE_SCRIPT` is provided by the bundle or CLI.
- `scripts/profile-naked-omc-vetest.sh` uses the same Python compare behavior
  and fails if compare was requested but the pulled output is missing or empty.
- `scripts/package-naked-omc-bundle.sh` can write `COMPARE_SCRIPT`.
- `docs/kirin-naked-omc-test-playbook.md` documents Python/numpy Sobel compare.
- Kirin9030 Sobel vector-fix bundles should carry `COMPARE_SCRIPT` in
  `bundle.env`, or the run command must pass `--compare-script`.

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

## Profiling enable_item follow-up

Issue #17 compact report from `profile_20260805_161905` showed:

- model execution passed: `RUN_STATUS=0`;
- Python precision compare passed: `compare_result=PASS`, `result=PASS`;
- profiling capture did not happen: `DATA_PROC_STATUS=NO_PROFILE_DIR`;
- auto mode selected no profiling flag: `PROFILING_ARG=`;
- the target `model_run_tool --help` exposes `--enable_item` as the profiling
  switch, with `--enable_item=1` enabling profiling.

Fix: `scripts/target-profile-omc.sh` now auto-detects `--enable_item` and runs
`model_run_tool` with `--enable_item=1`. It also runs the command from the run
directory so tools that emit relative profiling folders are captured under the
same evidence archive.

## Strict UINT8 output contract follow-up

Issue #4 later exposed that the real-device `output_0` could be exactly 4x the
Sobel tensor byte count while the first tensor-sized span matched the numpy
reference. That made the old candidate-format compare output too permissive and
too confusing for the real contract.

Decision:

- The inference wrapper must request the expected output dtype from
  `model_run_tool` only when `OUTPUT_TYPE` is provided by bundle metadata or
  CLI. It does not infer dtype from operator names.
- `bundle.env` may now carry `OUTPUT_TYPE`; the host test wrapper and profiling
  wrapper both read it.
- `scripts/compare-sobel-output.py` is the precision contract. It checks exactly
  one output representation: `uint8` shape `[1, 1, 761, 1022]`, expected size
  `777742` bytes.
- Any pulled output whose file size is not exactly `777742` bytes now fails with
  `decision=FAIL_OUTPUT_SIZE_MISMATCH`; the script no longer reports
  `best_candidate`, `raw_uint8_prefix`, `stride4`, `uint32`, or `float32`
  alternatives.
- A passing run should show `output_size_status=EXACT_TENSOR_SIZE`,
  `comparison.output_vs_golden.passes_threshold=true`, and
  `decision=PASS_ACCURACY_THRESHOLD`.

Validation:

- `bash -n` passed for the host wrappers and bundle packager.
- `sh -n` passed for `scripts/target-profile-omc.sh`.
- `python3 -m py_compile scripts/compare-sobel-output.py` passed.
- `shellcheck` passed for the shell scripts.
- A npu-group-3 numpy fixture proved strict compare behavior:
  exact-size output with max diff 1 returns `decision=PASS_ACCURACY_THRESHOLD`;
  exact-size output with max diff 6 returns `decision=FAIL_ACCURACY_THRESHOLD`;
  4x output returns `decision=FAIL_OUTPUT_SIZE_MISMATCH`.
- Dry-runs proved bundle metadata `OUTPUT_TYPE=UINT8` makes profiling pass
  `--output-type 'UINT8'` to the target helper, and the target helper runs
  `model_run_tool` with `--output_type=UINT8`.

## Operator-agnostic runner follow-up

The host wrappers briefly had Sobel/Soble name-based fallbacks for
`COMPARE_SCRIPT` and `OUTPUT_TYPE`. That violated the generic bundle model: the
runner should not know which operator or kernel is under test.

Current contract:

- `OUTPUT_TYPE` comes from `bundle.env`, environment, or `--output-type`.
- `COMPARE_SCRIPT` comes from `bundle.env`, environment, or `--compare-script`.
- If `COMPARE=1` and no `COMPARE_SCRIPT` is available, the wrapper fails instead
  of guessing.
- The packager only defaults `COMPARE=1` when both `--golden` and
  `--compare-script` are provided; explicit `--compare 1` requires
  `--compare-script`.
- A Sobel bundle is responsible for declaring `OUTPUT_TYPE=UINT8` and
  `COMPARE_SCRIPT=compare-sobel-output.py` if it wants strict Sobel validation.

## Sobel bundle metadata artifact refresh

Audit result: the local Kirin9030 Sobel bundle directories had been updated to
the metadata-driven contract, but the release zip assets were still old. The old
zip manifests only had `OUTPUT_NAME=output_0` and `COMPARE=1`; they did not
carry `OUTPUT_TYPE=UINT8`, `COMPARE_SCRIPT=compare-sobel-output.py`, or the
Python validator file.

Fix applied:

- Added `OUTPUT_TYPE="UINT8"` to each Kirin9030 Sobel `bundle.env`.
- Added `COMPARE_SCRIPT="compare-sobel-output.py"` to each Kirin9030 Sobel
  `bundle.env`.
- Copied `scripts/compare-sobel-output.py` into each bundle directory.
- Refreshed `README.txt`, `SHA256SUMS`, release `.zip`, and `.zip.sha256` for:
  `kirin9030-sobel-custom-2026-08-04`,
  `kirin9030-sobel-custom-clamped-2026-08-04`,
  `kirin9030-sobel-custom-tilefix-2026-08-04`, and
  `kirin9030-sobel-custom-vector-fix-2026-08-04`.
- Uploaded the refreshed zip assets with `gh release upload --clobber` using the
  `RockmanZheng` GitHub account.

Validation:

- `shasum -a 256 -c SHA256SUMS` passed in all four bundle directories.
- `unzip -p <release.zip> <bundle>/bundle.env` shows `OUTPUT_TYPE=UINT8`,
  `COMPARE=1`, and `COMPARE_SCRIPT=compare-sobel-output.py`.
- `unzip -l <release.zip>` shows `compare-sobel-output.py`, `bundle.env`, and
  `SHA256SUMS`.
- Host test wrapper dry-run and profiling wrapper dry-run both resolve
  `OUTPUT_TYPE=UINT8` and the bundle-local `compare-sobel-output.py` without
  command-line overrides.
- GitHub release asset timestamps after upload:
  `2026-08-06T02:26:51Z`, `2026-08-06T02:26:56Z`,
  `2026-08-06T02:27:00Z`, and `2026-08-06T02:27:06Z`.
