#!/usr/bin/env bash
# Run a naked CANN .omc model on a HarmonyOS target with model_run_tool.
#
# This follows the real physical-machine history captured in issue #1:
#   hdc file send <model.omc> /data/local/tmp/<model.omc>
#   hdc file send <input.bin> /data/local/tmp/<input.bin>
#   hdc shell "/data/local/tmp/model_run_tool --model=... --input=... --output_dir=/data/local/tmp/"
#   hdc file recv /data/local/tmp/output_0 ./output.bin

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_BUNDLE_DIR="${ROOT}/artifacts/naked-omc/kirin-sobel-naked-omc-2026-08-03"
PREBUILT_OMC="${ROOT}/artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo/entry/src/main/resources/rawfile/SobelCustom.omc"

BUNDLE_DIR="${BUNDLE_DIR:-}"
OMC="${OMC:-}"
INPUT="${INPUT:-}"
GOLDEN="${GOLDEN:-}"
TARGET="${TARGET:-}"
TARGET_SOC="${TARGET_SOC:-}"
MODEL_RUN_TOOL="${MODEL_RUN_TOOL:-/data/local/tmp/model_run_tool}"
DEVICE_DIR="${DEVICE_DIR:-/data/local/tmp}"
OUTPUT_NAME="${OUTPUT_NAME:-output_0}"
LOG_SECONDS="${LOG_SECONDS:-30}"
HILOG_CLEAR_TIMEOUT="${HILOG_CLEAR_TIMEOUT:-5}"
DIAG_TIMEOUT="${DIAG_TIMEOUT:-15}"
EVIDENCE_DIR="${EVIDENCE_DIR:-}"
EVIDENCE_ARCHIVE="${EVIDENCE_ARCHIVE:-}"
EVIDENCE_TEXT="${EVIDENCE_TEXT:-}"
CAPTURE_LOGS=1
HILOG_CLEAR=1
CHECK_TOOL=1
CHECK_SOC=1
COLLECT_DIAGS=1
EXPORT_LOGS=1
EXPORT_TEXT=1
RAW_HILOG_MODE="${RAW_HILOG_MODE:-auto}"
PULL_OUTPUT=1
COMPARE=1
STRICT=1
DRY_RUN=0
EVIDENCE_INITIALIZED=0
EXPORT_DONE=0
EXPORT_IN_PROGRESS=0
EVIDENCE_ARCHIVE_EXPLICIT=0
EVIDENCE_TEXT_EXPLICIT=0
OMC_EXPLICIT=0
INPUT_EXPLICIT=0
GOLDEN_EXPLICIT=0
LOG_RE="${LOG_RE:-model_run_tool|output_0|CANN|HIAI|NN|OH_NN|success|failed|ERROR|error|RunModel|LoadModel|InitIOTensors|compat|compil|Build|Executor|GetDeviceID|SetDevice|offline}"
RUNNER_LAUNCH_ERROR_RE="${RUNNER_LAUNCH_ERROR_RE:-inaccessible or not found|no such file|not found|permission denied|exec format error|cannot execute binary file|cannot link executable|bad elf|invalid elf|library .*not found|linker .*not found}"

usage() {
  cat <<'USAGE'
usage: scripts/test-naked-omc-vetest.sh [options]

Runs a naked .omc file on a HarmonyOS target using a native CLI runner:

  /data/local/tmp/model_run_tool \
    --model=/data/local/tmp/<model.omc> \
    --input=/data/local/tmp/<input.bin> \
    --output_dir=/data/local/tmp/

Options:
  --bundle-dir DIR       Directory containing SobelCustom.omc, x.bin, and optional y.bin.
  --omc PATH             Local OMC model file. Overrides --bundle-dir default.
  --input PATHS          Local input .bin path, or comma-separated local input paths.
                         Multi-input example: x1.bin,x2.bin
  --golden PATH          Expected output .bin file. Overrides --bundle-dir default.
  --target TARGET        hdc target id. Auto-detects when exactly one target is connected.
  --target-soc SOC       Expected target SoC, e.g. kirin9020, KirinX90, Kirin9030.
                         Use this when the device does not expose SoC via param get.
  --model-run-tool PATH  Remote runner path. Default: /data/local/tmp/model_run_tool
  --device-dir PATH      Remote work dir. Default: /data/local/tmp
  --output-name NAME     Remote output file name to pull. Default: output_0
  --log-seconds N        Log capture window after runner start. Default: 30
  --evidence-dir DIR     Evidence directory. Default: artifacts/naked-omc-runs/<timestamp>
  --evidence-archive PATH
                         Evidence archive path. Default: <evidence-dir>.tgz
  --evidence-text PATH   Copy/paste text report path. Default: <evidence-dir>/evidence-report.txt
  --no-clear-logs        Do not run "hdc shell hilog -r" before capture.
  --hilog-clear-timeout N
                         Seconds to wait for "hdc shell hilog -r". Default: 5
  --diag-timeout N       Seconds to wait for each diagnostic probe. Default: 15
  --skip-tool-check      Do not check model_run_tool before running.
  --skip-soc-check       Do not preflight-check .omc SoC metadata against the target.
                         Use only when intentionally running despite unknown/mismatched SoC.
  --no-diagnostics       Do not collect extra target/runner/file diagnostics.
  --no-export-logs       Do not create evidence-files.txt, the .tgz archive, or the text report.
  --no-export-text       Do not create the copy/paste evidence-report.txt.
  --include-raw-hilog    Always include hilog.raw.log in the evidence archive.
  --no-raw-hilog         Never include hilog.raw.log in the evidence archive.
                         Default: include raw hilog only when filtered hilog is empty.
  --no-pull-output       Do not pull output from the device.
  --no-compare           Do not compare pulled output with the golden file.
  --no-logs              Do not capture hilog.
  --no-strict            Do not exit nonzero when output/log validation is incomplete.
  --dry-run              Print resolved settings and exit.
  -h, --help             Show this help.

Typical Sobel run on the HarmonyOS PC from issue #1:
  scripts/test-naked-omc-vetest.sh \
    --target SH236HS0488 \
    --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
    --no-clear-logs

Multi-input example, matching the observed model_run_tool convention:
  scripts/test-naked-omc-vetest.sh \
    --target SH236HS0488 \
    --omc /path/to/add_1.omc \
    --input /path/to/add_x1.bin,/path/to/add_x2.bin \
    --no-compare
USAGE
}

log() {
  printf '[kirin-naked-omc] %s\n' "$*"
}

warn() {
  printf '[kirin-naked-omc] WARN: %s\n' "$*" >&2
}

