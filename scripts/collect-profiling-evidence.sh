#!/usr/bin/env bash
# Build a copy/paste kernel-performance report from a profiling evidence directory.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

RUN_DIR=""
RUN_ID=""
OUTPUT=""
REPORT_MODE="kernel"
MAX_FILE_LINES=80
MAX_LIST_LINES=80

usage() {
  cat <<'USAGE'
usage: scripts/collect-profiling-evidence.sh [options]

Collects a copy/paste profiling report from artifacts/profiling/.
By default it selects the newest artifacts/profiling/profile_* directory and
prints a high-signal kernel-performance report.

Options:
  --run-dir DIR       Specific profiling evidence directory.
  --run-id ID         Run id under artifacts/profiling/, e.g. profile_20260805_161905.
  --output PATH       Also write the report to PATH.
  --kernel            Kernel-performance report. Default.
  --performance       Alias for --kernel.
  --brief             Compact diagnostics report with key profiling/accuracy evidence.
  --full              Full report with every known host/target/archive section.
  --max-file-lines N  Max lines per text file section. Default: 80.
  --max-list-lines N  Max lines for file/archive listing sections. Default: 80.
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

emit_file_grep_section() {
  local title="$1"
  local path="$2"
  local pattern="$3"
  local max_lines="${4:-${MAX_FILE_LINES}}"

  section "${title}"
  if [ ! -f "${path}" ]; then
    printf 'missing: %s\n' "${path}"
    return 0
  fi

  grep -Ei "${pattern}" "${path}" 2>/dev/null | sed -n "1,${max_lines}p" || printf 'no matching lines\n'
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

emit_target_file_content() {
  local basename="$1"
  local archive="$2"
  local direct_path="${RUN_DIR}/target-run/${basename}"
  local member=""

  if [ -f "${direct_path}" ]; then
    cat "${direct_path}"
    return 0
  fi

  if [ -n "${archive}" ]; then
    member="$(find_archive_member_by_basename "${archive}" "${basename}")"
    if [ -n "${member}" ]; then
      archive_extract "${archive}" "${member}" 2>/dev/null
      return 0
    fi
  fi

  return 1
}

emit_target_file_section() {
  local title="$1"
  local basename="$2"
  local archive="$3"

  section "${title}"
  if ! emit_target_file_content "${basename}" "${archive}" >/dev/null 2>&1; then
    printf 'missing: target-run/%s\n' "${basename}"
    return 0
  fi

  { emit_target_file_content "${basename}" "${archive}" 2>/dev/null || true; } | sed -n "1,${MAX_FILE_LINES}p"
}

emit_target_grep_section() {
  local title="$1"
  local basename="$2"
  local archive="$3"
  local pattern="$4"
  local max_lines="${5:-${MAX_FILE_LINES}}"

  section "${title}"
  if ! emit_target_file_content "${basename}" "${archive}" >/dev/null 2>&1; then
    printf 'missing: target-run/%s\n' "${basename}"
    return 0
  fi

  { emit_target_file_content "${basename}" "${archive}" 2>/dev/null || true; } | grep -Ei "${pattern}" | sed -n "1,${max_lines}p" || printf 'no matching lines\n'
}

kv_get() {
  local path="$1"
  local key="$2"

  [ -f "${path}" ] || return 0
  awk -F= -v key="${key}" '
    $1 == key {
      value = substr($0, index($0, "=") + 1)
    }
    END {
      if (value != "") {
        print value
      }
    }
  ' "${path}" 2>/dev/null || true
}

target_grep() {
  local basename="$1"
  local archive="$2"
  local pattern="$3"
  local max_lines="${4:-${MAX_FILE_LINES}}"

  { emit_target_file_content "${basename}" "${archive}" 2>/dev/null || true; } | grep -Ei "${pattern}" | sed -n "1,${max_lines}p" || true
}

target_first_line() {
  local basename="$1"
  local archive="$2"
  local pattern="$3"

  target_grep "${basename}" "${archive}" "${pattern}" 1
}

target_count() {
  local basename="$1"
  local archive="$2"
  local pattern="$3"

  { emit_target_file_content "${basename}" "${archive}" 2>/dev/null || true; } | grep -Eci "${pattern}" || true
}

archive_matching_members() {
  local archive="$1"
  local pattern="$2"
  local max_lines="${3:-${MAX_LIST_LINES}}"

  [ -n "${archive}" ] || return 0
  { archive_list "${archive}" 2>/dev/null || true; } | grep -Ei "${pattern}" | sed -n "1,${max_lines}p" || true
}

profile_artifact_members() {
  local archive="$1"

  {
    archive_matching_members "${archive}" '(^|/)(csv|prof_data|profiling|msprof|trace|timeline|kernel|task)(/|[^/]*$)|\.(csv|json|trace)$' "${MAX_LIST_LINES}"
    if [ -d "${RUN_DIR}/target-run" ]; then
      find "${RUN_DIR}/target-run" -maxdepth 5 -type f 2>/dev/null | grep -Ei '(^|/)(csv|prof_data|profiling|msprof|trace|timeline|kernel|task)(/|[^/]*$)|\.(csv|json|trace)$' || true
    fi
  } | { grep -Ev '/$' || true; } | { grep -Eiv 'profile-candidates|model_run_tool|data_proc_tool|profiling-evidence-report|manifest|target-info|files-before|files-after|hashes|command\.txt|output_0' || true; } | sed '/^$/d' | sort -u | sed -n "1,${MAX_LIST_LINES}p"
}

first_csv_member() {
  local archive="$1"

  [ -n "${archive}" ] || return 0
  archive_matching_members "${archive}" '\.csv$|(^|/)csv/' 1
}

emit_value() {
  local key="$1"
  local value="${2:-}"

  if [ -n "${value}" ]; then
    printf '%s=%s\n' "${key}" "${value}"
  else
    printf '%s=<empty>\n' "${key}"
  fi
}

emit_kernel_csv_preview() {
  local archive="$1"
  local csv_member=""

  csv_member="$(first_csv_member "${archive}")"
  [ -n "${csv_member}" ] || return 0

  section "kernel csv preview"
  printf 'source=%s\n' "${csv_member}"
  archive_extract "${archive}" "${csv_member}" 2>/dev/null | sed -n "1,${MAX_FILE_LINES}p" || true
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
  local max_lines="${4:-${MAX_LIST_LINES}}"

  section "${title}"
  if [ -d "${path}" ]; then
    find "${path}" -maxdepth "${maxdepth}" -type f 2>/dev/null | sort | sed -n "1,${max_lines}p"
  else
    printf 'missing: %s\n' "${path}"
  fi
}

emit_archive_listing() {
  local archive="$1"
  local max_lines="${2:-${MAX_LIST_LINES}}"

  section "archive listing"
  if [ -z "${archive}" ]; then
    printf 'missing: no .tgz/.tar in %s\n' "${RUN_DIR}"
    return 0
  fi

  printf '%s\n' "--- ${archive}"
  { archive_list "${archive}" 2>/dev/null || true; } | sed -n "1,${max_lines}p"
}

emit_archive_grep_listing() {
  local archive="$1"
  local pattern="$2"
  local max_lines="${3:-${MAX_LIST_LINES}}"

  section "archive profiling listing"
  if [ -z "${archive}" ]; then
    printf 'missing: no .tgz/.tar in %s\n' "${RUN_DIR}"
    return 0
  fi

  printf '%s\n' "--- ${archive}"
  { archive_list "${archive}" 2>/dev/null || true; } | grep -Ei "${pattern}" | sed -n "1,${max_lines}p" || printf 'no matching lines\n'
}

emit_report_header() {
  local archive="$1"

  if [ "${REPORT_MODE}" = "kernel" ]; then
    printf '# Kirin kernel performance report\n'
  else
    printf '# Kirin profiling evidence report\n'
  fi
  printf 'generated_at=%s\n' "$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  if [ "${REPORT_MODE}" != "kernel" ]; then
    printf 'repo_root=%s\n' "${ROOT}"
  fi
  printf 'run_dir=%s\n' "${RUN_DIR}"
  printf 'archive=%s\n' "${archive:-<none>}"
  printf 'mode=%s\n' "${REPORT_MODE}"
}

emit_kernel_report() {
  local archive="$1"
  local summary_path="${RUN_DIR}/summary.txt"
  local manifest_path="${RUN_DIR}/manifest.env"
  local host_manifest_path="${RUN_DIR}/host-manifest.env"
  local result=""
  local compare_result=""
  local run_status=""
  local data_proc_status=""
  local data_proc_result_path=""
  local profile_mode=""
  local profiling_arg=""
  local add_times=""
  local times=""
  local target=""
  local model_run_tool=""
  local data_proc_tool=""
  local output_type=""
  local target_soc=""
  local check_soc=""
  local profile_artifacts=""
  local profile_candidates=""
  local inference_success_count=""
  local profiling_enabled_count=""
  local profiling_disabled_count=""
  local profiling_config=""
  local data_proc_probe=""
  local verdict="UNAVAILABLE"
  local reason=""

  result="$(kv_get "${summary_path}" "result")"
  compare_result="$(kv_get "${summary_path}" "compare_result")"
  run_status="$(kv_get "${manifest_path}" "RUN_STATUS")"
  data_proc_status="$(kv_get "${manifest_path}" "DATA_PROC_STATUS")"
  data_proc_result_path="$(kv_get "${manifest_path}" "DATA_PROC_RESULT_PATH")"
  profile_mode="$(kv_get "${manifest_path}" "PROFILE_MODE")"
  profiling_arg="$(kv_get "${manifest_path}" "PROFILING_ARG")"
  add_times="$(kv_get "${manifest_path}" "ADD_TIMES")"
  times="$(kv_get "${manifest_path}" "TIMES")"
  target="$(kv_get "${host_manifest_path}" "TARGET")"
  model_run_tool="$(kv_get "${host_manifest_path}" "MODEL_RUN_TOOL")"
  data_proc_tool="$(kv_get "${host_manifest_path}" "DATA_PROC_TOOL")"
  output_type="$(kv_get "${host_manifest_path}" "OUTPUT_TYPE")"
  target_soc="$(kv_get "${host_manifest_path}" "TARGET_SOC")"
  check_soc="$(kv_get "${host_manifest_path}" "CHECK_SOC")"
  profile_artifacts="$(profile_artifact_members "${archive}")"
  profile_candidates="$(emit_target_file_content "profile-candidates.txt" "${archive}" 2>/dev/null || true)"
  inference_success_count="$(target_count "model_run_tool-run.log" "${archive}" 'Inference: running model succeeded')"
  profiling_enabled_count="$(target_count "model_run_tool-run.log" "${archive}" 'Profiling is enabled')"
  profiling_disabled_count="$(target_count "model_run_tool-run.log" "${archive}" 'Profiling is disabled')"
  profiling_config="$(target_first_line "model_run_tool-run.log" "${archive}" 'Profiling mode configStr')"
  profiling_config="${profiling_config#*configStr: }"
  data_proc_probe="$(target_first_line "data_proc_tool-help.txt" "${archive}" 'ERROR|WARN|usage|help|profile|csv|result|path')"

  if [ -n "${profile_artifacts}" ]; then
    verdict="AVAILABLE"
    reason="candidate profiling artifact files are present; inspect the listed CSV/profile files"
  elif [ "${data_proc_status}" = "NO_PROFILE_DIR" ]; then
    reason="data_proc_tool was not run because no raw profiling directory was found"
  elif [ -z "${data_proc_status}" ]; then
    reason="manifest has no DATA_PROC_STATUS and no CSV/profile artifacts were found"
  else
    reason="DATA_PROC_STATUS=${data_proc_status}, but no CSV/profile artifacts were found"
  fi

  emit_report_header "${archive}"

  section "kernel performance verdict"
  emit_value "kernel_performance_data" "${verdict}"
  emit_value "reason" "${reason}"
  emit_value "result" "${result}"
  emit_value "compare_result" "${compare_result}"
  emit_value "run_status" "${run_status}"
  emit_value "data_proc_status" "${data_proc_status}"
  emit_value "data_proc_result_path" "${data_proc_result_path}"

  section "run setup"
  emit_value "target" "${target}"
  emit_value "target_soc" "${target_soc}"
  emit_value "check_soc" "${check_soc}"
  emit_value "times" "${times}"
  emit_value "profile_mode" "${profile_mode}"
  emit_value "profiling_arg" "${profiling_arg}"
  emit_value "add_times" "${add_times}"
  emit_value "output_type" "${output_type}"
  emit_value "model_run_tool" "${model_run_tool}"
  emit_value "data_proc_tool" "${data_proc_tool}"

  section "kernel timing data"
  if [ -n "${profile_artifacts}" ]; then
    printf '%s\n' "${profile_artifacts}"
  else
    printf 'kernel_elapsed_time=<unavailable>\n'
    printf 'operator_or_kernel_csv=<none>\n'
    printf 'No per-kernel elapsed-time CSV/profile artifact was found in this run.\n'
    printf 'The model_run_tool lines named "time: N" are counted as iteration numbers here, not kernel elapsed time.\n'
  fi

  if [ -n "${profile_candidates}" ]; then
    section "profile candidates"
    printf '%s\n' "${profile_candidates}" | sed -n "1,${MAX_FILE_LINES}p"
  else
    section "profile candidates"
    printf 'empty\n'
  fi

  section "runner evidence"
  emit_value "model_run_tool_inference_success_count" "${inference_success_count}"
  if [ "${profiling_enabled_count}" != "0" ]; then
    printf 'model_run_tool_profiling_enabled=yes\n'
  else
    printf 'model_run_tool_profiling_enabled=no\n'
  fi
  if [ "${profiling_disabled_count}" != "0" ]; then
    printf 'model_run_tool_profiling_disabled=yes\n'
  else
    printf 'model_run_tool_profiling_disabled=no\n'
  fi
  emit_value "profiling_config" "${profiling_config}"

  section "data_proc evidence"
  if emit_target_file_content "data_proc_tool-run.log" "${archive}" >/dev/null 2>&1; then
    target_grep "data_proc_tool-run.log" "${archive}" 'ERROR|WARN|profil|profile|csv|result|path|elapsed|kernel|task' "${MAX_FILE_LINES}"
  else
    printf 'data_proc_tool-run.log=missing\n'
  fi
  emit_value "data_proc_tool_probe" "${data_proc_probe}"

  emit_kernel_csv_preview "${archive}"

  section "next action"
  if [ "${verdict}" = "AVAILABLE" ]; then
    printf 'Analyze the CSV/profile artifacts listed above for operator/kernel elapsed time.\n'
  else
    printf 'This run only proves inference and accuracy, not kernel performance. Capture the raw profiling directory first; then data_proc_tool can produce the kernel timing report.\n'
  fi
}

emit_brief_report() {
  local archive="$1"

  emit_report_header "${archive}"

  emit_key_fields
  emit_file_section "summary.txt" "${RUN_DIR}/summary.txt"
  emit_file_grep_section "manifest profiling fields" "${RUN_DIR}/manifest.env" '^(RUN_STATUS|DATA_PROC_STATUS|DATA_PROC_RESULT_PATH|PROFILE_MODE|PROFILING_ARG|ADD_TIMES|PROFILE_CANDIDATES_FILE|ARCHIVE_PATH|OUTPUT_PATH)='
  emit_file_grep_section "target-script status/warnings" "${RUN_DIR}/target-script.log" 'WARN|ERROR|status|run directory|archive|profil|profile|data_proc|model_run_tool'
  emit_file_grep_section "pull status" "${RUN_DIR}/pull.log" 'error|fail|manifest|archive|output|FileTransfer|Size:'
  emit_file_grep_section "compare decision" "${RUN_DIR}/compare.log" 'compare_script=|decision=|best_candidate|max_abs|mean_abs|nonzero|dump_size|PASS|FAIL'
  emit_target_file_section "target-run/command.txt" "command.txt" "${archive}"
  emit_target_grep_section "target-run/model_run_tool-help profiling lines" "model_run_tool-help.txt" "${archive}" 'usage|option|help|profil|profile|trace|dump|time|times|model|input|output'
  emit_target_grep_section "target-run/data_proc_tool-help profiling lines" "data_proc_tool-help.txt" "${archive}" 'usage|option|help|profil|profile|result|output|path|csv'
  emit_target_file_section "target-run/profile-candidates.txt" "profile-candidates.txt" "${archive}"
  emit_target_grep_section "target-run/files-after profiling lines" "files-after.txt" "${archive}" 'prof|profile|csv|output_0|\.json|\.csv|\.bin|\.txt'
  emit_find_section "evidence files" "${RUN_DIR}" 3
  emit_archive_grep_listing "${archive}" 'prof|profile|csv|json|bin|output|command|model_run|data_proc'
}

emit_full_report() {
  local archive="$1"

  emit_report_header "${archive}"

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

emit_report() {
  local archive="$1"

  case "${REPORT_MODE}" in
    kernel)
      emit_kernel_report "${archive}"
      ;;
    brief)
      emit_brief_report "${archive}"
      ;;
    full)
      emit_full_report "${archive}"
      ;;
    *)
      die "unknown report mode: ${REPORT_MODE}"
      ;;
  esac
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
    --kernel|--performance)
      REPORT_MODE="kernel"
      shift
      ;;
    --brief)
      REPORT_MODE="brief"
      shift
      ;;
    --full)
      REPORT_MODE="full"
      shift
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
