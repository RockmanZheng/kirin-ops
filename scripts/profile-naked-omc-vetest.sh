#!/usr/bin/env bash
# Push a naked OMC bundle to a HarmonyOS/Kirin target, collect profiling, and pull evidence.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEFAULT_BUNDLE_DIR="${ROOT}/artifacts/naked-omc/kirin9030-sobel-custom-2026-08-04"
TARGET_SCRIPT="${ROOT}/scripts/target-profile-omc.sh"

BUNDLE_DIR="${BUNDLE_DIR:-}"
BUNDLE_MANIFEST="${BUNDLE_MANIFEST:-}"
BUNDLE_NAME=""
BUNDLE_DESCRIPTION=""
BUNDLE_OMC=""
BUNDLE_INPUT=""
BUNDLE_GOLDEN=""
BUNDLE_OUTPUT_NAME=""
BUNDLE_OUTPUT_TYPE=""
BUNDLE_TARGET_SOC=""
BUNDLE_COMPARE=""
BUNDLE_COMPARE_SCRIPT=""
OMC="${OMC:-}"
INPUT="${INPUT:-}"
GOLDEN="${GOLDEN:-}"
COMPARE_SCRIPT="${COMPARE_SCRIPT:-}"
PYTHON_BIN="${PYTHON_BIN:-}"
OUTPUT_NAME="${OUTPUT_NAME:-output_0}"
OUTPUT_TYPE="${OUTPUT_TYPE:-}"
TARGET="${TARGET:-}"
TARGET_SOC="${TARGET_SOC:-}"
DEVICE_DIR="${DEVICE_DIR:-/data/local/tmp/z84378291}"
MODEL_RUN_TOOL="${MODEL_RUN_TOOL:-}"
DATA_PROC_TOOL="${DATA_PROC_TOOL:-}"
TIMES="${TIMES:-50}"
HILOG_CLEAR_TIMEOUT="${HILOG_CLEAR_TIMEOUT:-5}"
PROFILE_MODE="${PROFILE_MODE:-auto}"
PROFILING_ARG="${PROFILING_ARG:-}"
PROFILE_DIR="${PROFILE_DIR:-prof_data}"
RUN_ID="${RUN_ID:-}"
EVIDENCE_DIR="${EVIDENCE_DIR:-}"
COMPARE=1
COMPARE_EXPLICIT=0
COMPARE_SCRIPT_EXPLICIT=0
NO_DATA_PROC=0
NO_ARCHIVE=0
NO_CLEAN=0
CHECK_SOC=1
HILOG_CLEAR=1
DRY_RUN=0
OMC_EXPLICIT=0
INPUT_EXPLICIT=0
GOLDEN_EXPLICIT=0
OUTPUT_NAME_EXPLICIT=0
OUTPUT_TYPE_EXPLICIT=0
TARGET_SOC_EXPLICIT=0

usage() {
  cat <<'USAGE'
usage: scripts/profile-naked-omc-vetest.sh [options]

One-command profiling collection for a naked .omc bundle on a Kirin/HarmonyOS target.

The script:
  1. Reads bundle.env when --bundle-dir is used.
  2. Auto-selects the hdc target if exactly one target is connected.
  3. Pushes target-profile-omc.sh, the .omc, and input files to the target.
  4. Runs target-profile-omc.sh on-device.
  5. Pulls the profiling archive or run directory back to artifacts/profiling/.
  6. Pulls output_0 and validates it with the Python precision script when available.

Options:
  --bundle-dir DIR       Local bundle dir. Default:
                         artifacts/naked-omc/kirin9030-sobel-custom-2026-08-04
  --bundle-manifest PATH Manifest path. Default: <bundle-dir>/bundle.env.
  --omc PATH             Local .omc file. Overrides bundle OMC.
  --input PATHS          Local input .bin path, or comma-separated local input paths.
  --golden PATH          Optional local golden output file.
  --output-name NAME     Target output name. Default: output_0.
  --output-type TYPE     Target output tensor dtype passed to model_run_tool.
  --target TARGET        hdc target id. Auto-detects when exactly one target is connected.
  --target-soc SOC       Expected target SoC, e.g. Kirin9020 or Kirin9030.
                         Used as a device-side assertion before profiling.
  --skip-soc-check       Do not assert the target SoC before profiling.
                         Use only when intentionally running despite unknown
                         or mismatched SoC metadata.
  --device-dir DIR       Target work root. Default: /data/local/tmp/z84378291
  --model-run-tool PATH  Target-side model_run_tool path. Prefer passing this
                         explicitly for shared real-device test machines.
  --data-proc-tool PATH  Target data_proc_tool path. Default: <device-dir>/data_proc_tool.
  --times N              Profiling inference count when supported. Default: 50.
  --no-clear-logs        Do not run "hdc shell hilog -r" before profiling.
  --hilog-clear-timeout N
                         Seconds to wait for "hdc shell hilog -r". Default: 5.
  --profile-mode MODE    auto, none, or arg. Default: auto.
  --profiling-arg ARG    Explicit target model_run_tool profiling flag.
  --profile-dir NAME     Expected raw profile dir name. Default: prof_data.
  --run-id ID            Stable run id. Default: host timestamp.
  --evidence-dir DIR     Host evidence dir. Default: artifacts/profiling/<run-id>.
  --no-compare           Pull output but skip golden comparison.
  --compare-script PATH  Python precision validator. Required when COMPARE=1.
                         Byte cmp is retired.
  --python-bin PATH      Python interpreter for host-side precision validation.
                         Default: first python with numpy from PATH, /usr/bin,
                         python3.13, python3.12, or python3.11.
  --no-data-proc         Ask target script not to run data_proc_tool.
  --no-archive           Ask target script not to archive the run directory.
  --no-clean             Ask target script not to remove stale profile/output files.
  --dry-run              Print resolved commands without executing them.
  -h, --help             Show this help.

Example:
  scripts/profile-naked-omc-vetest.sh \
    --target "$TARGET_SN" \
    --bundle-dir "$PWD/kirin-sobel-naked-omc-2026-08-03" \
    --device-dir "$REMOTE_DIR" \
    --model-run-tool "$REMOTE_DIR/model_run_tool" \
    --target-soc Kirin9020 \
    --no-clear-logs
USAGE
}