die() {
  local message="$*"

  printf '[kirin-naked-omc] ERROR: %s\n' "${message}" >&2
  if [ "${EVIDENCE_INITIALIZED:-0}" -eq 1 ] && [ "${EXPORT_IN_PROGRESS:-0}" -eq 0 ]; then
    write_failure_summary_if_missing "${message}" || true
    export_evidence_bundle || true
  fi
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

normalize_soc_token() {
  local token="$1"
  local lower

  lower="$(printf '%s' "${token}" | tr '[:upper:]' '[:lower:]')"
  case "${lower}" in
    kirinx[0-9]*)
      printf '%s\n' "${lower}" | sed -E 's/^(kirinx[0-9]+).*/\1/'
      ;;
    kirin[0-9]*)
      printf '%s\n' "${lower}" | sed -E 's/^(kirin[0-9]+).*/\1/'
      ;;
    *)
      return 1
      ;;
  esac
}

extract_soc_versions_from_text() {
  awk '
    {
      line = tolower($0)
      scan = line
      while (match(scan, /kirin[ _-]*x?[ _-]*[0-9][0-9]*/)) {
        token = substr(scan, RSTART, RLENGTH)
        gsub(/[ _-]/, "", token)
        print token
        scan = substr(scan, RSTART + RLENGTH)
      }
      print line
    }
  ' | tr -cs '[:alnum:]_+-' '\n' | while IFS= read -r token; do
    normalize_soc_token "${token}" || true
  done | awk 'NF && !seen[$0]++'
}

extract_model_soc_versions() {
  local file="$1"

  if command -v strings >/dev/null 2>&1; then
    strings -a "${file}" | extract_soc_versions_from_text
  fi
}

format_soc_set() {
  local values="$1"

  if [ -z "${values}" ]; then
    printf '<unknown>\n'
    return
  fi

  printf '%s\n' "${values}" | awk 'NF { if (out) { out = out "," $0 } else { out = $0 } } END { print out }'
}

soc_sets_intersect() {
  local left="$1"
  local right="$2"
  local token

  while IFS= read -r token; do
    [ -n "${token}" ] || continue
    if printf '%s\n' "${right}" | grep -Fqx "${token}"; then
      return 0
    fi
  done <<EOF
${left}
EOF

  return 1
}

write_model_metadata() {
  local file="$1"

  {
    echo "file=${file}"
    printf 'size_bytes='
    wc -c < "${file}" | tr -d '[:space:]'
    echo
    echo
    echo "normalized_soc_versions:"
    extract_model_soc_versions "${file}" || true
    echo
    echo "raw_strings:"
    if command -v strings >/dev/null 2>&1; then
      strings -a "${file}" | awk '
        /soc_version|kirin[0-9]+|Kirin[0-9]+|Bisheng-Compiler|custom_ascendc_lib|SobelCustom|hiai_version/ {
          print
          count++
          if (count >= 120) {
            exit
          }
        }
      '
    else
      echo "strings unavailable"
    fi
  }
}

remote_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
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

collect_remote_diag() {
  local label="$1"
  local output_file="$2"
  local remote_cmd="$3"
  local status

  [ "${COLLECT_DIAGS}" -eq 1 ] || return 0

  log "collecting ${label}: ${output_file}"
  set +e
  run_with_timeout "${DIAG_TIMEOUT}" "${output_file}" "${HDC_TARGET[@]}" shell "${remote_cmd}"
  status=$?
  set -e

  if [ "${status}" -eq 124 ]; then
    warn "${label} timed out after ${DIAG_TIMEOUT}s; partial output may be in ${output_file}"
  elif [ "${status}" -ne 0 ]; then
    warn "${label} exited with status ${status}; inspect ${output_file}"
  fi
}

append_evidence_file() {
  local path="$1"

  [ -n "${path}" ] || return 0
  [ -e "${path}" ] || return 0
  case "${path}" in
    "${EVIDENCE_DIR}/"*)
      printf '%s\n' "${path#"${EVIDENCE_DIR}/"}" >> "${EVIDENCE_MANIFEST}"
      ;;
    *)
      warn "skipping evidence file outside evidence dir: ${path}"
      ;;
  esac
}

write_failure_summary_if_missing() {
  local failure_message="$1"

  [ -n "${SUMMARY:-}" ] || return 0
  [ ! -s "${SUMMARY}" ] || return 0

  {
    echo "timestamp=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    echo "target=${TARGET:-}"
    echo "model_run_tool=${MODEL_RUN_TOOL:-}"
    echo "device_dir=${DEVICE_DIR:-}"
    echo "omc=${OMC:-}"
    echo "input=${INPUT:-}"
    echo "golden=${GOLDEN:-}"
    echo "check_soc=${CHECK_SOC:-}"
    echo "model_soc_versions=$(format_soc_set "${MODEL_SOC_VERSIONS:-}")"
    echo "target_soc_versions=$(format_soc_set "${TARGET_SOC_VERSIONS:-}")"
    echo "soc_check_result=${SOC_CHECK_RESULT:-NOT_REACHED}"
    echo "output_name=${OUTPUT_NAME:-}"
    echo "output_remote=${OUTPUT_REMOTE:-}"
    echo "output_local=${OUTPUT_LOCAL:-}"
    echo "output_pulled=${OUTPUT_PULLED:-0}"
    echo "compare=${COMPARE:-}"
    echo "compare_result=${COMPARE_RESULT:-NOT_REACHED}"
    echo "run_failed=${RUN_FAILED:-1}"
    echo "run_failure_reason=${RUN_FAILURE_REASON:-${failure_message}}"
    echo "runner_launch_failed=${RUNNER_LAUNCH_FAILED:-0}"
    echo "model_load_failed=${MODEL_LOAD_FAILED:-0}"
    echo "capture_logs=${CAPTURE_LOGS:-}"
    echo "collect_diagnostics=${COLLECT_DIAGS:-}"
    echo "export_logs=${EXPORT_LOGS:-}"
    echo "raw_hilog_mode=${RAW_HILOG_MODE:-}"
    echo "evidence_manifest=${EVIDENCE_MANIFEST:-}"
    echo "evidence_archive=${EVIDENCE_ARCHIVE:-}"
    echo "evidence_text=${EVIDENCE_TEXT:-}"
    echo "diag_timeout=${DIAG_TIMEOUT:-}"
    echo "strict=${STRICT:-}"
    echo "result=EARLY_FAILURE"
    echo "failure=${failure_message}"
    if [ -s "${EVIDENCE_DIR}/host-inputs.sha256" ]; then
      echo
      echo "host_input_hashes:"
      cat "${EVIDENCE_DIR}/host-inputs.sha256"
    fi
  } > "${SUMMARY}"
}

