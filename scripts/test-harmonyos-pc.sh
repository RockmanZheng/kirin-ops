#!/usr/bin/env bash
# Install and validate the Sobel CANN/HarmonyOS HAP on a HarmonyOS PC/device via hdc.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_HAP="${ROOT}/artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo/entry/build/default/outputs/default/entry-default-unsigned.hap"

HAP="${HAP:-${DEFAULT_HAP}}"
TARGET="${TARGET:-}"
BUNDLE="${BUNDLE:-com.example.hdc_sobel_demo}"
ABILITY="${ABILITY:-EntryAbility}"
LOG_SECONDS="${LOG_SECONDS:-90}"
HILOG_CLEAR_TIMEOUT="${HILOG_CLEAR_TIMEOUT:-5}"
EVIDENCE_DIR="${EVIDENCE_DIR:-}"
INSTALL=1
START_APP=1
CAPTURE_LOGS=1
HILOG_CLEAR=1
STRICT=1
UNINSTALL_FIRST=0
BUGREPORT=0
DRY_RUN=0
LOG_RE="${LOG_RE:-HiAIFoundationDemo|CANNKitDemo|LoadModelFromBuffer|InitIOTensors|OH_NNExecutor_RunSync|GetResult|HIAI|NN|compibility|compatibility|failed|ERROR}"

usage() {
  cat <<'USAGE'
usage: scripts/test-harmonyos-pc.sh [options]

Installs a Sobel CANN/HarmonyOS HAP through hdc, starts the app, captures hilog,
and checks for NPU inference success markers.

Options:
  --hap PATH             HAP to install. Defaults to the local prebuilt unsigned HAP.
  --target TARGET        hdc target id. Auto-detects when exactly one target is connected.
  --bundle NAME          Bundle name. Default: com.example.hdc_sobel_demo
  --ability NAME         Ability name. Default: EntryAbility
  --log-seconds N        Log capture window after app start. Default: 90
  --evidence-dir DIR     Directory for logs and command output. Default: artifacts/real-device-runs/<timestamp>
  --uninstall-first      Uninstall the bundle before install.
  --no-install           Do not install the HAP.
  --no-start             Do not start the app.
  --no-logs              Do not capture hilog.
  --no-clear-logs        Do not run "hdc hilog -r" before capture.
  --hilog-clear-timeout N
                         Seconds to wait for "hdc hilog -r". Default: 5
  --no-strict            Do not exit nonzero when success log markers are missing.
  --bugreport            Save hdc bugreport after the run.
  --dry-run              Print resolved settings and exit.
  -h, --help             Show this help.

Environment overrides:
  HAP, TARGET, BUNDLE, ABILITY, LOG_SECONDS, HILOG_CLEAR_TIMEOUT,
  EVIDENCE_DIR, LOG_RE

Typical run:
  scripts/test-harmonyos-pc.sh --hap /path/to/entry-default-signed.hap

If multiple hdc targets are connected:
  scripts/test-harmonyos-pc.sh --target <target-id> --hap /path/to/entry-default-signed.hap
USAGE
}

log() {
  printf '[kirin-test] %s\n' "$*"
}

die() {
  printf '[kirin-test] ERROR: %s\n' "$*" >&2
  exit 1
}

warn() {
  printf '[kirin-test] WARN: %s\n' "$*" >&2
}

run_cmd() {
  log "+ $*"
  "$@"
}

file_matches() {
  local file="$1"
  local pattern="$2"

  [ -s "${file}" ] || return 1
  grep -Eiq "${pattern}" "${file}"
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
    --hap)
      [ "$#" -ge 2 ] || die "--hap requires a path"
      HAP="$2"
      shift 2
      ;;
    --target)
      [ "$#" -ge 2 ] || die "--target requires a target id"
      TARGET="$2"
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
    --uninstall-first)
      UNINSTALL_FIRST=1
      shift
      ;;
    --no-install)
      INSTALL=0
      shift
      ;;
    --no-start)
      START_APP=0
      shift
      ;;
    --no-logs)
      CAPTURE_LOGS=0
      shift
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
    --no-strict)
      STRICT=0
      shift
      ;;
    --bugreport)
      BUGREPORT=1
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
command -v unzip >/dev/null 2>&1 || die "unzip not found in PATH"

if [ -z "${EVIDENCE_DIR}" ]; then
  STAMP="$(date +%Y%m%d_%H%M%S)"
  EVIDENCE_DIR="${ROOT}/artifacts/real-device-runs/${STAMP}"
