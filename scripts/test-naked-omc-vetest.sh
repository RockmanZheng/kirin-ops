#!/usr/bin/env bash
# Push a naked OMC plus input data to a HarmonyOS CANN vetest runner and execute it.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_BUNDLE_DIR="${ROOT}/artifacts/naked-omc/kirin-sobel-naked-omc-2026-08-03"
PREBUILT_OMC="${ROOT}/artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo/entry/src/main/resources/rawfile/SobelCustom.omc"

BUNDLE_DIR="${BUNDLE_DIR:-}"
OMC="${OMC:-}"
INPUT="${INPUT:-}"
GOLDEN="${GOLDEN:-}"
TARGET="${TARGET:-}"
RUNNER_HAP="${RUNNER_HAP:-}"
BUNDLE="${BUNDLE:-com.example.naticvetestdemo}"
ABILITY="${ABILITY:-EntryAbility}"
DEVICE_DIR="${DEVICE_DIR:-/mnt/hmdfs/100/account/device_view/local/files/Docs/Download/com.example.naticvetestdemo}"
OUTPUT_NAME="${OUTPUT_NAME:-output0.bin}"
LOG_SECONDS="${LOG_SECONDS:-30}"
HILOG_CLEAR_TIMEOUT="${HILOG_CLEAR_TIMEOUT:-5}"
EVIDENCE_DIR="${EVIDENCE_DIR:-}"
CAPTURE_LOGS=1
HILOG_CLEAR=1
CHECK_RUNNER=1
PULL_OUTPUT=1
COMPARE=1
STRICT=1
DRY_RUN=0
OMC_EXPLICIT=0
INPUT_EXPLICIT=0
GOLDEN_EXPLICIT=0
LOG_RE="${LOG_RE:-naticvetestdemo|omPath|output0|CANN|HIAI|NN|OH_NN|success|failed|ERROR|error}"

usage() {
  cat <<'USAGE'
usage: scripts/test-naked-omc-vetest.sh [options]

Sends a naked .omc file and an input .bin to a HarmonyOS native CANN
vetest runner, starts the runner with --ps path/--ps omPath, pulls output0.bin,
and optionally compares it with a golden file.

Options:
  --bundle-dir DIR       Directory containing SobelCustom.omc, x.bin, and optional y.bin.
  --omc PATH             OMC model file. Overrides --bundle-dir default.
  --input PATH           Input .bin file. Overrides --bundle-dir default.
  --golden PATH          Expected output .bin file. Overrides --bundle-dir default.
  --target TARGET        hdc target id. Auto-detects when exactly one target is connected.
  --runner-hap PATH      Optional signed runner HAP to install before running.
  --bundle NAME          Runner bundle name. Default: com.example.naticvetestdemo
  --ability NAME         Runner ability name. Default: EntryAbility
  --device-dir PATH      Device documents directory used by the runner.
  --output-name NAME     Output file name produced by the runner. Default: output0.bin
  --log-seconds N        Log capture window after runner start. Default: 30
  --evidence-dir DIR     Evidence directory. Default: artifacts/naked-omc-runs/<timestamp>
  --no-clear-logs        Do not run "hdc hilog -r" before capture.
  --hilog-clear-timeout N
                         Seconds to wait for "hdc hilog -r". Default: 5
  --skip-runner-check    Do not check whether the runner bundle is installed.
  --no-pull-output       Do not pull output0.bin from the device.
  --no-compare           Do not compare pulled output with the golden file.
  --no-logs              Do not capture hilog.
  --no-strict            Do not exit nonzero when output/log validation is incomplete.
  --dry-run              Print resolved settings and exit.
  -h, --help             Show this help.

Typical run after downloading the release bundle:
  scripts/test-naked-omc-vetest.sh \
    --target <target-id> \
    --bundle-dir /path/to/kirin-sobel-naked-omc-2026-08-03 \
    --no-clear-logs

If the vetest runner is not installed yet and you have a signed runner HAP:
  scripts/test-naked-omc-vetest.sh \
    --target <target-id> \
    --runner-hap /path/to/entry-default-signed.hap \
    --bundle-dir /path/to/kirin-sobel-naked-omc-2026-08-03 \
    --no-clear-logs
USAGE
}