append_evidence_text_file() {
  local path="$1"
  local rel
  local size

  [ "${EXPORT_TEXT:-0}" -eq 1 ] || return 0
  [ -n "${path}" ] || return 0
  [ -e "${path}" ] || return 0

  case "${path}" in
    "${EVIDENCE_DIR}/"*)
      rel="${path#"${EVIDENCE_DIR}/"}"
      ;;
    *)
      rel="${path}"
      ;;
  esac

  size="$(wc -c < "${path}" | tr -d '[:space:]')"
  {
    echo
    echo "================================================================================"
    echo "FILE: ${rel}"
    echo "PATH: ${path}"
    echo "SIZE_BYTES: ${size}"
    echo "================================================================================"
    if [ -s "${path}" ]; then
      sed -e 's/\r$//' "${path}" || true
    else
      echo "<empty>"
    fi
  } >> "${EVIDENCE_TEXT}"
}

write_evidence_text_report() {
  local text_dir

  [ "${EXPORT_TEXT:-0}" -eq 1 ] || return 0
  [ -n "${EVIDENCE_TEXT:-}" ] || return 0

  text_dir="$(dirname "${EVIDENCE_TEXT}")"
  mkdir -p "${text_dir}"

  {
    echo "KIRIN_NAKED_OMC_EVIDENCE_REPORT"
    echo "generated_utc=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
    echo "evidence_dir=${EVIDENCE_DIR}"
    echo "evidence_archive=${EVIDENCE_ARCHIVE}"
    echo "raw_hilog_mode=${RAW_HILOG_MODE}"
    echo
    echo "Copy/paste this whole file into the GitHub issue when artifact upload is unavailable."
    echo "Binary outputs are not inlined; use hashes and file listings below for binary evidence."
  } > "${EVIDENCE_TEXT}"

  append_evidence_text_file "${SUMMARY:-}"
  append_evidence_text_file "${HDC_INFO:-}"
  append_evidence_text_file "${TARGETS_FILE:-}"
  append_evidence_text_file "${TARGET_INFO:-}"
  append_evidence_text_file "${SOC_CHECK_LOG:-}"
  append_evidence_text_file "${TARGET_DIAG_LOG:-}"
  append_evidence_text_file "${RUNNER_DIAG_LOG:-}"
  append_evidence_text_file "${MODEL_INFO:-}"
  append_evidence_text_file "${TOOL_CHECK_LOG:-}"
  append_evidence_text_file "${MKDIR_LOG:-}"
  append_evidence_text_file "${REMOTE_CLEAN_LOG:-}"
  append_evidence_text_file "${SEND_OMC_LOG:-}"
  append_evidence_text_file "${SEND_INPUT_LOG:-}"
  append_evidence_text_file "${RUN_LOG:-}"
  append_evidence_text_file "${REMOTE_FILES_BEFORE_LOG:-}"
  append_evidence_text_file "${REMOTE_FILES_AFTER_LOG:-}"
  append_evidence_text_file "${REMOTE_LIST_LOG:-}"
  append_evidence_text_file "${PULL_OUTPUT_LOG:-}"
  append_evidence_text_file "${COMPARE_LOG:-}"
  append_evidence_text_file "${HILOG_CLEAR_LOG:-}"
  append_evidence_text_file "${HILOG_FILTERED:-}"
  append_evidence_text_file "${EVIDENCE_DIR}/host-inputs.sha256"
  append_evidence_text_file "${EVIDENCE_DIR}/output.sha256"

  if [ "${RAW_HILOG_MODE}" = "always" ] || { [ "${RAW_HILOG_MODE}" = "auto" ] && [ -f "${HILOG_RAW:-}" ] && [ ! -s "${HILOG_FILTERED:-}" ]; }; then
    append_evidence_text_file "${HILOG_RAW:-}"
  fi
}

write_evidence_manifest() {
  : > "${EVIDENCE_MANIFEST}"

  append_evidence_file "${SUMMARY:-}"
  append_evidence_file "${HDC_INFO:-}"
  append_evidence_file "${TARGETS_FILE:-}"
  append_evidence_file "${TARGET_INFO:-}"
  append_evidence_file "${SOC_CHECK_LOG:-}"
  append_evidence_file "${TARGET_DIAG_LOG:-}"
  append_evidence_file "${RUNNER_DIAG_LOG:-}"
  append_evidence_file "${MODEL_INFO:-}"
  append_evidence_file "${TOOL_CHECK_LOG:-}"
  append_evidence_file "${MKDIR_LOG:-}"
  append_evidence_file "${REMOTE_CLEAN_LOG:-}"
  append_evidence_file "${SEND_OMC_LOG:-}"
  append_evidence_file "${SEND_INPUT_LOG:-}"
  append_evidence_file "${RUN_LOG:-}"
  append_evidence_file "${REMOTE_FILES_BEFORE_LOG:-}"
  append_evidence_file "${REMOTE_FILES_AFTER_LOG:-}"
  append_evidence_file "${REMOTE_LIST_LOG:-}"
  append_evidence_file "${PULL_OUTPUT_LOG:-}"
  append_evidence_file "${COMPARE_LOG:-}"
  append_evidence_file "${HILOG_CLEAR_LOG:-}"
  append_evidence_file "${HILOG_FILTERED:-}"
  append_evidence_file "${EVIDENCE_DIR}/host-inputs.sha256"
  append_evidence_file "${EVIDENCE_DIR}/output.sha256"
  append_evidence_file "${OUTPUT_LOCAL:-}"
  append_evidence_file "${EVIDENCE_TEXT:-}"

  if [ "${RAW_HILOG_MODE}" = "always" ] || { [ "${RAW_HILOG_MODE}" = "auto" ] && [ -f "${HILOG_RAW:-}" ] && [ ! -s "${HILOG_FILTERED:-}" ]; }; then
    append_evidence_file "${HILOG_RAW:-}"
  fi

  append_evidence_file "${EVIDENCE_MANIFEST}"
  awk 'NF && !seen[$0]++' "${EVIDENCE_MANIFEST}" > "${EVIDENCE_MANIFEST}.tmp"
  mv "${EVIDENCE_MANIFEST}.tmp" "${EVIDENCE_MANIFEST}"
}