fi

if [ "${DRY_RUN}" -eq 1 ]; then
  cat <<EOF
HDC_BIN=${HDC_BIN}
HAP=${HAP}
TARGET=${TARGET:-<auto>}
BUNDLE=${BUNDLE}
ABILITY=${ABILITY}
LOG_SECONDS=${LOG_SECONDS}
HILOG_CLEAR_TIMEOUT=${HILOG_CLEAR_TIMEOUT}
EVIDENCE_DIR=${EVIDENCE_DIR}
INSTALL=${INSTALL}
START_APP=${START_APP}
CAPTURE_LOGS=${CAPTURE_LOGS}
HILOG_CLEAR=${HILOG_CLEAR}
STRICT=${STRICT}
UNINSTALL_FIRST=${UNINSTALL_FIRST}
BUGREPORT=${BUGREPORT}
LOG_RE=${LOG_RE}
EOF
  exit 0
fi

[ -f "${HAP}" ] || die "HAP not found: ${HAP}"
mkdir -p "${EVIDENCE_DIR}"

SUMMARY="${EVIDENCE_DIR}/summary.txt"
HAP_LIST="${EVIDENCE_DIR}/hap-contents.txt"
TARGETS_FILE="${EVIDENCE_DIR}/hdc-targets.txt"
INSTALL_LOG="${EVIDENCE_DIR}/install.log"
START_LOG="${EVIDENCE_DIR}/aa-start.log"
TARGET_INFO="${EVIDENCE_DIR}/target-info.txt"
HILOG_RAW="${EVIDENCE_DIR}/hilog.raw.log"
HILOG_FILTERED="${EVIDENCE_DIR}/hilog.filtered.log"
HILOG_CLEAR_LOG="${EVIDENCE_DIR}/hilog-clear.log"
BM_DUMP="${EVIDENCE_DIR}/bm-dump.txt"
BUGREPORT_FILE="${EVIDENCE_DIR}/bugreport.txt"

log "evidence dir: ${EVIDENCE_DIR}"
log "hap: ${HAP}"

if command -v shasum >/dev/null 2>&1; then
  shasum -a 256 "${HAP}" | tee "${EVIDENCE_DIR}/hap.sha256" >/dev/null
elif command -v sha256sum >/dev/null 2>&1; then
  sha256sum "${HAP}" | tee "${EVIDENCE_DIR}/hap.sha256" >/dev/null
else
  warn "no SHA256 tool found; skipping HAP hash"
fi

unzip -l "${HAP}" > "${HAP_LIST}"

grep -q 'resources/rawfile/SobelCustom\.omc' "${HAP_LIST}" || die "HAP is missing resources/rawfile/SobelCustom.omc"
grep -q 'resources/rawfile/SobelCustom\.om' "${HAP_LIST}" || warn "HAP is missing resources/rawfile/SobelCustom.om"
grep -q 'libs/arm64-v8a/libentry\.so' "${HAP_LIST}" || die "HAP is missing libs/arm64-v8a/libentry.so"

log "HAP content check passed"

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
elif [ "${CAPTURE_LOGS}" -eq 1 ]; then
  log "skipping hilog clear"
fi

if [ "${UNINSTALL_FIRST}" -eq 1 ]; then
  run_cmd "${HDC_TARGET[@]}" uninstall "${BUNDLE}" | tee "${EVIDENCE_DIR}/uninstall.log" || true
fi

if [ "${INSTALL}" -eq 1 ]; then
  set +e
  "${HDC_TARGET[@]}" install -r "${HAP}" 2>&1 | tee "${INSTALL_LOG}"
  INSTALL_STATUS=${PIPESTATUS[0]}
  set -e
  INSTALL_FAILED=0
  if [ "${INSTALL_STATUS}" -ne 0 ]; then
    warn "hdc install exited with status ${INSTALL_STATUS}"
    INSTALL_FAILED=1
  elif file_matches "${INSTALL_LOG}" 'msg:error|error:|failed to install|install failed|no signature file|Error Code'; then
    warn "hdc install output contains failure text even though hdc returned 0"
    INSTALL_FAILED=1
  fi
  if [ "${INSTALL_FAILED}" -eq 1 ]; then
    warn "install failed. If this is an unsigned HAP, regenerate a DevEco debug/signed HAP."
    if [ "${INSTALL_STATUS}" -ne 0 ]; then
      exit "${INSTALL_STATUS}"
    fi
    exit 1
  fi
