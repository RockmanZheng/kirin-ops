#!/usr/bin/env bash
# Build a copy/paste diagnostics report from a profiling evidence directory.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

RUN_DIR=""
RUN_ID=""
OUTPUT=""
MAX_FILE_LINES=420
MAX_LIST_LINES=420

usage() {
  cat <<'USAGE'
usage: scripts/collect-profiling-evidence.sh [options]

Collects a copy/paste profiling diagnostics report from artifacts/profiling/.
By default it selects the newest artifacts/profiling/profile_* directory.

Options:
  --run-dir DIR       Specific profiling evidence directory.
  --run-id ID         Run id under artifacts/profiling/, e.g. profile_20260805_161905.
  --output PATH       Also write the report to PATH.
  --max-file-lines N  Max lines per text file section. Default: 420.
  --max-list-lines N  Max lines for file/archive listing sections. Default: 420.
  -h, --help          Show this help.

Example:
  scripts/collect-profiling-evidence.sh

  scripts/collect-profiling-evidence.sh \
    --run-id profile_20260805_161905 \
    --output artifacts/profiling/profile_20260805_161905/profiling-evidence-report.txt
USAGE
}

die() {
  printf '[collect-profiling-evidence] ERROR: %s\n' "$*" >&2
  exit 1
}

is_uint() {
  case "$1" in
    ''|0|*[!0-9]*)
      return 1
      ;;
  esac
}

abs_existing_dir() {
  local path="$1"

  [ -d "${path}" ] || die "directory not found: ${path}"
  (cd "${path}" && pwd -P)
}

select_latest_run_dir() {
  local profiling_root="${ROOT}/artifacts/profiling"
  local -a candidates=()

  [ -d "${profiling_root}" ] || die "profiling root not found: ${profiling_root}"
  while IFS= read -r candidate; do
    candidates+=("${candidate}")
  done < <(find "${profiling_root}" -maxdepth 1 -type d -name 'profile_*' -print 2>/dev/null)

  [ "${#candidates[@]}" -gt 0 ] || die "no artifacts/profiling/profile_* run directory found"
  printf '%s\n' "${candidates[@]}" | sort | tail -1
}

section() {
  printf '\n### %s\n' "$1"
}

emit_file_section() {
  local title="$1"
  local path="$2"
  local max_lines="${3:-${MAX_FILE_LINES}}"

  section "${title}"
  if [ -f "${path}" ]; then
    sed -n "1,${max_lines}p" "${path}" || true
  else
    printf 'missing: %s\n' "${path}"
  fi
}

find_archive() {
  local -a archives=()

  while IFS= read -r archive; do
    archives+=("${archive}")
  done < <(find "${RUN_DIR}" -maxdepth 1 -type f \( -name '*.tgz' -o -name '*.tar' \) -print 2>/dev/null)

  [ "${#archives[@]}" -gt 0 ] || return 0
  printf '%s\n' "${archives[@]}" | sort | tail -1
}

archive_list() {
  local archive="$1"

  case "${archive}" in
    *.tgz)
      tar -tzf "${archive}"
      ;;
    *.tar)
      tar -tf "${archive}"
      ;;
  esac
}

archive_extract() {
  local archive="$1"
  local member="$2"

  case "${archive}" in
    *.tgz)
      tar -xOzf "${archive}" "${member}"
      ;;
    *.tar)
      tar -xOf "${archive}" "${member}"
      ;;
  esac
}

find_archive_member_by_basename() {
  local archive="$1"
  local basename="$2"

  archive_list "${archive}" 2>/dev/null | awk -v basename="${basename}" '
    $0 == basename { print; exit }
    substr($0, length($0) - length(basename)) == "/" basename { print; exit }
  '
}

emit_target_file_section() {
  local title="$1"
  local basename="$2"
  local archive="$3"
  local direct_path="${RUN_DIR}/target-run/${basename}"
  local member=""

  section "${title}"
  if [ -f "${direct_path}" ]; then
    sed -n "1,${MAX_FILE_LINES}p" "${direct_path}" || true
    return 0
  fi

  if [ -n "${archive}" ]; then
    member="$(find_archive_member_by_basename "${archive}" "${basename}")"
    if [ -n "${member}" ]; then
      { archive_extract "${archive}" "${member}" 2>/dev/null || true; } | sed -n "1,${MAX_FILE_LINES}p"
      return 0
    fi
  fi

  printf 'missing: target-run/%s\n' "${basename}"
}