export_evidence_bundle() {
  local archive_dir
  local status

  [ "${EXPORT_LOGS:-0}" -eq 1 ] || return 0
  [ "${EVIDENCE_INITIALIZED:-0}" -eq 1 ] || return 0
  [ "${EXPORT_DONE:-0}" -eq 0 ] || return 0
  [ "${EXPORT_IN_PROGRESS:-0}" -eq 0 ] || return 0

  EXPORT_IN_PROGRESS=1

  write_evidence_text_report
  if [ -s "${EVIDENCE_TEXT:-}" ]; then
    log "evidence text report: ${EVIDENCE_TEXT}"
  fi

  if ! command -v tar >/dev/null 2>&1; then
    warn "tar not found; skipping evidence archive export"
    EXPORT_DONE=1
    EXPORT_IN_PROGRESS=0
    return 0
  fi

  write_evidence_manifest
  if [ ! -s "${EVIDENCE_MANIFEST}" ]; then
    warn "no evidence files found to export"
    EXPORT_DONE=1
    EXPORT_IN_PROGRESS=0
    return 0
  fi

  archive_dir="$(dirname "${EVIDENCE_ARCHIVE}")"
  mkdir -p "${archive_dir}"

  set +e
  tar -czf "${EVIDENCE_ARCHIVE}" -C "${EVIDENCE_DIR}" -T "${EVIDENCE_MANIFEST}"
  status=$?
  set -e

  if [ "${status}" -ne 0 ]; then
    warn "failed to create evidence archive: ${EVIDENCE_ARCHIVE}"
  else
    sha256_file "${EVIDENCE_ARCHIVE}" > "${EVIDENCE_ARCHIVE}.sha256" || true
    log "evidence manifest: ${EVIDENCE_MANIFEST}"
    log "evidence archive: ${EVIDENCE_ARCHIVE}"
    if [ -s "${EVIDENCE_ARCHIVE}.sha256" ]; then
      log "evidence archive sha256: ${EVIDENCE_ARCHIVE}.sha256"
    fi
  fi

  EXPORT_DONE=1
  EXPORT_IN_PROGRESS=0
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
      [ "$#" -ge 2 ] || die "--input requires a path or comma-separated paths"
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
    --target-soc)
      [ "$#" -ge 2 ] || die "--target-soc requires a SoC value"
      TARGET_SOC="$2"
      shift 2
      ;;
    --model-run-tool)
      [ "$#" -ge 2 ] || die "--model-run-tool requires a path"
      MODEL_RUN_TOOL="$2"
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
    --evidence-archive)
      [ "$#" -ge 2 ] || die "--evidence-archive requires a path"
      EVIDENCE_ARCHIVE="$2"
      EVIDENCE_ARCHIVE_EXPLICIT=1
      shift 2
      ;;
    --evidence-text)
      [ "$#" -ge 2 ] || die "--evidence-text requires a path"
      EVIDENCE_TEXT="$2"
      EVIDENCE_TEXT_EXPLICIT=1
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
    --diag-timeout)
      [ "$#" -ge 2 ] || die "--diag-timeout requires a number"
      DIAG_TIMEOUT="$2"
      shift 2
      ;;
    --skip-tool-check)
      CHECK_TOOL=0
      shift
      ;;
    --skip-soc-check)
      CHECK_SOC=0
      shift
      ;;
    --no-diagnostics)
      COLLECT_DIAGS=0
      shift
      ;;
    --no-export-logs)
      EXPORT_LOGS=0
      shift
      ;;
    --no-export-text)
      EXPORT_TEXT=0
      shift
      ;;
    --include-raw-hilog)
      RAW_HILOG_MODE="always"
      shift
      ;;
    --no-raw-hilog)
      RAW_HILOG_MODE="never"
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
  if [ "${GOLDEN_EXPLICIT}" -eq 0 ] && [ "${OMC_EXPLICIT}" -eq 0 ] && [ "${INPUT_EXPLICIT}" -eq 0 ]; then
    GOLDEN="${BUNDLE_DIR}/y.bin"
  fi
elif [ -d "${DEFAULT_BUNDLE_DIR}" ]; then
  [ "${OMC_EXPLICIT}" -eq 1 ] || OMC="${DEFAULT_BUNDLE_DIR}/SobelCustom.omc"
  [ "${INPUT_EXPLICIT}" -eq 1 ] || INPUT="${DEFAULT_BUNDLE_DIR}/x.bin"
  if [ "${GOLDEN_EXPLICIT}" -eq 0 ] && [ "${OMC_EXPLICIT}" -eq 0 ] && [ "${INPUT_EXPLICIT}" -eq 0 ]; then
    GOLDEN="${DEFAULT_BUNDLE_DIR}/y.bin"
  fi
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

case "${DIAG_TIMEOUT}" in
  ''|*[!0-9]*)
    die "--diag-timeout must be a positive integer"
    ;;
esac

if [ "${DIAG_TIMEOUT}" -lt 1 ]; then
  die "--diag-timeout must be at least 1"
fi

case "${RAW_HILOG_MODE}" in
  auto|always|never)
    ;;
  *)
    die "RAW_HILOG_MODE must be auto, always, or never"
    ;;
esac

if ! command -v hdc >/dev/null 2>&1 && [ -f "${ROOT}/scripts/local-macos-env.sh" ]; then
  # shellcheck disable=SC1091
  source "${ROOT}/scripts/local-macos-env.sh" >/dev/null 2>&1 || true
fi

HDC_BIN="$(command -v hdc || true)"
[ -n "${HDC_BIN}" ] || die "hdc not found in PATH"

if [ -z "${EVIDENCE_DIR}" ]; then
  STAMP="$(date +%Y%m%d_%H%M%S)"
  EVIDENCE_DIR="${ROOT}/artifacts/naked-omc-runs/${STAMP}"
fi

if [ -z "${EVIDENCE_ARCHIVE}" ]; then
  EVIDENCE_ARCHIVE="${EVIDENCE_DIR}.tgz"
fi
if [ -z "${EVIDENCE_TEXT}" ]; then
  EVIDENCE_TEXT="${EVIDENCE_DIR}/evidence-report.txt"
fi

if [ "${DRY_RUN}" -eq 1 ]; then
  cat <<EOF
HDC_BIN=${HDC_BIN}
TARGET=${TARGET:-<auto>}
TARGET_SOC=${TARGET_SOC:-<auto>}
MODEL_RUN_TOOL=${MODEL_RUN_TOOL}
BUNDLE_DIR=${BUNDLE_DIR:-<none>}
OMC=${OMC:-<missing>}
INPUT=${INPUT:-<missing>}
GOLDEN=${GOLDEN:-<none>}
DEVICE_DIR=${DEVICE_DIR}
OUTPUT_NAME=${OUTPUT_NAME}
LOG_SECONDS=${LOG_SECONDS}
HILOG_CLEAR_TIMEOUT=${HILOG_CLEAR_TIMEOUT}
DIAG_TIMEOUT=${DIAG_TIMEOUT}
EVIDENCE_DIR=${EVIDENCE_DIR}
EVIDENCE_ARCHIVE=${EVIDENCE_ARCHIVE}
EVIDENCE_TEXT=${EVIDENCE_TEXT}
CAPTURE_LOGS=${CAPTURE_LOGS}
HILOG_CLEAR=${HILOG_CLEAR}
CHECK_TOOL=${CHECK_TOOL}
CHECK_SOC=${CHECK_SOC}
COLLECT_DIAGS=${COLLECT_DIAGS}
EXPORT_LOGS=${EXPORT_LOGS}
EXPORT_TEXT=${EXPORT_TEXT}
RAW_HILOG_MODE=${RAW_HILOG_MODE}
RUNNER_LAUNCH_ERROR_RE=${RUNNER_LAUNCH_ERROR_RE}
PULL_OUTPUT=${PULL_OUTPUT}
COMPARE=${COMPARE}
STRICT=${STRICT}
LOG_RE=${LOG_RE}
EOF
  exit 0