else
  log "skipping install"
fi

set +e
"${HDC_TARGET[@]}" shell bm dump -a > "${BM_DUMP}" 2>&1
BM_STATUS=$?
set -e
if [ "${BM_STATUS}" -eq 0 ]; then
  if grep -q "${BUNDLE}" "${BM_DUMP}"; then
    log "bundle appears in bm dump: ${BUNDLE}"
  else
    warn "bundle not found in bm dump output: ${BUNDLE}"
  fi
else
  warn "bm dump failed; continuing"
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

if [ "${START_APP}" -eq 1 ]; then
  set +e
  "${HDC_TARGET[@]}" shell aa start -a "${ABILITY}" -b "${BUNDLE}" 2>&1 | tee "${START_LOG}"
  START_STATUS=${PIPESTATUS[0]}
  set -e
  START_FAILED=0
  if [ "${START_STATUS}" -ne 0 ]; then
    warn "aa start exited with status ${START_STATUS}"
    START_FAILED=1
  elif file_matches "${START_LOG}" 'error:|failed to start ability|Error Code|does not exist|not installed'; then
    warn "aa start output contains failure text even though hdc returned 0"
    START_FAILED=1
  fi
  if [ "${START_FAILED}" -eq 1 ]; then
    warn "aa start failed; try launching the app manually on the HarmonyOS PC/device"
    if [ "${STRICT}" -eq 1 ]; then
      if [ "${START_STATUS}" -ne 0 ]; then
        exit "${START_STATUS}"
      fi
      exit 1
    fi
  fi
else
  log "skipping app start"
fi

if [ "${CAPTURE_LOGS}" -eq 1 ]; then
  log "tap NPU推理 in the app now; waiting ${LOG_SECONDS}s for runtime logs"
  sleep "${LOG_SECONDS}"
  cleanup
  grep -E "${LOG_RE}" "${HILOG_RAW}" > "${HILOG_FILTERED}" || true
else
  log "skipping log capture"
fi

if [ "${BUGREPORT}" -eq 1 ]; then
  log "saving bugreport to ${BUGREPORT_FILE}"
  "${HDC_TARGET[@]}" bugreport "${BUGREPORT_FILE}" >/dev/null 2>&1 || warn "bugreport failed"
fi

PASS=1
if [ "${CAPTURE_LOGS}" -eq 1 ]; then
  for marker in \
    'LoadModelFromBuffer success' \
    'InitIOTensors success' \
    'OH_NNExecutor_RunSync success' \
    'GetResult success'
  do
    if grep -q "${marker}" "${HILOG_RAW}"; then
      log "found marker: ${marker}"
    else
      warn "missing marker: ${marker}"
      PASS=0
    fi
  done
else
  PASS=0
fi

{
  echo "timestamp=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  echo "target=${TARGET}"
  echo "hap=${HAP}"
  echo "bundle=${BUNDLE}"
  echo "ability=${ABILITY}"
  echo "evidence_dir=${EVIDENCE_DIR}"
  echo "install=${INSTALL}"
  echo "start_app=${START_APP}"
  echo "capture_logs=${CAPTURE_LOGS}"
  echo "hilog_clear=${HILOG_CLEAR}"
  echo "hilog_clear_timeout=${HILOG_CLEAR_TIMEOUT}"
  echo "strict=${STRICT}"
  echo
  echo "required_log_markers:"
  echo "- LoadModelFromBuffer success"
  echo "- InitIOTensors success"
  echo "- OH_NNExecutor_RunSync success"
  echo "- GetResult success"
  echo
  if [ "${PASS}" -eq 1 ]; then
    echo "result=PASS_CANDIDATE"
    echo "note=Confirm the UI also showed NPU运行时间 and a processed Sobel image."
  else
    echo "result=NOT_CONFIRMED"
    echo "note=Inspect hilog and UI evidence before claiming real-device success."
  fi
} > "${SUMMARY}"

log "summary: ${SUMMARY}"
log "filtered hilog: ${HILOG_FILTERED}"

if [ "${PASS}" -eq 1 ]; then
  log "PASS_CANDIDATE: log markers were found. Confirm UI evidence manually."
  exit 0
fi

if [ "${STRICT}" -eq 1 ]; then
  die "real-device success was not confirmed from logs"
fi

warn "real-device success was not confirmed from logs"
exit 0