emit_key_fields() {
  section "key fields"
  {
    grep -E '^(target_status|pull_archive_status|pull_output_status|compare|compare_result|result)=' "${RUN_DIR}/summary.txt" 2>/dev/null || true
    grep -E '^(RUN_STATUS|DATA_PROC_STATUS|DATA_PROC_RESULT_PATH|PROFILE_MODE|PROFILING_ARG|ADD_TIMES|PROFILE_CANDIDATES_FILE|ARCHIVE_PATH|OUTPUT_PATH)=' "${RUN_DIR}/manifest.env" 2>/dev/null || true
  } | sed '/^$/d'
}

emit_find_section() {
  local title="$1"
  local path="$2"
  local maxdepth="$3"

  section "${title}"
  if [ -d "${path}" ]; then
    find "${path}" -maxdepth "${maxdepth}" -type f 2>/dev/null | sort | sed -n "1,${MAX_LIST_LINES}p"
  else
    printf 'missing: %s\n' "${path}"
  fi
}

emit_archive_listing() {
  local archive="$1"

  section "archive listing"
  if [ -z "${archive}" ]; then
    printf 'missing: no .tgz/.tar in %s\n' "${RUN_DIR}"
    return 0
  fi

  printf '%s\n' "--- ${archive}"
  { archive_list "${archive}" 2>/dev/null || true; } | sed -n "1,${MAX_LIST_LINES}p"
}

emit_report() {
  local archive="$1"

  printf '# Kirin profiling evidence report\n'
  printf 'generated_at=%s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  printf 'repo_root=%s\n' "${ROOT}"
  printf 'run_dir=%s\n' "${RUN_DIR}"
  printf 'archive=%s\n' "${archive:-<none>}"

  emit_key_fields
  emit_file_section "summary.txt" "${RUN_DIR}/summary.txt"
  emit_file_section "manifest.env" "${RUN_DIR}/manifest.env"
  emit_file_section "host-manifest.env" "${RUN_DIR}/host-manifest.env"
  emit_file_section "target-script.log" "${RUN_DIR}/target-script.log"
  emit_file_section "pull.log" "${RUN_DIR}/pull.log"
  emit_file_section "compare.log" "${RUN_DIR}/compare.log"
  emit_target_file_section "target-run/command.txt" "command.txt" "${archive}"
  emit_target_file_section "target-run/model_run_tool-help.txt" "model_run_tool-help.txt" "${archive}"
  emit_target_file_section "target-run/model_run_tool-run.log" "model_run_tool-run.log" "${archive}"
  emit_target_file_section "target-run/data_proc_tool-help.txt" "data_proc_tool-help.txt" "${archive}"
  emit_target_file_section "target-run/data_proc_tool-run.log" "data_proc_tool-run.log" "${archive}"
  emit_target_file_section "target-run/profile-candidates.txt" "profile-candidates.txt" "${archive}"
  emit_target_file_section "target-run/files-before.txt" "files-before.txt" "${archive}"
  emit_target_file_section "target-run/files-after.txt" "files-after.txt" "${archive}"
  emit_find_section "target-run files" "${RUN_DIR}/target-run" 4
  emit_find_section "evidence files" "${RUN_DIR}" 4
  emit_archive_listing "${archive}"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --run-dir)
      [ "$#" -ge 2 ] || die "--run-dir requires a directory"
      RUN_DIR="$2"
      shift 2
      ;;
    --run-id)
      [ "$#" -ge 2 ] || die "--run-id requires a run id"
      RUN_ID="$2"
      shift 2
      ;;
    --output)
      [ "$#" -ge 2 ] || die "--output requires a path"
      OUTPUT="$2"
      shift 2
      ;;
    --max-file-lines)
      [ "$#" -ge 2 ] || die "--max-file-lines requires an integer"
      is_uint "$2" || die "--max-file-lines must be a positive integer"
      MAX_FILE_LINES="$2"
      shift 2
      ;;
    --max-list-lines)
      [ "$#" -ge 2 ] || die "--max-list-lines requires an integer"
      is_uint "$2" || die "--max-list-lines must be a positive integer"
      MAX_LIST_LINES="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "unknown option: $1"
      ;;
  esac
done

[ -z "${RUN_DIR}" ] || [ -z "${RUN_ID}" ] || die "use only one of --run-dir or --run-id"

if [ -n "${RUN_ID}" ]; then
  RUN_DIR="${ROOT}/artifacts/profiling/${RUN_ID}"
elif [ -z "${RUN_DIR}" ]; then
  RUN_DIR="$(select_latest_run_dir)"
fi

RUN_DIR="$(abs_existing_dir "${RUN_DIR}")"
ARCHIVE="$(find_archive)"

if [ -n "${OUTPUT}" ]; then
  mkdir -p "$(dirname "${OUTPUT}")"
  emit_report "${ARCHIVE}" | tee "${OUTPUT}"
else
  emit_report "${ARCHIVE}"
fi