fi

mkdir -p "${EVIDENCE_DIR}"
EVIDENCE_DIR="$(cd "${EVIDENCE_DIR}" && pwd -P)"
if [ "${EVIDENCE_ARCHIVE_EXPLICIT}" -eq 0 ]; then
  EVIDENCE_ARCHIVE="${EVIDENCE_DIR}.tgz"
fi
if [ "${EVIDENCE_TEXT_EXPLICIT}" -eq 0 ]; then
  EVIDENCE_TEXT="${EVIDENCE_DIR}/evidence-report.txt"
fi

[ -f "${OMC}" ] || die "OMC file not found: ${OMC:-<missing>}"
[ -n "${INPUT}" ] || die "input file not provided"
IFS=',' read -r -a INPUT_FILES <<< "${INPUT}"
for input_file in "${INPUT_FILES[@]}"; do
  [ -n "${input_file}" ] || die "empty input path in --input"
  [ -f "${input_file}" ] || die "input file not found: ${input_file}"
done

if [ -n "${GOLDEN}" ] && [ ! -f "${GOLDEN}" ]; then
  warn "golden file not found; disabling compare: ${GOLDEN}"
  COMPARE=0
fi

SUMMARY="${EVIDENCE_DIR}/summary.txt"
EVIDENCE_MANIFEST="${EVIDENCE_DIR}/evidence-files.txt"
TARGETS_FILE="${EVIDENCE_DIR}/hdc-targets.txt"
HDC_INFO="${EVIDENCE_DIR}/hdc-info.txt"
TARGET_INFO="${EVIDENCE_DIR}/target-info.txt"
MODEL_INFO="${EVIDENCE_DIR}/model-strings-info.txt"
SOC_CHECK_LOG="${EVIDENCE_DIR}/soc-preflight.log"
TARGET_DIAG_LOG="${EVIDENCE_DIR}/target-diagnostics.log"
RUNNER_DIAG_LOG="${EVIDENCE_DIR}/runner-diagnostics.log"
TOOL_CHECK_LOG="${EVIDENCE_DIR}/model-run-tool-check.log"
MKDIR_LOG="${EVIDENCE_DIR}/device-mkdir.log"
REMOTE_CLEAN_LOG="${EVIDENCE_DIR}/remote-clean-output.log"
SEND_OMC_LOG="${EVIDENCE_DIR}/send-omc.log"
SEND_INPUT_LOG="${EVIDENCE_DIR}/send-input.log"
RUN_LOG="${EVIDENCE_DIR}/model-run-tool.log"
REMOTE_LIST_LOG="${EVIDENCE_DIR}/remote-list-after-run.log"
REMOTE_FILES_BEFORE_LOG="${EVIDENCE_DIR}/remote-files-before-run.log"
REMOTE_FILES_AFTER_LOG="${EVIDENCE_DIR}/remote-files-after-run.log"
HILOG_CLEAR_LOG="${EVIDENCE_DIR}/hilog-clear.log"
HILOG_RAW="${EVIDENCE_DIR}/hilog.raw.log"
HILOG_FILTERED="${EVIDENCE_DIR}/hilog.filtered.log"
PULL_OUTPUT_LOG="${EVIDENCE_DIR}/pull-output.log"
COMPARE_LOG="${EVIDENCE_DIR}/compare.log"
OUTPUT_LOCAL="${EVIDENCE_DIR}/${OUTPUT_NAME}"
OUTPUT_REMOTE="${DEVICE_DIR}/${OUTPUT_NAME}"
EVIDENCE_INITIALIZED=1
RUNNER_LAUNCH_FAILED=0
MODEL_LOAD_FAILED=0
RUN_FAILED=0
RUN_FAILURE_REASON=""

log "evidence dir: ${EVIDENCE_DIR}"
log "evidence archive: ${EVIDENCE_ARCHIVE}"
log "model_run_tool: ${MODEL_RUN_TOOL}"
log "remote work dir: ${DEVICE_DIR}"
log "omc: ${OMC}"
log "input: ${INPUT}"

{
  sha256_file "${OMC}" || true
  for input_file in "${INPUT_FILES[@]}"; do
    sha256_file "${input_file}" || true
  done
  if [ -n "${GOLDEN}" ] && [ -f "${GOLDEN}" ]; then
    sha256_file "${GOLDEN}" || true
  fi
} > "${EVIDENCE_DIR}/host-inputs.sha256"

MODEL_SOC_VERSIONS="$(extract_model_soc_versions "${OMC}" || true)"
write_model_metadata "${OMC}" > "${MODEL_INFO}" || true
if [ -n "${MODEL_SOC_VERSIONS}" ]; then
  log "model soc hint(s): $(format_soc_set "${MODEL_SOC_VERSIONS}")"
fi

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
  echo "### hdc binary"
  printf '%s\n' "${HDC_BIN}"
  echo
  echo "### hdc version"
  "${HDC_BIN}" -v 2>&1 || "${HDC_BIN}" version 2>&1 || true
  echo
  echo "### hdc checkserver"
  "${HDC_BIN}" checkserver 2>&1 || true
  echo
  echo "### hdc list targets -v"
  "${HDC_BIN}" list targets -v 2>&1 || true
} > "${HDC_INFO}"

{
  echo "target=${TARGET}"
  echo "model_run_tool=${MODEL_RUN_TOOL}"
  echo "device_dir=${DEVICE_DIR}"
  echo
  for key in \
    const.product.model \
    const.product.name \
    const.product.device \
    const.product.board \
    const.product.hardwareversion \
    const.product.manufacturer \
    const.product.software.version \
    const.ohos.apiversion \
    ohos.boot.chiptype \
    const.product.soc \
    const.product.socversion \
    const.soc_version \
    ro.product.model \
    ro.product.name \
    ro.product.device \
    ro.product.board \
    ro.board.platform \
    ro.hardware \
    ro.soc.model \
    ro.soc.manufacturer
  do
    printf '%s=' "${key}"
    "${HDC_TARGET[@]}" shell param get "${key}" 2>&1 || true
  done
  echo
  "${HDC_TARGET[@]}" shell uname -a 2>&1 || true
} | tee "${TARGET_INFO}" >/dev/null