COLOR_RESET=""
COLOR_INFO=""
COLOR_WARN=""
COLOR_ERROR=""
COLOR_SUCCESS=""
COLOR_FORCE=0
if [ "${FORCE_COLOR:-}" = "1" ] || [ "${CLICOLOR_FORCE:-}" = "1" ]; then
  COLOR_FORCE=1
fi
if [ "${COLOR_FORCE}" -eq 1 ] || {
  [ -z "${NO_COLOR:-}" ] && { [ -t 1 ] || [ -t 2 ]; }
}; then
  COLOR_RESET="$(printf '\033[0m')"
  COLOR_INFO="$(printf '\033[36m')"
  COLOR_WARN="$(printf '\033[33m')"
  COLOR_ERROR="$(printf '\033[31m')"
  COLOR_SUCCESS="$(printf '\033[1;32m')"
fi

log() {
  printf '%s[kirin-profile]%s %s\n' "${COLOR_INFO}" "${COLOR_RESET}" "$*"
}

log_success() {
  printf '%s[kirin-profile] PASS:%s %s\n' "${COLOR_SUCCESS}" "${COLOR_RESET}" "$*"
}

warn() {
  printf '%s[kirin-profile] WARN:%s %s\n' "${COLOR_WARN}" "${COLOR_RESET}" "$*" >&2
}

die() {
  printf '%s[kirin-profile] ERROR:%s %s\n' "${COLOR_ERROR}" "${COLOR_RESET}" "$*" >&2
  exit 1
}

trim_value() {
  sed -E 's/^[[:space:]]+//; s/[[:space:]]+$//'
}

unquote_manifest_value() {
  local value="$1"

  value="$(printf '%s' "${value}" | trim_value)"
  case "${value}" in
    \"*\")
      value="${value#\"}"
      value="${value%\"}"
      ;;
    \'*\')
      value="${value#\'}"
      value="${value%\'}"
      ;;
  esac
  printf '%s\n' "${value}"
}

