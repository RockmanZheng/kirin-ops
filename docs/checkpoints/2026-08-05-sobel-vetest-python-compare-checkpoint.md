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
- Add compare mode resolution to `scripts/test-naked-omc-vetest.sh`:
  - `auto`: Sobel/Soble bundle names or OMC names use the Sobel comparator.
  - `tensor`: use a Python/numpy tensor validator selected by
    `COMPARE_VALIDATOR`.
  - `byte`: force byte-for-byte compare.
- Preserve `COMPARE=0` as output-pulled-only confirmation.
- Let bundle manifests optionally specify `COMPARE_MODE` and
  `COMPARE_VALIDATOR`.
- Keep prod-side validation on the host that runs `hdc`; the Python script
  validates the output pulled back from the real device.

Naming correction:

- `sobel` is an output-contract validator, not a comparison mode.
- The public mode name is now `tensor`; the selected validator is recorded as
  `compare_validator=sobel`.

## Current State

- `scripts/test-naked-omc-vetest.sh` now runs
  `scripts/compare-sobel-output.py` for `COMPARE_MODE=tensor` with
  `COMPARE_VALIDATOR=sobel`.
- `scripts/package-naked-omc-bundle.sh` can write `COMPARE_MODE` and
  `COMPARE_VALIDATOR`.
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
  --compare-mode tensor \
  --no-clear-logs
```

Expected success signal:

```text
[kirin-naked-omc] checking tensor output with Python/numpy validator: sobel
[kirin-naked-omc] golden compare passed (tensor/sobel)
[kirin-naked-omc] PASS_CANDIDATE
```

The evidence `compare.log` should include `decision=PASS_ACCURACY_THRESHOLD`.