# shellcheck disable=SC2016
TARGET_DIAG_CMD='echo "### identity"; id 2>&1 || true; whoami 2>&1 || true; pwd 2>&1 || true; echo; echo "### kernel"; uname -a 2>&1 || true; echo; echo "### hdc shell path"; echo "$PATH" 2>&1 || true; echo; echo "### selected params"; for key in const.product.model const.product.name const.product.device const.product.board const.product.hardwareversion const.product.manufacturer const.product.software.version const.ohos.apiversion ohos.boot.chiptype const.product.soc const.product.socversion const.soc_version ro.product.model ro.product.name ro.product.device ro.product.board ro.board.platform ro.hardware ro.soc.model ro.soc.manufacturer ro.build.version.incremental ro.build.version.release; do printf "%s=" "$key"; param get "$key" 2>&1 || true; done; echo; echo "### param scan"; param ls 2>&1 | grep -Ei "kirin|soc|chip|npu|hiai|ai|product|hardware|board|device|model|nn|neural" | head -400 || true; echo; echo "### process scan"; (ps -ef 2>/dev/null || ps -A 2>/dev/null || true) | grep -Ei "hiai|nn|npu|ai|model|neural" | head -160 || true; echo; echo "### storage"; df -h /data /data/local/tmp 2>&1 || true; echo; echo "### mount info"; mount 2>&1 | grep -E " /data |/data/local|tmp" | head -80 || true; echo; echo "### likely runtime libs"; for d in /system/lib64 /system/lib /vendor/lib64 /vendor/lib /chip_prod/lib64 /chip_prod/lib /data/local/tmp; do [ -d "$d" ] || continue; echo "# $d"; ls -l "$d"/libhiai* "$d"/libneural_network* "$d"/*nn* "$d"/*NN* 2>/dev/null || true; done'
collect_remote_diag "target diagnostics" "${TARGET_DIAG_LOG}" "${TARGET_DIAG_CMD}"

RUNNER_DIAG_CMD="echo '### runner path'; ls -l $(remote_quote "${MODEL_RUN_TOOL}") 2>&1 || true; echo; echo '### runner hash'; if command -v sha256sum >/dev/null 2>&1; then sha256sum $(remote_quote "${MODEL_RUN_TOOL}") 2>&1 || true; elif command -v md5sum >/dev/null 2>&1; then md5sum $(remote_quote "${MODEL_RUN_TOOL}") 2>&1 || true; else echo 'sha256sum/md5sum unavailable'; fi; echo; echo '### runner file type'; if command -v file >/dev/null 2>&1; then file $(remote_quote "${MODEL_RUN_TOOL}") 2>&1 || true; else echo 'file unavailable'; fi; echo; echo '### runner interpreter'; if command -v readelf >/dev/null 2>&1; then readelf -l $(remote_quote "${MODEL_RUN_TOOL}") 2>&1 | grep -Ei 'interpreter|program interpreter' || true; else echo 'readelf unavailable'; fi; echo; echo '### runner version'; $(remote_quote "${MODEL_RUN_TOOL}") --version 2>&1 || true; echo; echo '### runner help'; $(remote_quote "${MODEL_RUN_TOOL}") --help 2>&1 || true; echo; echo '### runner strings'; if command -v strings >/dev/null 2>&1; then strings $(remote_quote "${MODEL_RUN_TOOL}") 2>/dev/null | grep -Ei 'usage|--model|--input|--output|soc|kirin|hiai|HIAI_F|nn|neural|cann|omc|offline|compat|compil|build|executor|device|version|model_run_tool|error|status|return' | head -220 || true; else echo 'strings unavailable'; fi"
collect_remote_diag "runner diagnostics" "${RUNNER_DIAG_LOG}" "${RUNNER_DIAG_CMD}"

SOC_CHECK_RESULT="SKIPPED"
TARGET_SOC_VERSIONS=""
if [ "${CHECK_SOC}" -eq 1 ]; then
  if [ -n "${TARGET_SOC}" ]; then
    TARGET_SOC_VERSIONS="$(printf '%s\n' "${TARGET_SOC}" | extract_soc_versions_from_text || true)"
    [ -n "${TARGET_SOC_VERSIONS}" ] || die "--target-soc must look like a Kirin target, for example kirin9020 or KirinX90"
    TARGET_SOC_SOURCE="cli/env"
  else
    TARGET_SOC_SCAN_FILES=("${TARGET_INFO}")
    if [ -f "${TARGET_DIAG_LOG}" ]; then
      TARGET_SOC_SCAN_FILES+=("${TARGET_DIAG_LOG}")
    fi
    TARGET_SOC_VERSIONS="$(cat "${TARGET_SOC_SCAN_FILES[@]}" | extract_soc_versions_from_text || true)"
    TARGET_SOC_SOURCE="target-info,target-diagnostics"
  fi

  {
    echo "check_soc=${CHECK_SOC}"
    echo "model_soc_versions=$(format_soc_set "${MODEL_SOC_VERSIONS}")"
    echo "target_soc_source=${TARGET_SOC_SOURCE}"
    echo "target_soc_versions=$(format_soc_set "${TARGET_SOC_VERSIONS}")"
  } > "${SOC_CHECK_LOG}"

  if [ -z "${MODEL_SOC_VERSIONS}" ]; then
    SOC_CHECK_RESULT="UNKNOWN_MODEL_SOC"
    echo "result=${SOC_CHECK_RESULT}" >> "${SOC_CHECK_LOG}"
    warn "could not extract SoC metadata from OMC; continuing because compatibility cannot be preflighted"
  elif [ -z "${TARGET_SOC_VERSIONS}" ]; then
    SOC_CHECK_RESULT="UNKNOWN_TARGET_SOC"
    echo "result=${SOC_CHECK_RESULT}" >> "${SOC_CHECK_LOG}"
    if [ "${STRICT}" -eq 1 ]; then
      die "could not infer target SoC from device params while model SoC is known ($(format_soc_set "${MODEL_SOC_VERSIONS}")). Pass --target-soc, or use --skip-soc-check/--no-strict to run anyway. Inspect ${SOC_CHECK_LOG} and ${TARGET_INFO}"
    fi
    warn "could not infer target SoC from device params; continuing because --no-strict is set. Pass --target-soc to make the compatibility preflight deterministic"
  elif soc_sets_intersect "${MODEL_SOC_VERSIONS}" "${TARGET_SOC_VERSIONS}"; then
    SOC_CHECK_RESULT="PASS"
    echo "result=${SOC_CHECK_RESULT}" >> "${SOC_CHECK_LOG}"
    log "soc preflight passed: model=$(format_soc_set "${MODEL_SOC_VERSIONS}") target=$(format_soc_set "${TARGET_SOC_VERSIONS}")"
  else
    SOC_CHECK_RESULT="MISMATCH"
    echo "result=${SOC_CHECK_RESULT}" >> "${SOC_CHECK_LOG}"
    if [ "${STRICT}" -eq 1 ]; then
      die "soc preflight failed: model=$(format_soc_set "${MODEL_SOC_VERSIONS}") target=$(format_soc_set "${TARGET_SOC_VERSIONS}"). Inspect ${SOC_CHECK_LOG}; use --no-strict to continue anyway."
    fi
    warn "soc preflight mismatch: model=$(format_soc_set "${MODEL_SOC_VERSIONS}") target=$(format_soc_set "${TARGET_SOC_VERSIONS}"); continuing because --no-strict is set"
  fi
