#!/usr/bin/env bash
# Collect a compact readiness report for a macOS-hosted Kirin9030 phone test.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_BUNDLE_NAME="kirin9030-sobel-custom-vector-fix-2026-08-04"

TARGET="${TARGET:-}"
DEVICE_DIR="${DEVICE_DIR:-/data/local/tmp/z84378291}"
MODEL_RUN_TOOL="${MODEL_RUN_TOOL:-}"
BUNDLE_DIR="${BUNDLE_DIR:-}"
PYTHON_BIN="${PYTHON_BIN:-}"
REPORT="${REPORT:-}"

usage() {
  cat <<'USAGE'
usage: scripts/check-kirin9030-phone-hdc-env.sh [options]

Checks whether this host plus a connected HarmonyOS phone is ready for the
Kirin9030 naked OMC flow. The command writes a copy/paste report and exits
nonzero when a readiness gate fails.

Options:
  --target TARGET        hdc target id. Auto-selects when exactly one target is connected.
  --device-dir DIR       Device work dir. Default: /data/local/tmp/z84378291.
  --model-run-tool PATH  Device-side model_run_tool. Default: <device-dir>/model_run_tool.
  --bundle-dir DIR       Local Kirin9030 Sobel bundle dir.
  --python-bin PATH      Host Python interpreter for numpy precision validation.
  --report PATH          Report path. Default: artifacts/phone-env-checks/<timestamp>.txt.
  -h, --help             Show this help.

Example:
  source scripts/local-macos-env.sh
  scripts/check-kirin9030-phone-hdc-env.sh --target "$SN"
USAGE
}

log() {
  printf '[kirin-phone-env] %s\n' "$*"
}

warn() {
  printf '[kirin-phone-env] WARN: %s\n' "$*"
}

remote_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