log() {
  printf '[kirin-naked-omc] %s\n' "$*"
}

warn() {
  printf '[kirin-naked-omc] WARN: %s\n' "$*" >&2
}

die() {
  printf '[kirin-naked-omc] ERROR: %s\n' "$*" >&2
  exit 1
}

file_matches() {
  local file="$1"
  local pattern="$2"

  [ -s "${file}" ] || return 1
  grep -Eiq "${pattern}" "${file}"
}

sha256_file() {
  local file="$1"

  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "${file}"
  elif command -v sha256sum >/dev/null 2>&1; then
    sha256sum "${file}"
  else
    return 1
  fi
}

run_with_timeout() {
  local seconds="$1"
  local output_file="$2"
  shift 2

  "$@" > "${output_file}" 2>&1 &
  local cmd_pid=$!
  local waited=0

  while kill -0 "${cmd_pid}" >/dev/null 2>&1; do
    if [ "${waited}" -ge "${seconds}" ]; then
      kill "${cmd_pid}" >/dev/null 2>&1 || true
      wait "${cmd_pid}" >/dev/null 2>&1 || true
      return 124
    fi
    sleep 1
    waited=$((waited + 1))
  done

  wait "${cmd_pid}"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --bundle-dir)
      [ "$#" -ge 2 ] || die "--bundle-dir requires a directory"
      BUNDLE_DIR="$2"
      shift 2
      ;;
    --omc)
      [ "$#" -ge 2 ] || die "--omc requires a path"
      OMC="$2"
      OMC_EXPLICIT=1
      shift 2
      ;;
    --input)
      [ "$#" -ge 2 ] || die "--input requires a path"
      INPUT="$2"
      INPUT_EXPLICIT=1
      shift 2
      ;;
    --golden)
      [ "$#" -ge 2 ] || die "--golden requires a path"
      GOLDEN="$2"
      GOLDEN_EXPLICIT=1
      shift 2
      ;;
    --target)
      [ "$#" -ge 2 ] || die "--target requires a target id"
      TARGET="$2"
      shift 2
      ;;
    --runner-hap)
      [ "$#" -ge 2 ] || die "--runner-hap requires a path"
      RUNNER_HAP="$2"
      shift 2
      ;;
    --bundle)
      [ "$#" -ge 2 ] || die "--bundle requires a bundle name"
      BUNDLE="$2"
      shift 2
      ;;
    --ability)
      [ "$#" -ge 2 ] || die "--ability requires an ability name"
      ABILITY="$2"
      shift 2
      ;;
    --device-dir)
      [ "$#" -ge 2 ] || die "--device-dir requires a path"
      DEVICE_DIR="$2"
      shift 2
      ;;
    --output-name)
      [ "$#" -ge 2 ] || die "--output-name requires a file name"
      OUTPUT_NAME="$2"
      shift 2
      ;;
    --log-seconds)
      [ "$#" -ge 2 ] || die "--log-seconds requires a number"
      LOG_SECONDS="$2"
      shift 2
      ;;
    --evidence-dir)
      [ "$#" -ge 2 ] || die "--evidence-dir requires a directory"
      EVIDENCE_DIR="$2"
      shift 2
      ;;
    --no-clear-logs)
      HILOG_CLEAR=0
      shift
      ;;
    --hilog-clear-timeout)
      [ "$#" -ge 2 ] || die "--hilog-clear-timeout requires a number"
      HILOG_CLEAR_TIMEOUT="$2"
      shift 2
      ;;
    --skip-runner-check)
      CHECK_RUNNER=0
      shift
      ;;
    --no-pull-output)
      PULL_OUTPUT=0
      shift
      ;;
    --no-compare)
      COMPARE=0
      shift
      ;;
    --no-logs)
      CAPTURE_LOGS=0
      shift
      ;;
    --no-strict)
      STRICT=0
      shift
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown argument: $1"
      ;;
  esac
done

if [ -n "${BUNDLE_DIR}" ]; then
  [ "${OMC_EXPLICIT}" -eq 1 ] || OMC="${BUNDLE_DIR}/SobelCustom.omc"
  [ "${INPUT_EXPLICIT}" -eq 1 ] || INPUT="${BUNDLE_DIR}/x.bin"
  [ "${GOLDEN_EXPLICIT}" -eq 1 ] || GOLDEN="${BUNDLE_DIR}/y.bin"