fi

if [ "${CHECK_TOOL}" -eq 1 ]; then
  log "checking model_run_tool"
  TOOL_CHECK_CMD="echo '### runner path'; ls -l $(remote_quote "${MODEL_RUN_TOOL}") 2>&1 || true; echo; echo '### runner launch probe'; $(remote_quote "${MODEL_RUN_TOOL}") --version 2>&1 || $(remote_quote "${MODEL_RUN_TOOL}") --help 2>&1 || true"
  set +e
  "${HDC_TARGET[@]}" shell "${TOOL_CHECK_CMD}" > "${TOOL_CHECK_LOG}" 2>&1
  TOOL_STATUS=$?
  set -e
  if [ "${TOOL_STATUS}" -ne 0 ] || file_matches "${TOOL_CHECK_LOG}" "${RUNNER_LAUNCH_ERROR_RE}"; then
    RUNNER_LAUNCH_FAILED=1
    RUN_FAILED=1
    RUN_FAILURE_REASON="model_run_tool launch failed"
    die "model_run_tool is missing, inaccessible, or cannot be launched on this target: ${MODEL_RUN_TOOL}. Inspect ${TOOL_CHECK_LOG} and ${RUNNER_DIAG_LOG}"
  fi
  if ! file_matches "${TOOL_CHECK_LOG}" 'model_run_tool|usage|version'; then
    warn "model_run_tool launch probe produced no recognizable usage/version output; inspect ${TOOL_CHECK_LOG}"
  fi
fi

if [ "${CAPTURE_LOGS}" -eq 1 ] && [ "${HILOG_CLEAR}" -eq 1 ]; then
  log "clearing hilog with ${HILOG_CLEAR_TIMEOUT}s timeout"
  set +e
  run_with_timeout "${HILOG_CLEAR_TIMEOUT}" "${HILOG_CLEAR_LOG}" "${HDC_TARGET[@]}" shell "hilog -r"
  HILOG_CLEAR_STATUS=$?
  set -e
  if [ "${HILOG_CLEAR_STATUS}" -eq 124 ]; then
    warn "hdc shell hilog -r timed out after ${HILOG_CLEAR_TIMEOUT}s; continuing without clearing logs"
  elif [ "${HILOG_CLEAR_STATUS}" -ne 0 ]; then
    warn "hdc shell hilog -r failed with status ${HILOG_CLEAR_STATUS}; continuing without clearing logs"
  fi
fi

log "ensuring remote work dir exists"
set +e
"${HDC_TARGET[@]}" shell "mkdir -p $(remote_quote "${DEVICE_DIR}")" > "${MKDIR_LOG}" 2>&1
MKDIR_STATUS=$?
set -e
if [ "${MKDIR_STATUS}" -ne 0 ]; then
  die "failed to create device directory; inspect ${MKDIR_LOG}"
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

OMC_BASENAME="$(basename "${OMC}")"
REMOTE_OMC="${DEVICE_DIR}/${OMC_BASENAME}"

log "sending OMC to device"
set +e
"${HDC_TARGET[@]}" file send "${OMC}" "${REMOTE_OMC}" 2>&1 | tee "${SEND_OMC_LOG}"
SEND_OMC_STATUS=${PIPESTATUS[0]}
set -e
if [ "${SEND_OMC_STATUS}" -ne 0 ] || file_matches "${SEND_OMC_LOG}" 'error:|failed|Error Code'; then
  die "failed to send OMC; inspect ${SEND_OMC_LOG}"
fi

: > "${SEND_INPUT_LOG}"
REMOTE_INPUTS=""
REMOTE_INPUT_FILES_FOR_DIAG=""
for input_file in "${INPUT_FILES[@]}"; do
  input_base="$(basename "${input_file}")"
  remote_input="${DEVICE_DIR}/${input_base}"
  log "sending input to device: ${input_base}"
  set +e
  "${HDC_TARGET[@]}" file send "${input_file}" "${remote_input}" 2>&1 | tee -a "${SEND_INPUT_LOG}"
  SEND_INPUT_STATUS=${PIPESTATUS[0]}
  set -e
  if [ "${SEND_INPUT_STATUS}" -ne 0 ] || tail -20 "${SEND_INPUT_LOG}" | grep -Eiq 'error:|failed|Error Code'; then
    die "failed to send input; inspect ${SEND_INPUT_LOG}"
  fi
  if [ -z "${REMOTE_INPUTS}" ]; then
    REMOTE_INPUTS="${remote_input}"
  else
    REMOTE_INPUTS="${REMOTE_INPUTS},${remote_input}"
  fi
  REMOTE_INPUT_FILES_FOR_DIAG="${REMOTE_INPUT_FILES_FOR_DIAG} $(remote_quote "${remote_input}")"
done

REMOTE_FILES_FOR_DIAG="$(remote_quote "${REMOTE_OMC}") ${REMOTE_INPUT_FILES_FOR_DIAG} $(remote_quote "${OUTPUT_REMOTE}")"
REMOTE_FILES_CMD="echo '### model/input/output files'; ls -l ${REMOTE_FILES_FOR_DIAG} 2>&1 || true; echo; echo '### hashes if available'; if command -v sha256sum >/dev/null 2>&1; then sha256sum ${REMOTE_FILES_FOR_DIAG} 2>&1 || true; elif command -v md5sum >/dev/null 2>&1; then md5sum ${REMOTE_FILES_FOR_DIAG} 2>&1 || true; else echo 'no sha256sum/md5sum on target'; fi; echo; echo '### work dir newest'; ls -lt $(remote_quote "${DEVICE_DIR}") 2>&1 | head -50 || true"
collect_remote_diag "remote files before run" "${REMOTE_FILES_BEFORE_LOG}" "${REMOTE_FILES_CMD}"

log "removing stale output"
"${HDC_TARGET[@]}" shell "rm -f $(remote_quote "${OUTPUT_REMOTE}")" > "${REMOTE_CLEAN_LOG}" 2>&1 || true

RUN_CMD="$(remote_quote "${MODEL_RUN_TOOL}") --model=$(remote_quote "${REMOTE_OMC}") --input=$(remote_quote "${REMOTE_INPUTS}") --output_dir=$(remote_quote "${DEVICE_DIR}/")"
log "running model_run_tool"
set +e
"${HDC_TARGET[@]}" shell "${RUN_CMD}" 2>&1 | tee "${RUN_LOG}"
RUN_STATUS=${PIPESTATUS[0]}
set -e
if file_matches "${RUN_LOG}" 'Load model .* failed|loading model .* failed|Model Process ret failed|ConstructWithOfflineModelBuffer failed|OH_NNCompilation_Build failed'; then
  MODEL_LOAD_FAILED=1
  warn "model_run_tool reached the model loader, but the model failed to load. This points at OMC/target SoC/runtime compatibility, not HAP signing or aa start."