resolve_executable() {
  local candidate="$1"

  case "${candidate}" in
    */*)
      [ -x "${candidate}" ] || return 1
      printf '%s\n' "${candidate}"
      ;;
    *)
      command -v "${candidate}" 2>/dev/null || return 1
      ;;
  esac
}

select_python_bin() {
  local candidate
  local resolved
  local -a candidates

  if [ -n "${PYTHON_BIN}" ]; then
    resolved="$(resolve_executable "${PYTHON_BIN}" || true)"
    [ -n "${resolved}" ] || return 1
    "${resolved}" -c 'import numpy' >/dev/null 2>&1 || return 1
    PYTHON_BIN="${resolved}"
    return 0
  fi

  candidates=(python3 /usr/bin/python3 python3.13 python3.12 python3.11)
  for candidate in "${candidates[@]}"; do
    resolved="$(resolve_executable "${candidate}" || true)"
    [ -n "${resolved}" ] || continue
    if "${resolved}" -c 'import numpy' >/dev/null 2>&1; then
      PYTHON_BIN="${resolved}"
      return 0
    fi
  done

  return 1
}

normalize_chip() {
  local value="$1"
  local lower

  lower="$(printf '%s' "${value}" | tr '[:upper:]' '[:lower:]')"
  case "${lower}" in
    *kirin*9030*|*changsha*|*q709030*)
      printf '%s\n' "kirin9030"
      ;;
    *kirin*9020*)
      printf '%s\n' "kirin9020"
      ;;
    *)
      printf '%s\n' ""
      ;;
  esac
}

append_failure() {
  local reason="$1"

  FAIL_COUNT=$((FAIL_COUNT + 1))
  warn "${reason}"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --target)
      [ "$#" -ge 2 ] || { warn "--target requires a value"; exit 2; }
      TARGET="$2"
      shift 2
      ;;
    --device-dir)
      [ "$#" -ge 2 ] || { warn "--device-dir requires a value"; exit 2; }
      DEVICE_DIR="$2"
      shift 2
      ;;
    --model-run-tool)
      [ "$#" -ge 2 ] || { warn "--model-run-tool requires a value"; exit 2; }
      MODEL_RUN_TOOL="$2"
      shift 2
      ;;
    --bundle-dir)
      [ "$#" -ge 2 ] || { warn "--bundle-dir requires a value"; exit 2; }
      BUNDLE_DIR="$2"
      shift 2
      ;;
    --python-bin)
      [ "$#" -ge 2 ] || { warn "--python-bin requires a value"; exit 2; }
      PYTHON_BIN="$2"
      shift 2
      ;;
    --report)
      [ "$#" -ge 2 ] || { warn "--report requires a path"; exit 2; }
      REPORT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      warn "unknown argument: $1"
      usage >&2
      exit 2
      ;;
  esac
done

if [ -z "${REPORT}" ]; then
  REPORT="${ROOT}/artifacts/phone-env-checks/$(date +%Y%m%d_%H%M%S).txt"
fi
mkdir -p "$(dirname "${REPORT}")"
exec > >(tee "${REPORT}") 2>&1

FAIL_COUNT=0

if ! command -v hdc >/dev/null 2>&1 && [ -f "${ROOT}/scripts/local-macos-env.sh" ]; then
  # shellcheck disable=SC1091
  source "${ROOT}/scripts/local-macos-env.sh" >/dev/null 2>&1 || true
fi

HDC_BIN="$(command -v hdc || true)"
if [ -z "${MODEL_RUN_TOOL}" ]; then
  MODEL_RUN_TOOL="${DEVICE_DIR}/model_run_tool"
fi

if [ -z "${BUNDLE_DIR}" ]; then
  if [ -d "${PWD}/${DEFAULT_BUNDLE_NAME}" ]; then
    BUNDLE_DIR="${PWD}/${DEFAULT_BUNDLE_NAME}"
  elif [ -d "${ROOT}/artifacts/naked-omc/${DEFAULT_BUNDLE_NAME}" ]; then
    BUNDLE_DIR="${ROOT}/artifacts/naked-omc/${DEFAULT_BUNDLE_NAME}"
  fi
fi

log "report: ${REPORT}"
log "timestamp: $(date -u '+%Y-%m-%dT%H:%M:%SZ')"

echo
echo "## Host"
echo "root=${ROOT}"
echo "uname=$(uname -a)"
if command -v sw_vers >/dev/null 2>&1; then
  sw_vers
fi
echo "hdc=${HDC_BIN:-<missing>}"
if [ -n "${HDC_BIN}" ]; then
  "${HDC_BIN}" -v 2>&1 || "${HDC_BIN}" version 2>&1 || true
  "${HDC_BIN}" checkserver 2>&1 || true
else
  append_failure "hdc not found; source scripts/local-macos-env.sh or install DevEco command-line tools"
fi

echo
echo "## Host Python"
if select_python_bin; then
  echo "python_bin=${PYTHON_BIN}"
  "${PYTHON_BIN}" - <<'PY'
import sys
import numpy as np
print("python_version=" + sys.version.split()[0])
print("numpy_version=" + np.__version__)
PY
else
  echo "python_bin=${PYTHON_BIN:-<none>}"
  append_failure "no Python interpreter with numpy found; set --python-bin or install numpy"
fi

echo
echo "## HDC Targets"
TARGETS_TEXT=""
if [ -n "${HDC_BIN}" ]; then
  TARGETS_TEXT="$("${HDC_BIN}" list targets 2>&1 || true)"
  printf '%s\n' "${TARGETS_TEXT}"
  echo
  "${HDC_BIN}" list targets -v 2>&1 || true
fi

if [ -n "${HDC_BIN}" ] && [ -z "${TARGET}" ]; then
  TARGET_COUNT=0
  FIRST_TARGET=""
  while IFS= read -r line; do
    line="${line%$'\r'}"
    [ "${line}" != "[Empty]" ] || continue
    target_word="$(printf '%s\n' "${line}" | awk 'NF {print $1}')"
    [ -n "${target_word}" ] || continue
    TARGET_COUNT=$((TARGET_COUNT + 1))
    [ -n "${FIRST_TARGET}" ] || FIRST_TARGET="${target_word}"
  done <<EOF
${TARGETS_TEXT}
EOF

  if [ "${TARGET_COUNT}" -eq 1 ]; then
    TARGET="${FIRST_TARGET}"
    log "auto-selected target: ${TARGET}"
  elif [ "${TARGET_COUNT}" -eq 0 ]; then
    append_failure "no hdc target connected or authorized"
  else
    append_failure "multiple hdc targets connected; pass --target"
  fi
fi

echo
echo "## Target"
echo "target=${TARGET:-<none>}"
echo "device_dir=${DEVICE_DIR}"
echo "model_run_tool=${MODEL_RUN_TOOL}"

if [ -n "${HDC_BIN}" ] && [ -n "${TARGET}" ]; then
  chip_text="$("${HDC_BIN}" -t "${TARGET}" shell param get ohos.boot.chiptype 2>&1 | tr -d '\r' || true)"
  chip_norm="$(normalize_chip "${chip_text}")"
  echo "ohos.boot.chiptype=${chip_text}"
  echo "normalized_chip=${chip_norm:-<unknown>}"
  if [ "${chip_norm}" != "kirin9030" ]; then
    append_failure "target chip is not proven Kirin9030/Changsha/Q709030"
  fi

  uname_m="$("${HDC_BIN}" -t "${TARGET}" shell uname -m 2>&1 | tr -d '\r' || true)"
  echo "uname_m=${uname_m}"
  if [ "${uname_m}" != "aarch64" ]; then
    append_failure "target uname -m is not aarch64"
  fi

  echo
  echo "### selected params"
  # shellcheck disable=SC2016
  "${HDC_BIN}" -t "${TARGET}" shell 'for key in const.product.model const.product.name const.product.software.version const.ohos.apiversion const.product.soc const.product.socversion const.soc_version ro.board.platform ro.hardware ro.soc.model ro.product.board const.product.hardwareversion; do printf "%s=" "$key"; param get "$key" 2>/dev/null || true; done' 2>&1 || true

  echo
  echo "### target directory probe"
  probe_cmd="mkdir -p $(remote_quote "${DEVICE_DIR}") && echo ok > $(remote_quote "${DEVICE_DIR}/.kirin_env_probe") && rm -f $(remote_quote "${DEVICE_DIR}/.kirin_env_probe") && ls -ld $(remote_quote "${DEVICE_DIR}")"
  if ! "${HDC_BIN}" -t "${TARGET}" shell "${probe_cmd}" 2>&1; then
    append_failure "device dir is not writable: ${DEVICE_DIR}"
  fi

  echo
  echo "### model_run_tool probe"
  runner_cmd="ls -l $(remote_quote "${MODEL_RUN_TOOL}") 2>&1; ($(remote_quote "${MODEL_RUN_TOOL}") --version 2>&1 || $(remote_quote "${MODEL_RUN_TOOL}") --help 2>&1); echo runner_rc=\$?"
  if ! "${HDC_BIN}" -t "${TARGET}" shell "${runner_cmd}" 2>&1; then
    append_failure "model_run_tool probe failed"
  fi
else
  append_failure "target checks skipped because no usable hdc target is selected"
fi

echo
echo "## Bundle"
echo "bundle_dir=${BUNDLE_DIR:-<none>}"
if [ -n "${BUNDLE_DIR}" ] && [ -f "${BUNDLE_DIR}/bundle.env" ]; then
  sed -n '1,120p' "${BUNDLE_DIR}/bundle.env"
  for required in 'OUTPUT_TYPE="UINT8"' 'TARGET_SOC="kirin9030"' 'COMPARE="1"' 'COMPARE_SCRIPT="compare-sobel-output.py"'; do
    if ! grep -Fqx "${required}" "${BUNDLE_DIR}/bundle.env"; then
      append_failure "bundle.env missing required line: ${required}"
    fi
  done
  if [ ! -f "${BUNDLE_DIR}/compare-sobel-output.py" ]; then
    append_failure "bundle missing compare-sobel-output.py"
  fi
else
  append_failure "Kirin9030 Sobel bundle is not present; download and unzip ${DEFAULT_BUNDLE_NAME}.zip"
fi

echo
if [ "${FAIL_COUNT}" -eq 0 ]; then
  echo "result=READY"
  exit 0
fi

echo "result=NOT_READY"
echo "failure_count=${FAIL_COUNT}"
exit 1