elif [ -d "${DEFAULT_BUNDLE_DIR}" ]; then
  [ "${OMC_EXPLICIT}" -eq 1 ] || OMC="${DEFAULT_BUNDLE_DIR}/SobelCustom.omc"
  [ "${INPUT_EXPLICIT}" -eq 1 ] || INPUT="${DEFAULT_BUNDLE_DIR}/x.bin"
  [ "${GOLDEN_EXPLICIT}" -eq 1 ] || GOLDEN="${DEFAULT_BUNDLE_DIR}/y.bin"
elif [ -z "${OMC}" ] && [ -f "${PREBUILT_OMC}" ]; then
  OMC="${PREBUILT_OMC}"
fi

case "${LOG_SECONDS}" in
  ''|*[!0-9]*)
    die "--log-seconds must be a positive integer"
    ;;
esac

if [ "${LOG_SECONDS}" -lt 1 ]; then
  die "--log-seconds must be at least 1"
fi

case "${HILOG_CLEAR_TIMEOUT}" in
  ''|*[!0-9]*)
    die "--hilog-clear-timeout must be a positive integer"
    ;;
esac

if [ "${HILOG_CLEAR_TIMEOUT}" -lt 1 ]; then
  die "--hilog-clear-timeout must be at least 1"
fi

if ! command -v hdc >/dev/null 2>&1 && [ -f "${ROOT}/scripts/local-macos-env.sh" ]; then
  # On this Mac workspace, hdc is usually provided by command-line-tools.
  # shellcheck disable=SC1091
  source "${ROOT}/scripts/local-macos-env.sh" >/dev/null 2>&1 || true
fi

HDC_BIN="$(command -v hdc || true)"
[ -n "${HDC_BIN}" ] || die "hdc not found in PATH"

if [ -z "${EVIDENCE_DIR}" ]; then
  STAMP="$(date +%Y%m%d_%H%M%S)"
  EVIDENCE_DIR="${ROOT}/artifacts/naked-omc-runs/${STAMP}"
fi

if [ "${DRY_RUN}" -eq 1 ]; then
  cat <<EOF
HDC_BIN=${HDC_BIN}
BUNDLE_DIR=${BUNDLE_DIR:-<none>}
OMC=${OMC:-<missing>}
INPUT=${INPUT:-<missing>}
GOLDEN=${GOLDEN:-<none>}
TARGET=${TARGET:-<auto>}
RUNNER_HAP=${RUNNER_HAP:-<none>}
BUNDLE=${BUNDLE}
ABILITY=${ABILITY}
DEVICE_DIR=${DEVICE_DIR}
OUTPUT_NAME=${OUTPUT_NAME}
LOG_SECONDS=${LOG_SECONDS}
HILOG_CLEAR_TIMEOUT=${HILOG_CLEAR_TIMEOUT}
EVIDENCE_DIR=${EVIDENCE_DIR}
CAPTURE_LOGS=${CAPTURE_LOGS}
HILOG_CLEAR=${HILOG_CLEAR}
CHECK_RUNNER=${CHECK_RUNNER}
PULL_OUTPUT=${PULL_OUTPUT}
COMPARE=${COMPARE}
STRICT=${STRICT}
LOG_RE=${LOG_RE}
EOF
  exit 0
fi

[ -f "${OMC}" ] || die "OMC file not found: ${OMC:-<missing>}"
[ -f "${INPUT}" ] || die "input file not found: ${INPUT:-<missing>}"
if [ -n "${GOLDEN}" ] && [ ! -f "${GOLDEN}" ]; then
  warn "golden file not found; disabling compare: ${GOLDEN}"
  COMPARE=0
fi
if [ -n "${RUNNER_HAP}" ]; then
  [ -f "${RUNNER_HAP}" ] || die "runner HAP not found: ${RUNNER_HAP}"
fi

mkdir -p "${EVIDENCE_DIR}"