fi
if file_matches "${RUN_LOG}" "${RUNNER_LAUNCH_ERROR_RE}"; then
  RUNNER_LAUNCH_FAILED=1
  warn "model_run_tool could not launch on this target. This points at a missing/incompatible runner binary or target execution environment, not OMC model compatibility."
fi
if [ "${RUN_STATUS}" -ne 0 ] || file_matches "${RUN_LOG}" 'error:|failed|No such file|not found|permission denied|Error Code'; then
  RUN_FAILED=1
  if [ "${RUNNER_LAUNCH_FAILED}" -eq 1 ]; then
    RUN_FAILURE_REASON="model_run_tool launch failed"
  elif [ "${MODEL_LOAD_FAILED}" -eq 1 ]; then
    RUN_FAILURE_REASON="model load failed"
  else
    RUN_FAILURE_REASON="model_run_tool failed"
  fi
  if [ "${STRICT}" -eq 1 ]; then
    warn "${RUN_FAILURE_REASON}; collecting diagnostics before exiting"
  else
    warn "model_run_tool did not look successful; continuing because --no-strict is set"
  fi
fi

"${HDC_TARGET[@]}" shell "ls -lt $(remote_quote "${DEVICE_DIR}") | head -30" > "${REMOTE_LIST_LOG}" 2>&1 || true
collect_remote_diag "remote files after run" "${REMOTE_FILES_AFTER_LOG}" "${REMOTE_FILES_CMD}"

if [ "${CAPTURE_LOGS}" -eq 1 ]; then
  log "waiting ${LOG_SECONDS}s for runner logs"
  sleep "${LOG_SECONDS}"
  cleanup
  grep -E "${LOG_RE}" "${HILOG_RAW}" > "${HILOG_FILTERED}" || true
fi

OUTPUT_PULLED=0
if [ "${PULL_OUTPUT}" -eq 1 ]; then
  log "pulling output: ${OUTPUT_REMOTE}"
  set +e
  "${HDC_TARGET[@]}" file recv "${OUTPUT_REMOTE}" "${OUTPUT_LOCAL}" 2>&1 | tee "${PULL_OUTPUT_LOG}"
  PULL_STATUS=${PIPESTATUS[0]}
  set -e
  if [ "${PULL_STATUS}" -eq 0 ] && [ -s "${OUTPUT_LOCAL}" ] && ! file_matches "${PULL_OUTPUT_LOG}" 'error:|failed|Error Code|No such file'; then
    OUTPUT_PULLED=1
    sha256_file "${OUTPUT_LOCAL}" > "${EVIDENCE_DIR}/output.sha256" || true
  else
    warn "output was not pulled successfully; inspect ${PULL_OUTPUT_LOG} and ${REMOTE_LIST_LOG}"
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
  echo "model_run_tool=${MODEL_RUN_TOOL}"
  echo "device_dir=${DEVICE_DIR}"
  echo "omc=${OMC}"
  echo "input=${INPUT}"
  echo "golden=${GOLDEN:-}"
  echo "check_soc=${CHECK_SOC}"
  echo "model_soc_versions=$(format_soc_set "${MODEL_SOC_VERSIONS:-}")"
  echo "target_soc_versions=$(format_soc_set "${TARGET_SOC_VERSIONS:-}")"
  echo "soc_check_result=${SOC_CHECK_RESULT:-SKIPPED}"
  echo "output_name=${OUTPUT_NAME}"
  echo "output_remote=${OUTPUT_REMOTE}"
  echo "output_local=${OUTPUT_LOCAL}"
  echo "output_pulled=${OUTPUT_PULLED}"
  echo "compare=${COMPARE}"
  echo "compare_result=${COMPARE_RESULT}"
  echo "run_failed=${RUN_FAILED:-0}"
  echo "run_failure_reason=${RUN_FAILURE_REASON:-}"
  echo "runner_launch_failed=${RUNNER_LAUNCH_FAILED:-0}"
  echo "model_load_failed=${MODEL_LOAD_FAILED:-0}"
  echo "capture_logs=${CAPTURE_LOGS}"
  echo "collect_diagnostics=${COLLECT_DIAGS}"
  echo "export_logs=${EXPORT_LOGS}"
  echo "export_text=${EXPORT_TEXT}"
  echo "raw_hilog_mode=${RAW_HILOG_MODE}"
  echo "evidence_manifest=${EVIDENCE_MANIFEST}"
  echo "evidence_archive=${EVIDENCE_ARCHIVE}"
  echo "evidence_text=${EVIDENCE_TEXT}"
  echo "diag_timeout=${DIAG_TIMEOUT}"
  echo "strict=${STRICT}"
  echo "result=${RESULT}"
  echo
  echo "host_input_hashes:"
  cat "${EVIDENCE_DIR}/host-inputs.sha256"
  echo
  echo "important_files:"
  echo "- ${TARGET_INFO}"
  echo "- ${HDC_INFO}"
  echo "- ${MODEL_INFO}"
  echo "- ${SOC_CHECK_LOG}"
  echo "- ${TARGET_DIAG_LOG}"
  echo "- ${RUNNER_DIAG_LOG}"
  echo "- ${TOOL_CHECK_LOG}"
  echo "- ${SEND_OMC_LOG}"
  echo "- ${SEND_INPUT_LOG}"
  echo "- ${RUN_LOG}"
  echo "- ${REMOTE_LIST_LOG}"
  echo "- ${REMOTE_FILES_BEFORE_LOG}"
  echo "- ${REMOTE_FILES_AFTER_LOG}"
  echo "- ${PULL_OUTPUT_LOG}"
  echo "- ${COMPARE_LOG}"
  echo "- ${HILOG_FILTERED}"
} > "${SUMMARY}"

export_evidence_bundle

log "summary: ${SUMMARY}"
if [ "${CAPTURE_LOGS}" -eq 1 ]; then
  log "filtered hilog: ${HILOG_FILTERED}"
fi

if [ "${RESULT}" = "PASS_CANDIDATE" ]; then
  log "PASS_CANDIDATE: output was pulled and matches the golden file."
  exit 0
fi

if [ "${STRICT}" -eq 1 ] && [ "${RUN_FAILED:-0}" -eq 1 ]; then
  die "${RUN_FAILURE_REASON}; inspect ${RUN_LOG}, ${MODEL_INFO}, ${SOC_CHECK_LOG}, ${TARGET_DIAG_LOG}, ${RUNNER_DIAG_LOG}, ${REMOTE_FILES_AFTER_LOG}, and ${HILOG_FILTERED}"
fi

if [ "${STRICT}" -eq 1 ]; then
  die "naked OMC run was not confirmed"
fi

warn "naked OMC run was not confirmed"
exit 0