manifest_value() {
  printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
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

resolve_bundle_path() {
  local value="$1"

  [ -n "${value}" ] || return 0
  case "${value}" in
    /*|~*)
      printf '%s\n' "${value}"
      ;;
    *)
      printf '%s/%s\n' "${BUNDLE_DIR}" "${value}"
      ;;
  esac
}

resolve_bundle_input_paths() {
  local value="$1"
  local item
  local resolved
  local output=""

  IFS=',' read -r -a bundle_input_items <<< "${value}"
  for item in "${bundle_input_items[@]}"; do
    item="$(printf '%s' "${item}" | trim_value)"
    [ -n "${item}" ] || continue
    resolved="$(resolve_bundle_path "${item}")"
    if [ -z "${output}" ]; then
      output="${resolved}"
    else
      output="${output},${resolved}"
    fi
  done

  printf '%s\n' "${output}"
}

resolve_compare_script() {
  if [ -n "${COMPARE_SCRIPT}" ]; then
    case "${COMPARE_SCRIPT}" in
      /*|~*)
        ;;
      *)
        if [ -n "${BUNDLE_DIR}" ] && [ -f "${BUNDLE_DIR}/${COMPARE_SCRIPT}" ]; then
          COMPARE_SCRIPT="${BUNDLE_DIR}/${COMPARE_SCRIPT}"
        elif [ -f "${ROOT}/${COMPARE_SCRIPT}" ]; then
          COMPARE_SCRIPT="${ROOT}/${COMPARE_SCRIPT}"
        fi
        ;;
    esac
  fi
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

run_python_compare() {
  local input_for_reference
  local status
  local -a compare_cmd

  if [ -z "${COMPARE_SCRIPT}" ]; then
    {
      echo "compare_script="
      echo "status=127"
      echo "reason=COMPARE_SCRIPT not set; byte comparison is retired"
    } > "${COMPARE_LOG}"
    return 127
  fi

  if [ ! -f "${COMPARE_SCRIPT}" ]; then
    {
      echo "compare_script=${COMPARE_SCRIPT}"
      echo "status=127"
      echo "reason=compare script not found"
    } > "${COMPARE_LOG}"
    return 127
  fi

  if ! select_python_bin; then
    {
      echo "compare_script=${COMPARE_SCRIPT}"
      echo "python_bin=${PYTHON_BIN:-}"
      echo "status=127"
      echo "reason=no Python interpreter with numpy found; set PYTHON_BIN or install numpy"
    } > "${COMPARE_LOG}"
    return 127
  fi

  input_for_reference="${INPUT_FILES[0]}"
  compare_cmd=(
    "${PYTHON_BIN}" "${COMPARE_SCRIPT}"
    --output "${OUTPUT_LOCAL}"
    --golden "${GOLDEN}"
    --input "${input_for_reference}"
  )

  set +e
  {
    echo "compare_script=${COMPARE_SCRIPT}"
    echo "python_bin=${PYTHON_BIN}"
    printf 'compare_command='
    printf '%q ' "${compare_cmd[@]}"
    printf '\n\n'
    "${compare_cmd[@]}"
  } > "${COMPARE_LOG}" 2>&1
  status=$?
  set -e

  return "${status}"
}

load_bundle_manifest() {
  local manifest="$1"
  local line
  local key
  local value

  [ -f "${manifest}" ] || die "bundle manifest not found: ${manifest}"

  while IFS= read -r line || [ -n "${line}" ]; do
    line="${line%$'\r'}"
    case "${line}" in
      ''|\#*)
        continue
        ;;
    esac

    key="${line%%=*}"
    value="${line#*=}"
    [ "${key}" != "${line}" ] || continue

    key="$(printf '%s' "${key}" | trim_value)"
    value="$(unquote_manifest_value "${value}")"

    case "${key}" in
      NAME|BUNDLE_NAME)
        BUNDLE_NAME="${value}"
        ;;
      DESCRIPTION|BUNDLE_DESCRIPTION)
        BUNDLE_DESCRIPTION="${value}"
        ;;
      OMC|MODEL|MODEL_FILE)
        BUNDLE_OMC="${value}"
        ;;
      INPUT|INPUTS)
        BUNDLE_INPUT="${value}"
        ;;
      GOLDEN|GOLDEN_FILE|EXPECTED_OUTPUT)
        BUNDLE_GOLDEN="${value}"
        ;;
      OUTPUT_NAME|OUTPUT)
        BUNDLE_OUTPUT_NAME="${value}"
        ;;
      OUTPUT_TYPE|OUTPUT_DTYPE|MODEL_OUTPUT_TYPE)
        BUNDLE_OUTPUT_TYPE="${value}"
        ;;
      TARGET_SOC|SOC|SOC_VERSION)
        BUNDLE_TARGET_SOC="${value}"
        ;;
      COMPARE)
        BUNDLE_COMPARE="${value}"
        ;;
      COMPARE_SCRIPT|COMPARE_TOOL|OUTPUT_COMPARE_SCRIPT|PRECISION_SCRIPT)
        BUNDLE_COMPARE_SCRIPT="${value}"
        ;;
      COMPARE_MODE|OUTPUT_COMPARE_MODE|COMPARE_VALIDATOR|OUTPUT_COMPARE_VALIDATOR)
        warn "ignoring retired bundle manifest key ${key} in ${manifest}; use COMPARE_SCRIPT"
        ;;
      *)
        warn "ignoring unknown bundle manifest key ${key} in ${manifest}"
        ;;
    esac
  done < "${manifest}"
}

remote_basename_for_input() {
  local file="$1"
  local base
  base="$(basename "${file}")"
  printf '%s/%s\n' "${REMOTE_INPUT_DIR}" "${base}"
}

append_remote_input() {
  local value="$1"

  if [ -z "${REMOTE_INPUTS}" ]; then
    REMOTE_INPUTS="${value}"
  else
    REMOTE_INPUTS="${REMOTE_INPUTS},${value}"
  fi
}

find_archive_path_from_manifest() {
  local manifest="$1"
  local value

  [ -f "${manifest}" ] || return 1
  value="$(awk -F= '$1=="ARCHIVE_PATH" {print substr($0, index($0, "=") + 1)}' "${manifest}" | tail -1)"
  [ -n "${value}" ] || return 1
  printf '%s\n' "${value}"
}

find_output_path_from_manifest() {
  local manifest="$1"
  local value

  [ -f "${manifest}" ] || return 1
  value="$(awk -F= '$1=="OUTPUT_PATH" {print substr($0, index($0, "=") + 1)}' "${manifest}" | tail -1)"
  [ -n "${value}" ] || return 1
  printf '%s\n' "${value}"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --bundle-dir)
      [ "$#" -ge 2 ] || die "--bundle-dir requires a directory"
      BUNDLE_DIR="$2"
      shift 2
      ;;
    --bundle-manifest)
      [ "$#" -ge 2 ] || die "--bundle-manifest requires a path"
      BUNDLE_MANIFEST="$2"
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
    --output-name)
      [ "$#" -ge 2 ] || die "--output-name requires a name"
      OUTPUT_NAME="$2"
      OUTPUT_NAME_EXPLICIT=1
      shift 2
      ;;
    --output-type)
      [ "$#" -ge 2 ] || die "--output-type requires a dtype"
      OUTPUT_TYPE="$2"
      OUTPUT_TYPE_EXPLICIT=1
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
      TARGET_SOC_EXPLICIT=1
      shift 2
      ;;
    --skip-soc-check)
      CHECK_SOC=0
      shift
      ;;
    --device-dir)
      [ "$#" -ge 2 ] || die "--device-dir requires a target path"
      DEVICE_DIR="$2"
      shift 2
      ;;
    --model-run-tool)
      [ "$#" -ge 2 ] || die "--model-run-tool requires a target path"
      MODEL_RUN_TOOL="$2"
      shift 2
      ;;
    --data-proc-tool)
      [ "$#" -ge 2 ] || die "--data-proc-tool requires a target path"
      DATA_PROC_TOOL="$2"
      shift 2
      ;;
    --times)
      [ "$#" -ge 2 ] || die "--times requires a value"
      TIMES="$2"
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
    --profile-mode)
      [ "$#" -ge 2 ] || die "--profile-mode requires auto, none, or arg"
      PROFILE_MODE="$2"
      shift 2
      ;;
    --profiling-arg)
      [ "$#" -ge 2 ] || die "--profiling-arg requires a value"
      PROFILING_ARG="$2"
      shift 2
      ;;
    --profile-dir)
      [ "$#" -ge 2 ] || die "--profile-dir requires a name"
      PROFILE_DIR="$2"
      shift 2
      ;;
    --run-id)
      [ "$#" -ge 2 ] || die "--run-id requires a value"
      RUN_ID="$2"
      shift 2
      ;;
    --evidence-dir)
      [ "$#" -ge 2 ] || die "--evidence-dir requires a directory"
      EVIDENCE_DIR="$2"
      shift 2
      ;;
    --no-compare)
      COMPARE=0
      COMPARE_EXPLICIT=1
      shift
      ;;
    --compare-script)
      [ "$#" -ge 2 ] || die "--compare-script requires a Python script path"
      COMPARE_SCRIPT="$2"
      COMPARE_SCRIPT_EXPLICIT=1
      shift 2
      ;;
    --python-bin)
      [ "$#" -ge 2 ] || die "--python-bin requires a Python interpreter path"
      PYTHON_BIN="$2"
      shift 2
      ;;
    --compare-mode|--compare-validator)
      die "$1 is retired; use --compare-script or bundle.env COMPARE_SCRIPT"
      ;;
    --no-data-proc)
      NO_DATA_PROC=1
      shift
      ;;
    --no-archive)
      NO_ARCHIVE=1
      shift
      ;;
    --no-clean)
      NO_CLEAN=1
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

case "${TIMES}" in
  ''|*[!0-9]*)
    die "--times must be an integer"
    ;;
esac

case "${HILOG_CLEAR_TIMEOUT}" in
  ''|*[!0-9]*)
    die "--hilog-clear-timeout must be an integer"
    ;;
esac
if [ "${HILOG_CLEAR_TIMEOUT}" -lt 1 ]; then
  die "--hilog-clear-timeout must be at least 1"
fi

case "${PROFILE_MODE}" in
  auto|none|arg)
    ;;
  *)
    die "--profile-mode must be auto, none, or arg"
    ;;
esac

[ -f "${TARGET_SCRIPT}" ] || die "target profiling script not found: ${TARGET_SCRIPT}"
if ! command -v hdc >/dev/null 2>&1 && [ -f "${ROOT}/scripts/local-macos-env.sh" ]; then
  # shellcheck disable=SC1091
  source "${ROOT}/scripts/local-macos-env.sh" >/dev/null 2>&1 || true
fi
command -v hdc >/dev/null 2>&1 || die "hdc not found in PATH"

if [ -z "${BUNDLE_DIR}" ] && [ -n "${BUNDLE_MANIFEST}" ]; then
  [ -f "${BUNDLE_MANIFEST}" ] || die "bundle manifest not found: ${BUNDLE_MANIFEST}"
  BUNDLE_DIR="$(cd "$(dirname "${BUNDLE_MANIFEST}")" && pwd -P)"
  BUNDLE_MANIFEST="$(cd "$(dirname "${BUNDLE_MANIFEST}")" && pwd -P)/$(basename "${BUNDLE_MANIFEST}")"
fi

if [ -z "${BUNDLE_DIR}" ] && [ "${OMC_EXPLICIT}" -eq 0 ] && [ -d "${DEFAULT_BUNDLE_DIR}" ]; then
  BUNDLE_DIR="${DEFAULT_BUNDLE_DIR}"
fi

if [ -n "${BUNDLE_DIR}" ]; then
  [ -d "${BUNDLE_DIR}" ] || die "bundle dir not found: ${BUNDLE_DIR}"
  BUNDLE_DIR="$(cd "${BUNDLE_DIR}" && pwd -P)"
  if [ -z "${BUNDLE_MANIFEST}" ] && [ -f "${BUNDLE_DIR}/bundle.env" ]; then
    BUNDLE_MANIFEST="${BUNDLE_DIR}/bundle.env"
  fi
  if [ -n "${BUNDLE_MANIFEST}" ]; then
    load_bundle_manifest "${BUNDLE_MANIFEST}"
  fi
fi

if [ "${OMC_EXPLICIT}" -eq 0 ]; then
  if [ -n "${BUNDLE_OMC}" ]; then
    OMC="$(resolve_bundle_path "${BUNDLE_OMC}")"
  elif [ -n "${BUNDLE_DIR}" ] && [ -f "${BUNDLE_DIR}/SobelCustom_kirin9030.omc" ]; then
    OMC="${BUNDLE_DIR}/SobelCustom_kirin9030.omc"
  elif [ -n "${BUNDLE_DIR}" ] && [ -f "${BUNDLE_DIR}/SobelCustom.omc" ]; then
    OMC="${BUNDLE_DIR}/SobelCustom.omc"
  fi
fi

if [ "${INPUT_EXPLICIT}" -eq 0 ]; then
  if [ -n "${BUNDLE_INPUT}" ]; then
    INPUT="$(resolve_bundle_input_paths "${BUNDLE_INPUT}")"
  elif [ -n "${BUNDLE_DIR}" ] && [ -f "${BUNDLE_DIR}/x.bin" ]; then
    INPUT="${BUNDLE_DIR}/x.bin"
  fi
fi

if [ "${GOLDEN_EXPLICIT}" -eq 0 ]; then
  if [ -n "${BUNDLE_GOLDEN}" ]; then
    GOLDEN="$(resolve_bundle_path "${BUNDLE_GOLDEN}")"
  elif [ -n "${BUNDLE_DIR}" ] && [ -z "${BUNDLE_MANIFEST}" ] && [ "${OMC_EXPLICIT}" -eq 0 ] && [ "${INPUT_EXPLICIT}" -eq 0 ] && [ -f "${BUNDLE_DIR}/y.bin" ]; then
    GOLDEN="${BUNDLE_DIR}/y.bin"
  fi
fi

if [ "${OUTPUT_NAME_EXPLICIT}" -eq 0 ] && [ -n "${BUNDLE_OUTPUT_NAME}" ]; then
  OUTPUT_NAME="${BUNDLE_OUTPUT_NAME}"
fi

if [ "${OUTPUT_TYPE_EXPLICIT}" -eq 0 ] && [ -n "${BUNDLE_OUTPUT_TYPE}" ]; then
  OUTPUT_TYPE="${BUNDLE_OUTPUT_TYPE}"
fi

if [ "${TARGET_SOC_EXPLICIT}" -eq 0 ] && [ -n "${BUNDLE_TARGET_SOC}" ]; then
  TARGET_SOC="${BUNDLE_TARGET_SOC}"
fi

if [ "${COMPARE_EXPLICIT}" -eq 0 ] && [ -n "${BUNDLE_COMPARE}" ]; then
  case "$(printf '%s' "${BUNDLE_COMPARE}" | tr '[:upper:]' '[:lower:]')" in
    0|false|no|none|skip|off)
      COMPARE=0
      ;;
    1|true|yes|on)
      COMPARE=1
      ;;
    *)
      die "bundle manifest COMPARE must be 0/1/true/false, got: ${BUNDLE_COMPARE}"
      ;;
  esac
fi

if [ "${COMPARE_SCRIPT_EXPLICIT}" -eq 0 ] && [ -n "${BUNDLE_COMPARE_SCRIPT}" ]; then
  COMPARE_SCRIPT="${BUNDLE_COMPARE_SCRIPT}"
fi

resolve_compare_script

[ -n "${OMC}" ] || die "OMC not provided and not found in bundle"
[ -f "${OMC}" ] || die "OMC file not found: ${OMC}"
[ -n "${INPUT}" ] || die "input not provided and not found in bundle"

IFS=',' read -r -a INPUT_FILES <<< "${INPUT}"
for input_file in "${INPUT_FILES[@]}"; do
  input_file="$(printf '%s' "${input_file}" | trim_value)"
  [ -n "${input_file}" ] || die "empty input path in --input"
  [ -f "${input_file}" ] || die "input file not found: ${input_file}"
done

if [ "${COMPARE}" -eq 1 ]; then
  if [ -z "${GOLDEN}" ]; then
    warn "golden not available; output will be pulled but not compared"
    COMPARE=0
  elif [ ! -f "${GOLDEN}" ]; then
    warn "golden file not found; output will be pulled but not compared: ${GOLDEN}"
    COMPARE=0
  fi
fi
if [ "${COMPARE}" -eq 1 ] && [ -z "${COMPARE_SCRIPT}" ]; then
  die "COMPARE_SCRIPT not set; byte comparison is retired"
fi
if [ "${COMPARE}" -eq 1 ] && [ -n "${COMPARE_SCRIPT}" ]; then
  select_python_bin || true
fi

if [ -z "${RUN_ID}" ]; then
  RUN_ID="profile_$(date +%Y%m%d_%H%M%S)"
fi

if [ -z "${EVIDENCE_DIR}" ]; then
  EVIDENCE_DIR="${ROOT}/artifacts/profiling/${RUN_ID}"
fi
mkdir -p "${EVIDENCE_DIR}"
EVIDENCE_DIR="$(cd "${EVIDENCE_DIR}" && pwd -P)"

TARGETS_FILE="${EVIDENCE_DIR}/hdc-targets.txt"
hdc list targets | tee "${TARGETS_FILE}" >/dev/null
if [ -z "${TARGET}" ]; then
  TARGET_COUNT=0
  FIRST_TARGET=""
  while IFS= read -r line; do
    line="${line%$'\r'}"
    [ "${line}" != "[Empty]" ] || continue
    target_word="$(printf '%s\n' "${line}" | awk 'NF {print $1}')"
    [ -n "${target_word}" ] || continue
    TARGET_COUNT=$((TARGET_COUNT + 1))
    [ -n "${FIRST_TARGET}" ] || FIRST_TARGET="${target_word}"
  done < "${TARGETS_FILE}"

  if [ "${TARGET_COUNT}" -eq 0 ]; then
    die "no hdc targets found; enable HDC debugging or connect the Kirin9030 target"
  elif [ "${TARGET_COUNT}" -eq 1 ]; then
    TARGET="${FIRST_TARGET}"
    log "auto-selected hdc target: ${TARGET}"
  else
    cat "${TARGETS_FILE}" >&2
    die "multiple hdc targets found; pass --target <target-id>"
  fi
fi

REMOTE_RUN_ROOT="${DEVICE_DIR}/profile-runs"
REMOTE_INPUT_DIR="${DEVICE_DIR}/inputs/${RUN_ID}"
REMOTE_SCRIPT="${DEVICE_DIR}/target-profile-omc.sh"
REMOTE_OMC="${REMOTE_INPUT_DIR}/$(basename "${OMC}")"
REMOTE_INPUTS=""

if [ -z "${MODEL_RUN_TOOL}" ]; then
  MODEL_RUN_TOOL="${DEVICE_DIR}/model_run_tool"
fi
if [ -z "${DATA_PROC_TOOL}" ]; then
  DATA_PROC_TOOL="${DEVICE_DIR}/data_proc_tool"
fi

TARGET_RUN_LOG="${EVIDENCE_DIR}/target-script.log"
HOST_MANIFEST="${EVIDENCE_DIR}/host-manifest.env"
PUSH_LOG="${EVIDENCE_DIR}/push.log"
PULL_LOG="${EVIDENCE_DIR}/pull.log"
HILOG_CLEAR_LOG="${EVIDENCE_DIR}/hilog-clear.log"
COMPARE_LOG="${EVIDENCE_DIR}/compare.log"
TARGET_MANIFEST_LOCAL="${EVIDENCE_DIR}/manifest.env"
OUTPUT_LOCAL="${EVIDENCE_DIR}/${OUTPUT_NAME}"

for input_file in "${INPUT_FILES[@]}"; do
  input_file="$(printf '%s' "${input_file}" | trim_value)"
  append_remote_input "$(remote_basename_for_input "${input_file}")"
done

TARGET_CMD="sh $(remote_quote "${REMOTE_SCRIPT}") --omc $(remote_quote "${REMOTE_OMC}") --input $(remote_quote "${REMOTE_INPUTS}") --work-dir $(remote_quote "${REMOTE_RUN_ROOT}") --model-run-tool $(remote_quote "${MODEL_RUN_TOOL}") --data-proc-tool $(remote_quote "${DATA_PROC_TOOL}") --output-name $(remote_quote "${OUTPUT_NAME}") --times $(remote_quote "${TIMES}") --profile-mode $(remote_quote "${PROFILE_MODE}") --profile-dir $(remote_quote "${PROFILE_DIR}") --run-id $(remote_quote "${RUN_ID}")"
if [ -n "${OUTPUT_TYPE}" ]; then
  TARGET_CMD="${TARGET_CMD} --output-type $(remote_quote "${OUTPUT_TYPE}")"
fi
if [ -n "${TARGET_SOC}" ]; then
  TARGET_CMD="${TARGET_CMD} --target-soc $(remote_quote "${TARGET_SOC}")"
fi
if [ "${CHECK_SOC}" -eq 0 ]; then
  TARGET_CMD="${TARGET_CMD} --skip-soc-check"
fi
if [ -n "${PROFILING_ARG}" ]; then
  TARGET_CMD="${TARGET_CMD} --profiling-arg $(remote_quote "${PROFILING_ARG}")"
fi
if [ "${NO_DATA_PROC}" -eq 1 ]; then
  TARGET_CMD="${TARGET_CMD} --no-data-proc"
fi
if [ "${NO_ARCHIVE}" -eq 1 ]; then
  TARGET_CMD="${TARGET_CMD} --no-archive"
fi
if [ "${NO_CLEAN}" -eq 1 ]; then
  TARGET_CMD="${TARGET_CMD} --no-clean"
fi

{
  printf 'RUN_ID="%s"\n' "$(manifest_value "${RUN_ID}")"
  printf 'TARGET="%s"\n' "$(manifest_value "${TARGET}")"
  printf 'TARGET_SOC="%s"\n' "$(manifest_value "${TARGET_SOC}")"
  printf 'BUNDLE_NAME="%s"\n' "$(manifest_value "${BUNDLE_NAME}")"
  printf 'BUNDLE_DESCRIPTION="%s"\n' "$(manifest_value "${BUNDLE_DESCRIPTION}")"
  printf 'BUNDLE_TARGET_SOC="%s"\n' "$(manifest_value "${BUNDLE_TARGET_SOC}")"
  printf 'BUNDLE_DIR="%s"\n' "$(manifest_value "${BUNDLE_DIR}")"
  printf 'BUNDLE_MANIFEST="%s"\n' "$(manifest_value "${BUNDLE_MANIFEST}")"
  printf 'OMC="%s"\n' "$(manifest_value "${OMC}")"
  printf 'INPUT="%s"\n' "$(manifest_value "${INPUT}")"
  printf 'GOLDEN="%s"\n' "$(manifest_value "${GOLDEN}")"
  printf 'OUTPUT_NAME="%s"\n' "$(manifest_value "${OUTPUT_NAME}")"
  printf 'OUTPUT_TYPE="%s"\n' "$(manifest_value "${OUTPUT_TYPE}")"
  printf 'DEVICE_DIR="%s"\n' "$(manifest_value "${DEVICE_DIR}")"
  printf 'REMOTE_RUN_ROOT="%s"\n' "$(manifest_value "${REMOTE_RUN_ROOT}")"
  printf 'REMOTE_INPUT_DIR="%s"\n' "$(manifest_value "${REMOTE_INPUT_DIR}")"
  printf 'REMOTE_OMC="%s"\n' "$(manifest_value "${REMOTE_OMC}")"
  printf 'REMOTE_INPUTS="%s"\n' "$(manifest_value "${REMOTE_INPUTS}")"
  printf 'MODEL_RUN_TOOL="%s"\n' "$(manifest_value "${MODEL_RUN_TOOL}")"
  printf 'DATA_PROC_TOOL="%s"\n' "$(manifest_value "${DATA_PROC_TOOL}")"
  printf 'CHECK_SOC="%s"\n' "${CHECK_SOC}"
  printf 'COMPARE="%s"\n' "${COMPARE}"
  printf 'COMPARE_SCRIPT="%s"\n' "$(manifest_value "${COMPARE_SCRIPT}")"
  printf 'PYTHON_BIN="%s"\n' "$(manifest_value "${PYTHON_BIN}")"
  printf 'HILOG_CLEAR="%s"\n' "${HILOG_CLEAR}"
  printf 'HILOG_CLEAR_TIMEOUT="%s"\n' "${HILOG_CLEAR_TIMEOUT}"
  printf 'OMC_EXPLICIT="%s"\n' "${OMC_EXPLICIT}"
  printf 'INPUT_EXPLICIT="%s"\n' "${INPUT_EXPLICIT}"
  printf 'GOLDEN_EXPLICIT="%s"\n' "${GOLDEN_EXPLICIT}"
  printf 'OUTPUT_NAME_EXPLICIT="%s"\n' "${OUTPUT_NAME_EXPLICIT}"
  printf 'TARGET_CMD="%s"\n' "$(manifest_value "${TARGET_CMD}")"
} > "${HOST_MANIFEST}"

{
  sha256_file "${TARGET_SCRIPT}" || true
  sha256_file "${OMC}" || true
  for input_file in "${INPUT_FILES[@]}"; do
    input_file="$(printf '%s' "${input_file}" | trim_value)"
    sha256_file "${input_file}" || true
  done
  if [ -n "${GOLDEN}" ] && [ -f "${GOLDEN}" ]; then
    sha256_file "${GOLDEN}" || true
  fi
} > "${EVIDENCE_DIR}/host-inputs.sha256"

if [ "${DRY_RUN}" -eq 1 ]; then
  cat <<EOF
TARGET=${TARGET}
TARGET_SOC=${TARGET_SOC:-<auto>}
CHECK_SOC=${CHECK_SOC}
RUN_ID=${RUN_ID}
EVIDENCE_DIR=${EVIDENCE_DIR}
DEVICE_DIR=${DEVICE_DIR}
REMOTE_RUN_ROOT=${REMOTE_RUN_ROOT}
REMOTE_INPUT_DIR=${REMOTE_INPUT_DIR}
REMOTE_SCRIPT=${REMOTE_SCRIPT}
REMOTE_OMC=${REMOTE_OMC}
REMOTE_INPUTS=${REMOTE_INPUTS}
MODEL_RUN_TOOL=${MODEL_RUN_TOOL}
DATA_PROC_TOOL=${DATA_PROC_TOOL}
OMC=${OMC}
INPUT=${INPUT}
GOLDEN=${GOLDEN:-<none>}
OUTPUT_NAME=${OUTPUT_NAME}
OUTPUT_TYPE=${OUTPUT_TYPE:-<none>}
COMPARE=${COMPARE}
COMPARE_SCRIPT=${COMPARE_SCRIPT:-<none>}
PYTHON_BIN=${PYTHON_BIN:-<none>}
HILOG_CLEAR=${HILOG_CLEAR}
HILOG_CLEAR_TIMEOUT=${HILOG_CLEAR_TIMEOUT}

Commands:
  hdc -t ${TARGET} shell mkdir -p ${REMOTE_INPUT_DIR}
  hdc -t ${TARGET} file send ${TARGET_SCRIPT} ${REMOTE_SCRIPT}
  hdc -t ${TARGET} file send ${OMC} ${REMOTE_OMC}
  hdc -t ${TARGET} file send <each input> ${REMOTE_INPUT_DIR}/
  hdc -t ${TARGET} shell ${TARGET_CMD}
EOF
  exit 0
fi

log "evidence dir: ${EVIDENCE_DIR}"
log "target: ${TARGET}"
log "run id: ${RUN_ID}"
log "bundle: ${BUNDLE_DIR:-<none>}"
log "device dir: ${DEVICE_DIR}"
log "target soc: ${TARGET_SOC:-<auto>}"
log "check soc: ${CHECK_SOC}"
log "model_run_tool: ${MODEL_RUN_TOOL}"
log "data_proc_tool: ${DATA_PROC_TOOL}"

{
  echo "### mkdir"
  hdc -t "${TARGET}" shell "mkdir -p $(remote_quote "${REMOTE_INPUT_DIR}") $(remote_quote "${REMOTE_RUN_ROOT}")"
  echo
  echo "### send target script"
  hdc -t "${TARGET}" file send "${TARGET_SCRIPT}" "${REMOTE_SCRIPT}"
  echo
  echo "### chmod target script"
  hdc -t "${TARGET}" shell "chmod 755 $(remote_quote "${REMOTE_SCRIPT}")"
  echo
  echo "### send omc"
  hdc -t "${TARGET}" file send "${OMC}" "${REMOTE_OMC}"
  echo
  echo "### send inputs"
  for input_file in "${INPUT_FILES[@]}"; do
    input_file="$(printf '%s' "${input_file}" | trim_value)"
    hdc -t "${TARGET}" file send "${input_file}" "$(remote_basename_for_input "${input_file}")"
  done
} 2>&1 | tee "${PUSH_LOG}"

if [ "${HILOG_CLEAR}" -eq 1 ]; then
  log "clearing hilog with ${HILOG_CLEAR_TIMEOUT}s timeout"
  set +e
  run_with_timeout "${HILOG_CLEAR_TIMEOUT}" "${HILOG_CLEAR_LOG}" hdc -t "${TARGET}" shell "hilog -r"
  HILOG_CLEAR_STATUS=$?
  set -e
  if [ "${HILOG_CLEAR_STATUS}" -eq 124 ]; then
    warn "hdc shell hilog -r timed out after ${HILOG_CLEAR_TIMEOUT}s; continuing without clearing logs"
  elif [ "${HILOG_CLEAR_STATUS}" -ne 0 ]; then
    warn "hdc shell hilog -r failed with status ${HILOG_CLEAR_STATUS}; continuing without clearing logs"
  fi
else
  printf 'skipped by --no-clear-logs\n' > "${HILOG_CLEAR_LOG}"
fi

log "running target-side profiling collector"
set +e
hdc -t "${TARGET}" shell "${TARGET_CMD}" 2>&1 | tee "${TARGET_RUN_LOG}"
TARGET_STATUS=${PIPESTATUS[0]}
set -e

REMOTE_RUN_DIR="${REMOTE_RUN_ROOT}/${RUN_ID}"
REMOTE_ARCHIVE="${REMOTE_RUN_ROOT}/${RUN_ID}.tgz"

{
  echo "### pull target manifest"
  hdc -t "${TARGET}" file recv "${REMOTE_RUN_DIR}/manifest.env" "${TARGET_MANIFEST_LOCAL}" || true
} > "${PULL_LOG}" 2>&1

if [ -f "${TARGET_MANIFEST_LOCAL}" ]; then
  parsed_archive="$(find_archive_path_from_manifest "${TARGET_MANIFEST_LOCAL}" || true)"
  if [ -n "${parsed_archive}" ]; then
    REMOTE_ARCHIVE="${parsed_archive}"
  fi
fi

log "pulling profiling archive"
set +e
hdc -t "${TARGET}" file recv "${REMOTE_ARCHIVE}" "${EVIDENCE_DIR}/" >> "${PULL_LOG}" 2>&1
PULL_ARCHIVE_STATUS=$?
set -e
if [ "${PULL_ARCHIVE_STATUS}" -ne 0 ]; then
  warn "archive pull failed; pulling run directory instead"
  hdc -t "${TARGET}" file recv "${REMOTE_RUN_DIR}" "${EVIDENCE_DIR}/target-run" >> "${PULL_LOG}" 2>&1 || true
fi

REMOTE_OUTPUT="${REMOTE_RUN_DIR}/${OUTPUT_NAME}"
if [ -f "${TARGET_MANIFEST_LOCAL}" ]; then
  parsed_output="$(find_output_path_from_manifest "${TARGET_MANIFEST_LOCAL}" || true)"
  if [ -n "${parsed_output}" ]; then
    REMOTE_OUTPUT="${parsed_output}"
  fi
fi

log "pulling model output"
set +e
hdc -t "${TARGET}" file recv "${REMOTE_OUTPUT}" "${OUTPUT_LOCAL}" >> "${PULL_LOG}" 2>&1
PULL_OUTPUT_STATUS=$?
set -e

COMPARE_RESULT="SKIPPED"
if [ "${COMPARE}" -eq 1 ] && [ "${PULL_OUTPUT_STATUS}" -eq 0 ] && [ -s "${OUTPUT_LOCAL}" ]; then
  log "checking output with Python precision validator: ${COMPARE_SCRIPT:-<missing>}"
  set +e
  run_python_compare
  COMPARE_STATUS=$?
  set -e

  if [ "${COMPARE_STATUS}" -eq 0 ]; then
    COMPARE_RESULT="PASS"
    log "golden compare passed"
  else
    COMPARE_RESULT="FAIL"
    warn "golden compare failed; inspect ${COMPARE_LOG}"
  fi
elif [ "${COMPARE}" -eq 1 ]; then
  COMPARE_RESULT="FAIL"
  {
    echo "compare_script=${COMPARE_SCRIPT:-}"
    echo "python_bin=${PYTHON_BIN:-}"
    echo "status=127"
    echo "reason=output pull failed or output is empty; Python precision validation could not run"
  } > "${COMPARE_LOG}"
  warn "output pull failed or output is empty; compare cannot run"
fi

RESULT="NOT_CONFIRMED"
if [ "${TARGET_STATUS}" -eq 0 ] && [ "${PULL_OUTPUT_STATUS}" -eq 0 ] && [ -s "${OUTPUT_LOCAL}" ]; then
  if [ "${COMPARE_RESULT}" = "PASS" ]; then
    RESULT="PASS"
  elif [ "${COMPARE}" -eq 0 ]; then
    RESULT="PASS_OUTPUT_PULLED_NO_COMPARE"
  else
    RESULT="OUTPUT_PULLED_NO_COMPARE"
  fi
fi

{
  echo "target_status=${TARGET_STATUS}"
  echo "remote_run_dir=${REMOTE_RUN_DIR}"
  echo "remote_archive=${REMOTE_ARCHIVE}"
  echo "pull_archive_status=${PULL_ARCHIVE_STATUS}"
  echo "pull_output_status=${PULL_OUTPUT_STATUS}"
  echo "output_local=${OUTPUT_LOCAL}"
  echo "check_soc=${CHECK_SOC}"
  echo "compare=${COMPARE}"
  echo "compare_script=${COMPARE_SCRIPT:-}"
  echo "python_bin=${PYTHON_BIN:-}"
  echo "compare_result=${COMPARE_RESULT}"
  echo "result=${RESULT}"
} > "${EVIDENCE_DIR}/summary.txt"

log "target status: ${TARGET_STATUS}"
log "output pull status: ${PULL_OUTPUT_STATUS}"
log "compare result: ${COMPARE_RESULT}"
log "result: ${RESULT}"
log "host evidence: ${EVIDENCE_DIR}"

if [ "${TARGET_STATUS}" -ne 0 ]; then
  die "target-side profiling collector failed; inspect ${TARGET_RUN_LOG}"
fi
if [ "${COMPARE_RESULT}" = "FAIL" ]; then
  die "output comparison failed; inspect ${COMPARE_LOG}"
fi

if [ "${RESULT}" = "PASS" ]; then
  log_success "profiling run completed and output passed golden comparison."
elif [ "${RESULT}" = "PASS_OUTPUT_PULLED_NO_COMPARE" ]; then
  log "PASS_OUTPUT_PULLED_NO_COMPARE: profiling run completed and output was pulled; no golden compare was requested."
fi

exit 0