SUMMARY="${EVIDENCE_DIR}/summary.txt"
TARGETS_FILE="${EVIDENCE_DIR}/hdc-targets.txt"
TARGET_INFO="${EVIDENCE_DIR}/target-info.txt"
INSTALL_LOG="${EVIDENCE_DIR}/runner-install.log"
BM_DUMP="${EVIDENCE_DIR}/bm-dump.txt"
MKDIR_LOG="${EVIDENCE_DIR}/device-mkdir.log"
SEND_OMC_LOG="${EVIDENCE_DIR}/send-omc.log"
SEND_INPUT_LOG="${EVIDENCE_DIR}/send-input.log"
START_LOG="${EVIDENCE_DIR}/aa-start.log"
HILOG_CLEAR_LOG="${EVIDENCE_DIR}/hilog-clear.log"
HILOG_RAW="${EVIDENCE_DIR}/hilog.raw.log"
HILOG_FILTERED="${EVIDENCE_DIR}/hilog.filtered.log"
PULL_OUTPUT_LOG="${EVIDENCE_DIR}/pull-output.log"
OUTPUT_LOCAL="${EVIDENCE_DIR}/${OUTPUT_NAME}"
COMPARE_LOG="${EVIDENCE_DIR}/compare.log"

log "evidence dir: ${EVIDENCE_DIR}"
log "omc: ${OMC}"
log "input: ${INPUT}"

{
  sha256_file "${OMC}" || true
  sha256_file "${INPUT}" || true
  if [ -n "${GOLDEN}" ] && [ -f "${GOLDEN}" ]; then
    sha256_file "${GOLDEN}" || true
  fi
} > "${EVIDENCE_DIR}/host-inputs.sha256"

"${HDC_BIN}" list targets | tee "${TARGETS_FILE}" >/dev/null

if [ -z "${TARGET}" ]; then
  TARGET_COUNT=0
  FIRST_TARGET=""
  while IFS= read -r line; do
    [ "${line}" != "[Empty]" ] || continue
    target_word="$(printf '%s\n' "${line}" | awk 'NF {print $1}')"
    [ -n "${target_word}" ] || continue
    TARGET_COUNT=$((TARGET_COUNT + 1))
    if [ -z "${FIRST_TARGET}" ]; then
      FIRST_TARGET="${target_word}"
    fi
  done < "${TARGETS_FILE}"

  if [ "${TARGET_COUNT}" -eq 0 ]; then
    die "no hdc targets found; enable HDC debugging or connect with hdc tconn"
  elif [ "${TARGET_COUNT}" -eq 1 ]; then
    TARGET="${FIRST_TARGET}"
    log "auto-selected hdc target: ${TARGET}"
  else
    cat "${TARGETS_FILE}" >&2
    die "multiple hdc targets found; pass --target <target-id>"
  fi
fi

HDC_TARGET=("${HDC_BIN}" -t "${TARGET}")

{
  echo "target=${TARGET}"
  echo "bundle=${BUNDLE}"
  echo "ability=${ABILITY}"
  echo "device_dir=${DEVICE_DIR}"
  echo
  for key in \
    const.product.model \
    const.product.name \
    const.product.software.version \
    const.ohos.apiversion
  do
    printf '%s=' "${key}"
    "${HDC_TARGET[@]}" shell param get "${key}" 2>&1 || true
  done
  echo
  "${HDC_TARGET[@]}" shell uname -a 2>&1 || true
} | tee "${TARGET_INFO}" >/dev/null

if [ "${CAPTURE_LOGS}" -eq 1 ] && [ "${HILOG_CLEAR}" -eq 1 ]; then
  log "clearing hilog with ${HILOG_CLEAR_TIMEOUT}s timeout"
  set +e
  run_with_timeout "${HILOG_CLEAR_TIMEOUT}" "${HILOG_CLEAR_LOG}" "${HDC_TARGET[@]}" hilog -r
  HILOG_CLEAR_STATUS=$?
  set -e
  if [ "${HILOG_CLEAR_STATUS}" -eq 124 ]; then
    warn "hdc hilog -r timed out after ${HILOG_CLEAR_TIMEOUT}s; continuing without clearing logs"
  elif [ "${HILOG_CLEAR_STATUS}" -ne 0 ]; then
    warn "hdc hilog -r failed with status ${HILOG_CLEAR_STATUS}; continuing without clearing logs"
  fi
fi

