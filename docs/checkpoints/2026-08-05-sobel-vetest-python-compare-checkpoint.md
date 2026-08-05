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
- `scripts/profile-naked-omc-vetest.sh` uses the same Python compare behavior.
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
[kirin-naked-omc] PASS_CANDIDATE
```

The evidence `compare.log` should include `decision=PASS_ACCURACY_THRESHOLD`.