if [ -n "${RUNNER_HAP}" ]; then
  log "installing runner HAP: ${RUNNER_HAP}"
  set +e
  "${HDC_TARGET[@]}" install -r "${RUNNER_HAP}" 2>&1 | tee "${INSTALL_LOG}"
  INSTALL_STATUS=${PIPESTATUS[0]}
  set -e
  if [ "${INSTALL_STATUS}" -ne 0 ] || file_matches "${INSTALL_LOG}" 'msg:error|error:|failed to install|install failed|no signature file|Error Code'; then
    die "runner HAP install failed; inspect ${INSTALL_LOG}"
  fi
fi

if [ "${CHECK_RUNNER}" -eq 1 ]; then
  log "checking runner bundle: ${BUNDLE}"
  set +e
  "${HDC_TARGET[@]}" shell bm dump -a > "${BM_DUMP}" 2>&1
  BM_STATUS=$?
  set -e
  if [ "${BM_STATUS}" -ne 0 ]; then
    die "bm dump failed; inspect ${BM_DUMP}"
  fi
  if ! grep -q "${BUNDLE}" "${BM_DUMP}"; then
    die "runner bundle not found: ${BUNDLE}. Install the native vetest runner HAP first or pass --runner-hap."
  fi
fi

log "ensuring device directory exists: ${DEVICE_DIR}"
set +e
"${HDC_TARGET[@]}" shell mkdir -p "${DEVICE_DIR}" > "${MKDIR_LOG}" 2>&1
MKDIR_STATUS=$?
set -e
if [ "${MKDIR_STATUS}" -ne 0 ]; then
  die "failed to create device directory; inspect ${MKDIR_LOG}"
fi

OMC_BASENAME="$(basename "${OMC}")"
INPUT_BASENAME="$(basename "${INPUT}")"

log "sending OMC to device"
set +e
"${HDC_TARGET[@]}" file send "${OMC}" "${DEVICE_DIR}/${OMC_BASENAME}" 2>&1 | tee "${SEND_OMC_LOG}"
SEND_OMC_STATUS=${PIPESTATUS[0]}
set -e
if [ "${SEND_OMC_STATUS}" -ne 0 ] || file_matches "${SEND_OMC_LOG}" 'error:|failed|Error Code'; then
  die "failed to send OMC; inspect ${SEND_OMC_LOG}"
fi

log "sending input to device"
set +e
"${HDC_TARGET[@]}" file send "${INPUT}" "${DEVICE_DIR}/${INPUT_BASENAME}" 2>&1 | tee "${SEND_INPUT_LOG}"
SEND_INPUT_STATUS=${PIPESTATUS[0]}
set -e
if [ "${SEND_INPUT_STATUS}" -ne 0 ] || file_matches "${SEND_INPUT_LOG}" 'error:|failed|Error Code'; then
  die "failed to send input; inspect ${SEND_INPUT_LOG}"
fi

LOG_PID=""
cleanup() {
  if [ -n "${LOG_PID}" ] && kill -0 "${LOG_PID}" >/dev/null 2>&1; then
    kill "${LOG_PID}" >/dev/null 2>&1 || true
    wait "${LOG_PID}" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

if [ "${CAPTURE_LOGS}" -eq 1 ]; then
  log "capturing hilog for ${LOG_SECONDS}s into ${HILOG_RAW}"
  ("${HDC_TARGET[@]}" hilog > "${HILOG_RAW}" 2>&1) &
  LOG_PID=$!
fi

log "starting runner"
set +e
"${HDC_TARGET[@]}" shell aa start \
  -a "${ABILITY}" \
  -b "${BUNDLE}" \
  --ps path "${INPUT_BASENAME}" \
  --ps omPath "${OMC_BASENAME}" 2>&1 | tee "${START_LOG}"
START_STATUS=${PIPESTATUS[0]}
set -e
if [ "${START_STATUS}" -ne 0 ] || file_matches "${START_LOG}" 'error:|failed to start ability|Error Code|does not exist|not installed'; then
  if [ "${STRICT}" -eq 1 ]; then
    die "aa start failed; inspect ${START_LOG}"
  fi
  warn "aa start did not look successful; continuing because --no-strict is set"
fi

if [ "${CAPTURE_LOGS}" -eq 1 ]; then
  log "waiting ${LOG_SECONDS}s for runner logs"
  sleep "${LOG_SECONDS}"
  cleanup
  grep -E "${LOG_RE}" "${HILOG_RAW}" > "${HILOG_FILTERED}" || true
fi

OUTPUT_PULLED=0
if [ "${PULL_OUTPUT}" -eq 1 ]; then
  log "pulling output from device: ${OUTPUT_NAME}"
  set +e
  "${HDC_TARGET[@]}" file recv "${DEVICE_DIR}/${OUTPUT_NAME}" "${OUTPUT_LOCAL}" 2>&1 | tee "${PULL_OUTPUT_LOG}"
  PULL_STATUS=${PIPESTATUS[0]}
  set -e
  if [ "${PULL_STATUS}" -eq 0 ] && [ -s "${OUTPUT_LOCAL}" ] && ! file_matches "${PULL_OUTPUT_LOG}" 'error:|failed|Error Code|No such file'; then
    OUTPUT_PULLED=1
    sha256_file "${OUTPUT_LOCAL}" > "${EVIDENCE_DIR}/output.sha256" || true
  else
    warn "output was not pulled successfully; inspect ${PULL_OUTPUT_LOG}"
  fi
fi

COMPARE_RESULT="SKIPPED"
if [ "${COMPARE}" -eq 1 ] && [ "${OUTPUT_PULLED}" -eq 1 ] && [ -n "${GOLDEN}" ] && [ -f "${GOLDEN}" ]; then
  if cmp -s "${OUTPUT_LOCAL}" "${GOLDEN}"; then
    COMPARE_RESULT="PASS"
    printf 'PASS: %s matches %s\n' "${OUTPUT_LOCAL}" "${GOLDEN}" > "${COMPARE_LOG}"
    log "golden compare passed"
  else
    COMPARE_RESULT="FAIL"
    {
      printf 'FAIL: %s does not match %s\n' "${OUTPUT_LOCAL}" "${GOLDEN}"
      wc -c "${OUTPUT_LOCAL}" "${GOLDEN}" || true
      sha256_file "${OUTPUT_LOCAL}" || true
      sha256_file "${GOLDEN}" || true
    } > "${COMPARE_LOG}"
    warn "golden compare failed; inspect ${COMPARE_LOG}"
  fi
fi

RESULT="NOT_CONFIRMED"
if [ "${COMPARE_RESULT}" = "PASS" ]; then
  RESULT="PASS_CANDIDATE"
elif [ "${OUTPUT_PULLED}" -eq 1 ]; then
  RESULT="OUTPUT_PULLED_NO_COMPARE"
fi

{
  echo "timestamp=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  echo "target=${TARGET}"
  echo "bundle=${BUNDLE}"
  echo "ability=${ABILITY}"
  echo "device_dir=${DEVICE_DIR}"
  echo "omc=${OMC}"
  echo "input=${INPUT}"
  echo "golden=${GOLDEN:-}"
  echo "output_name=${OUTPUT_NAME}"
  echo "output_local=${OUTPUT_LOCAL}"
  echo "output_pulled=${OUTPUT_PULLED}"
  echo "compare=${COMPARE}"
  echo "compare_result=${COMPARE_RESULT}"
  echo "capture_logs=${CAPTURE_LOGS}"
  echo "strict=${STRICT}"
  echo "result=${RESULT}"
  echo
  echo "host_input_hashes:"
  cat "${EVIDENCE_DIR}/host-inputs.sha256"
  echo
  echo "important_files:"
  echo "- ${TARGET_INFO}"
  echo "- ${START_LOG}"
  echo "- ${PULL_OUTPUT_LOG}"
  echo "- ${COMPARE_LOG}"
  echo "- ${HILOG_FILTERED}"
} > "${SUMMARY}"

log "summary: ${SUMMARY}"
if [ "${CAPTURE_LOGS}" -eq 1 ]; then
  log "filtered hilog: ${HILOG_FILTERED}"
fi

if [ "${RESULT}" = "PASS_CANDIDATE" ]; then
  log "PASS_CANDIDATE: runner produced output0.bin and it matches the golden file."
  exit 0
fi

if [ "${STRICT}" -eq 1 ]; then
  die "naked OMC run was not confirmed"
fi

warn "naked OMC run was not confirmed"
exit 0
