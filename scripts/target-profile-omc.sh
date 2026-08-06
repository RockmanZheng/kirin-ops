#!/system/bin/sh
# Collect CANNKit model_run_tool profiling data on a Kirin/HarmonyOS target.
#
# This script is intended to run on the target device, not on the host.
# Host-side examples assume hdc is already in PATH:
#
#   hdc file send scripts/target-profile-omc.sh /data/local/tmp/z84378291/target-profile-omc.sh
#   hdc shell "sh /data/local/tmp/z84378291/target-profile-omc.sh \
#     --omc /data/local/tmp/z84378291/SobelCustom.omc \
#     --input /data/local/tmp/z84378291/x.bin \
#     --work-dir /data/local/tmp/z84378291/profile-runs \
#     --model-run-tool /data/local/tmp/z84378291/model_run_tool \
#     --data-proc-tool /data/local/tmp/z84378291/data_proc_tool"
#
# The resulting run directory or archive can then be pulled from the host.

set -u

SCRIPT_NAME="kirin-target-profile"

MODEL_RUN_TOOL="${MODEL_RUN_TOOL:-/data/local/tmp/model_run_tool}"
DATA_PROC_TOOL="${DATA_PROC_TOOL:-/data/local/tmp/data_proc_tool}"
WORK_DIR="${WORK_DIR:-/data/local/tmp/kirin-profile-runs}"
OMC="${OMC:-}"
INPUT="${INPUT:-}"
OUTPUT_NAME="${OUTPUT_NAME:-output_0}"
OUTPUT_TYPE="${OUTPUT_TYPE:-}"
TARGET_SOC="${TARGET_SOC:-}"
TIMES="${TIMES:-50}"
PROFILE_MODE="${PROFILE_MODE:-auto}"
PROFILING_ARG="${PROFILING_ARG:-}"
PROFILE_DIR_HINT="${PROFILE_DIR_HINT:-prof_data}"
EXTRA_RUN_ARGS="${EXTRA_RUN_ARGS:-}"
RUN_ID="${RUN_ID:-}"
CLEAN=1
RUN_DATA_PROC=1
ARCHIVE=1
CHECK_SOC=1
DRY_RUN=0

log() {
  printf '[%s] %s\n' "${SCRIPT_NAME}" "$*"
}

warn() {
  printf '[%s] WARN: %s\n' "${SCRIPT_NAME}" "$*" >&2
}

die() {
  printf '[%s] ERROR: %s\n' "${SCRIPT_NAME}" "$*" >&2
  exit 1
}

usage() {
  cat <<'USAGE'
usage: sh target-profile-omc.sh --omc PATH --input PATHS [options]

Runs a CANN/CANNKit .omc with model_run_tool on the target device and collects
profiling artifacts into a durable run directory.

Required:
  --omc PATH              Target-side .omc path.
  --input PATHS           Target-side input .bin path, or comma-separated paths.

Options:
  --work-dir DIR          Target-side root for profiling runs.
                          Default: /data/local/tmp/kirin-profile-runs
  --model-run-tool PATH   Target-side model_run_tool path.
                          Default: /data/local/tmp/model_run_tool
  --data-proc-tool PATH   Target-side data_proc_tool path.
                          Default: /data/local/tmp/data_proc_tool
  --output-name NAME      Output file expected from model_run_tool.
                          Default: output_0
  --output-type TYPE      Explicit model_run_tool output tensor dtype, for
                          example UINT8. Passed as --output_type=TYPE.
  --target-soc SOC        Expected target SoC, e.g. Kirin9020 or Kirin9030.
                          Fails if target params expose a different Kirin SoC.
  --skip-soc-check        Do not assert the target SoC before profiling.
                          Use only when intentionally running despite unknown
                          or mismatched SoC metadata.
  --times N               Profiling inference count when supported.
                          Default: 50
  --profile-mode MODE     auto, none, or arg. Default: auto.
                          auto: detect common profiling flags from --help.
                          arg: require --profiling-arg.
                          none: do not add profiling-specific flags.
  --profiling-arg ARG     Explicit model_run_tool profiling flag, for example
                          --enable_item=1, --profiling, or --profile.
  --extra-run-arg ARG     Extra single-word model_run_tool argument.
                          Can be passed multiple times.
  --profile-dir NAME      Expected raw profile data directory name.
                          Default: prof_data
  --run-id ID             Run id/directory name. Default: date-based.
  --no-clean              Do not remove stale output/profile files in run dir.
  --no-data-proc          Do not run data_proc_tool.
  --no-archive            Do not create a .tgz/.tar archive.
  --dry-run               Print resolved command and exit.
  -h, --help              Show this help.

Host-side example, assuming model/input/tool files are already on the target:
  hdc file send scripts/target-profile-omc.sh /data/local/tmp/z84378291/target-profile-omc.sh
  hdc shell "sh /data/local/tmp/z84378291/target-profile-omc.sh \
    --omc /data/local/tmp/z84378291/SobelCustom.omc \
    --input /data/local/tmp/z84378291/x.bin \
    --work-dir /data/local/tmp/z84378291/profile-runs \
    --model-run-tool /data/local/tmp/z84378291/model_run_tool \
    --data-proc-tool /data/local/tmp/z84378291/data_proc_tool \
    --target-soc Kirin9030 \
    --times 50"

Host-side pull example for the run above:
  hdc file recv /data/local/tmp/z84378291/profile-runs/<run-id>.tgz artifacts/profiling/<run-id>.tgz

Notes:
  This script intentionally probes the target model_run_tool --help because
  profiling flags have varied across CANNKit debug tool builds.
USAGE
}

safe_date() {
  date '+%Y%m%d_%H%M%S' 2>/dev/null || date 2>/dev/null | tr ' :/' '___'
}

abs_path() {
  case "$1" in
    /*)
      printf '%s\n' "$1"
      ;;
    *)
      printf '%s/%s\n' "$(pwd)" "$1"
      ;;
  esac
}

command_exists() {
  command -v "$1" >/dev/null 2>&1
}

normalize_soc_token() {
  compact="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]' | tr -d ' _-')"
  case "$compact" in
    *kirinx[0-9]*)
      printf '%s\n' "$compact" | sed 's/.*\(kirinx[0-9][0-9]*\).*/\1/'
      ;;
    *kirin[0-9]*)
      printf '%s\n' "$compact" | sed 's/.*\(kirin[0-9][0-9]*\).*/\1/'
      ;;
    *)
      return 1
      ;;
  esac
}

extract_soc_versions_from_file() {
  awk '
    {
      line = tolower($0)
      gsub(/[ _-]/, "", line)
      while (match(line, /kirinx?[0-9][0-9]*/)) {
        token = substr(line, RSTART, RLENGTH)
        print token
        line = substr(line, RSTART + RLENGTH)
      }
    }
  ' "$1" | awk 'NF && !seen[$0]++'
}

soc_sets_intersect() {
  left_values="$1"
  right_values="$2"

  for left in $left_values; do
    for right in $right_values; do
      if [ "$left" = "$right" ]; then
        return 0
      fi
    done
  done
  return 1
}

file_size() {
  if command_exists stat; then
    stat -c '%s' "$1" 2>/dev/null || stat -f '%z' "$1" 2>/dev/null || wc -c < "$1"
  else
    wc -c < "$1"
  fi
}

hash_file_if_possible() {
  file="$1"
  if command_exists sha256sum; then
    sha256sum "$file" 2>/dev/null || true
  elif command_exists md5sum; then
    md5sum "$file" 2>/dev/null || true
  else
    printf 'hash_unavailable  %s\n' "$file"
  fi
}

contains_text() {
  pattern="$1"
  file="$2"
  grep -i "$pattern" "$file" >/dev/null 2>&1
}

shell_join_command() {
  for arg in "$@"; do
    case "$arg" in
      *"'"*)
        printf " '%s'" "$(printf '%s' "$arg" | sed "s/'/'\\\\''/g")"
        ;;
      *)
        printf " '%s'" "$arg"
        ;;
    esac
  done
  printf '\n'
}

probe_tool() {
  tool="$1"
  log_file="$2"

  {
    echo "### path"
    ls -l "$tool" 2>&1 || true
    echo
    echo "### version"
    "$tool" --version 2>&1 || true
    echo
    echo "### help"
    "$tool" --help 2>&1 || true
  } > "$log_file" 2>&1
}

detect_profile_arg() {
  help_file="$1"

  if [ -n "${PROFILING_ARG}" ]; then
    printf '%s\n' "${PROFILING_ARG}"
    return 0
  fi

  case "${PROFILE_MODE}" in
    none)
      printf '\n'
      return 0
      ;;
    arg)
      die "--profile-mode arg requires --profiling-arg"
      ;;
    auto)
      ;;
    *)
      die "--profile-mode must be auto, none, or arg"
      ;;
  esac

  if contains_text '\-\-enable_item' "$help_file"; then
    printf '%s\n' '--enable_item=1'
  elif contains_text '\-\-profiling' "$help_file"; then
    printf '%s\n' '--profiling'
  elif contains_text '\-\-profile' "$help_file"; then
    printf '%s\n' '--profile'
  elif contains_text '\-\-enable_profiling' "$help_file"; then
    printf '%s\n' '--enable_profiling'
  else
    printf '\n'
  fi
}

detect_times_supported() {
  help_file="$1"

  if contains_text '\-\-times' "$help_file"; then
    return 0
  fi
  return 1
}

write_target_info() {
  out="$1"
  {
    echo "### identity"
    id 2>&1 || true
    whoami 2>&1 || true
    pwd 2>&1 || true
    echo
    echo "### kernel"
    uname -a 2>&1 || true
    echo
    echo "### selected params"
    for key in \
      const.product.model \
      const.product.name \
      const.product.device \
      const.product.board \
      const.product.hardwareversion \
      const.product.software.version \
      const.ohos.apiversion \
      ohos.boot.chiptype \
      const.product.soc \
      const.product.socversion \
      ro.product.model \
      ro.product.name \
      ro.product.device \
      ro.product.board \
      ro.board.platform \
      ro.hardware \
      ro.soc.model \
      ro.build.version.incremental
    do
      printf '%s=' "$key"
      param get "$key" 2>&1 || true
    done
    echo
    echo "### storage"
    df -h /data /data/local/tmp 2>&1 || df /data /data/local/tmp 2>&1 || true
    echo
    echo "### process scan"
    { ps -ef 2>/dev/null || ps -A 2>/dev/null || true; } | grep -i 'hiai\|nn\|npu\|model\|neural' | head -120 || true
  } > "$out" 2>&1
}

find_profile_candidates() {
  {
    for path in \
      "$RUN_DIR/$PROFILE_DIR_HINT" \
      "$RUN_DIR/prof_data" \
      "$RUN_DIR/profile" \
      "$RUN_DIR/profiling" \
      "$RUN_DIR/PROF" \
      "$RUN_DIR"/PROF_* \
      "$RUN_DIR"/prof*
    do
      [ -d "$path" ] || continue
      printf '%s\n' "$path"
    done
    profile_search_roots | while IFS= read -r root; do
      [ -n "$root" ] || continue
      [ -d "$root" ] || continue
      find "$root" -maxdepth 4 -type d \( \
        -name "$PROFILE_DIR_HINT" \
        -o -name prof_data \
        -o -name profile \
        -o -name profile_data \
        -o -name profiling \
        -o -name profiling_data \
        -o -name 'PROF*' \
        -o -name 'msprof*' \
      \) -print 2>/dev/null || true
    done
  } | awk 'NF && !seen[$0]++'
}

profile_search_roots() {
  {
    printf '%s\n' "$RUN_DIR"
    printf '%s\n' "$WORK_DIR"
    dirname "$RUN_DIR" 2>/dev/null || true
    dirname "$MODEL_RUN_TOOL" 2>/dev/null || true
    dirname "$OMC" 2>/dev/null || true
    printf '%s\n' '/data/local/tmp'
  } | awk 'NF && !seen[$0]++'
}

write_new_profile_candidates() {
  before_file="$1"
  after_file="$2"
  out_file="$3"

  awk '
    FILENAME == ARGV[1] {
      before[$0] = 1
      next
    }
    NF && !before[$0] && !seen[$0]++ {
      print
    }
  ' "$before_file" "$after_file" > "$out_file" 2>/dev/null || : > "$out_file"
}

write_profile_candidate_snapshot() {
  candidates_file="$1"
  out_file="$2"

  {
    if [ -s "$candidates_file" ]; then
      while IFS= read -r candidate_dir; do
        [ -n "$candidate_dir" ] || continue
        [ -d "$candidate_dir" ] || continue

        echo "###PROFILE_CANDIDATE_BEGIN $candidate_dir"
        ls -ld "$candidate_dir" 2>&1 || true
        if command_exists find; then
          find "$candidate_dir" -maxdepth 5 -print 2>/dev/null | sort | sed -n '1,360p' | while IFS= read -r entry; do
            [ -n "$entry" ] || continue
            ls -ld "$entry" 2>&1 || true
            if [ -f "$entry" ]; then
              printf 'size='
              file_size "$entry" 2>/dev/null || true
            fi
          done
        else
          # shellcheck disable=SC2012
          ls -lR "$candidate_dir" 2>&1 | sed -n '1,360p' || true
        fi
        echo "###PROFILE_CANDIDATE_END $candidate_dir"
        echo
      done < "$candidates_file"
    fi
  } > "$out_file" 2>&1
}

extract_profile_candidate_snapshot() {
  candidate_dir="$1"
  snapshot_file="$2"

  awk -v candidate_dir="$candidate_dir" '
    $0 == "###PROFILE_CANDIDATE_BEGIN " candidate_dir {
      emit = 1
      next
    }
    $0 == "###PROFILE_CANDIDATE_END " candidate_dir {
      emit = 0
      exit
    }
    emit {
      print
    }
  ' "$snapshot_file" 2>/dev/null || true
}

files_same() {
  left="$1"
  right="$2"

  if command_exists cmp; then
    cmp -s "$left" "$right"
    return $?
  fi
  if command_exists diff; then
    diff "$left" "$right" >/dev/null 2>&1
    return $?
  fi
  if command_exists cksum; then
    [ "$(cksum < "$left" 2>/dev/null || true)" = "$(cksum < "$right" 2>/dev/null || true)" ]
    return $?
  fi

  return 1
}

write_changed_profile_candidates() {
  before_candidates_file="$1"
  after_candidates_file="$2"
  before_snapshot_file="$3"
  after_snapshot_file="$4"
  out_file="$5"
  before_one="$RUN_DIR/.candidate-before.snapshot"
  after_one="$RUN_DIR/.candidate-after.snapshot"

  : > "$out_file"
  [ -s "$after_candidates_file" ] || return 0

  while IFS= read -r candidate_dir; do
    [ -n "$candidate_dir" ] || continue
    [ -d "$candidate_dir" ] || continue
    grep -Fx "$candidate_dir" "$before_candidates_file" >/dev/null 2>&1 || continue

    extract_profile_candidate_snapshot "$candidate_dir" "$before_snapshot_file" > "$before_one"
    extract_profile_candidate_snapshot "$candidate_dir" "$after_snapshot_file" > "$after_one"
    [ -s "$after_one" ] || continue

    if ! files_same "$before_one" "$after_one"; then
      printf '%s\n' "$candidate_dir" >> "$out_file"
    fi
  done < "$after_candidates_file"

  rm -f "$before_one" "$after_one" 2>/dev/null || true
  awk 'NF && !seen[$0]++' "$out_file" > "$out_file.tmp" 2>/dev/null && mv "$out_file.tmp" "$out_file"
}

merge_profile_candidates() {
  out_file="$1"
  shift

  : > "$out_file"
  for candidate_file in "$@"; do
    [ -f "$candidate_file" ] || continue
    cat "$candidate_file"
  done | awk 'NF && !seen[$0]++' > "$out_file"
}

safe_artifact_name() {
  printf '%s\n' "$1" | sed 's#^/##; s#[^A-Za-z0-9._-]#_#g'
}

copy_profile_artifacts() {
  candidates_file="$1"
  dest_root="$2"
  log_file="$3"

  : > "$log_file"
  [ -s "$candidates_file" ] || {
    echo "no selected profile candidates" >> "$log_file"
    return 0
  }

  mkdir -p "$dest_root" 2>> "$log_file" || {
    echo "failed to create artifact root: $dest_root" >> "$log_file"
    return 0
  }

  while IFS= read -r candidate_dir; do
    [ -n "$candidate_dir" ] || continue
    [ -d "$candidate_dir" ] || {
      echo "missing candidate dir: $candidate_dir" >> "$log_file"
      continue
    }

    case "$candidate_dir/" in
      "$RUN_DIR"/*)
        echo "already inside run dir: $candidate_dir" >> "$log_file"
        continue
        ;;
    esac

    dest="$dest_root/$(safe_artifact_name "$candidate_dir")"
    rm -rf "$dest" 2>> "$log_file" || true
    if cp -R "$candidate_dir" "$dest" >> "$log_file" 2>&1; then
      echo "copied: $candidate_dir -> $dest" >> "$log_file"
      continue
    fi

    echo "recursive copy failed; copying common profile files individually: $candidate_dir" >> "$log_file"
    mkdir -p "$dest" 2>> "$log_file" || continue
    find "$candidate_dir" -maxdepth 5 -type f \( \
      -name '*.csv' \
      -o -name '*.prof' \
      -o -name '*.json' \
      -o -name '*.trace' \
      -o -name '*.txt' \
      -o -name '*kernel*' \
      -o -name '*task*' \
      -o -name '*pmu*' \
      -o -name '*model*' \
      -o -name '*op*' \
    \) -print 2>/dev/null | while IFS= read -r profile_file; do
      [ -f "$profile_file" ] || continue
      target_file="$dest/$(safe_artifact_name "$profile_file")"
      cp "$profile_file" "$target_file" >> "$log_file" 2>&1 && echo "copied_file: $profile_file -> $target_file" >> "$log_file"
    done
  done < "$candidates_file"
}

write_profile_search_log() {
  {
    echo "### search roots"
    profile_search_roots
    echo
    echo "### candidate dirs before model_run_tool"
    if [ -s "$PROFILE_CANDIDATES_BEFORE" ]; then
      sed -n '1,80p' "$PROFILE_CANDIDATES_BEFORE"
    else
      echo "empty"
    fi
    echo
    echo "### candidate dirs after model_run_tool"
    if [ -s "$PROFILE_CANDIDATES_ALL" ]; then
      sed -n '1,80p' "$PROFILE_CANDIDATES_ALL"
    else
      echo "empty"
    fi
    echo
    echo "### new candidate dirs"
    if [ -s "$PROFILE_CANDIDATES_NEW" ]; then
      sed -n '1,80p' "$PROFILE_CANDIDATES_NEW"
    else
      echo "empty"
    fi
    echo
    echo "### changed/reused candidate dirs"
    if [ -s "$PROFILE_CANDIDATES_CHANGED" ]; then
      sed -n '1,80p' "$PROFILE_CANDIDATES_CHANGED"
    else
      echo "empty"
    fi
    echo
    echo "### selected candidate dirs for data_proc_tool"
    if [ -s "$PROFILE_CANDIDATES" ]; then
      sed -n '1,80p' "$PROFILE_CANDIDATES"
    else
      echo "empty"
    fi
    echo
    echo "### latest candidate snapshot"
    if [ -s "$PROFILE_CANDIDATE_SNAPSHOT_AFTER" ]; then
      sed -n '1,120p' "$PROFILE_CANDIDATE_SNAPSHOT_AFTER"
    else
      echo "empty"
    fi
  } > "$PROFILE_SEARCH_LOG" 2>&1
}

run_data_proc_tool() {
  result_path="$1"
  status=0
  output_path_probe_log="$RUN_DIR/data_proc_tool-output-path-probe.log"

  : > "$DATA_PROC_LOG"
  if contains_text '\-\-output_path' "$DATA_PROC_HELP"; then
    {
      echo "### data_proc_tool with help-declared output_path"
      echo "result_path=$result_path"
      echo "output_path=$RUN_DIR/csv"
    } >> "$DATA_PROC_LOG"
    (
      cd "$RUN_DIR" || exit 1
      "$DATA_PROC_TOOL" "--result_path=$result_path" "--output_path=$RUN_DIR/csv"
    ) >> "$DATA_PROC_LOG" 2>&1
    return $?
  fi

  {
    echo "### data_proc_tool output_path compatibility probe"
    echo "result_path=$result_path"
    echo "output_path=$RUN_DIR/csv"
    echo "note=optional probe; unsupported --output_path is expected on some target data_proc_tool builds"
  } >> "$DATA_PROC_LOG"
  (
    cd "$RUN_DIR" || exit 1
    "$DATA_PROC_TOOL" "--result_path=$result_path" "--output_path=$RUN_DIR/csv"
  ) > "$output_path_probe_log" 2>&1
  status=$?
  if [ "$status" -eq 0 ]; then
    echo "output_path_probe_status=SUPPORTED" >> "$DATA_PROC_LOG"
    sed -n '1,120p' "$output_path_probe_log" >> "$DATA_PROC_LOG"
    return 0
  fi
  if grep -Ei 'another instance|run later' "$output_path_probe_log" >/dev/null 2>&1; then
    echo "output_path_probe_status=FAILED_BUSY_OR_LOCKED" >> "$DATA_PROC_LOG"
    sed -n '1,120p' "$output_path_probe_log" >> "$DATA_PROC_LOG"
    return "$status"
  fi
  if grep -Ei 'unknown command line flag.*output_path|unknown.*output_path|invalid.*output_path|unrecognized.*output_path' "$output_path_probe_log" >/dev/null 2>&1; then
    echo "output_path_probe_status=UNSUPPORTED_BY_TOOL" >> "$DATA_PROC_LOG"
    echo "output_path_probe_action=fall back to --result_path only" >> "$DATA_PROC_LOG"
  else
    echo "output_path_probe_status=FAILED_STATUS_$status" >> "$DATA_PROC_LOG"
    sed -n '1,120p' "$output_path_probe_log" | sed 's/^/output_path_probe_output: /' >> "$DATA_PROC_LOG"
  fi

  {
    echo
    echo "### fallback: data_proc_tool without output_path"
    echo "result_path=$result_path"
    echo "cwd=$RUN_DIR"
  } >> "$DATA_PROC_LOG"
  (
    cd "$RUN_DIR" || exit 1
    "$DATA_PROC_TOOL" "--result_path=$result_path"
  ) >> "$DATA_PROC_LOG" 2>&1
  return $?
}

archive_run_dir() {
  archive_path=""
  run_base="$(basename "$RUN_DIR")"
  run_parent="$(dirname "$RUN_DIR")"
  archive_log="$run_parent/$run_base.archive.log"

  if ! command_exists tar; then
    warn "tar is unavailable on target; leaving run directory unarchived"
    return 0
  fi

  archive_path="$run_parent/$run_base.tgz"
  if (cd "$run_parent" && tar -czf "$archive_path" "$run_base") > "$archive_log" 2>&1; then
    printf '%s\n' "$archive_path"
    return 0
  fi

  archive_path="$run_parent/$run_base.tar"
  if (cd "$run_parent" && tar -cf "$archive_path" "$run_base") >> "$archive_log" 2>&1; then
    printf '%s\n' "$archive_path"
    return 0
  fi

  warn "failed to archive run directory; inspect $archive_log"
  return 0
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --omc)
      [ "$#" -ge 2 ] || die "--omc requires a path"
      OMC="$2"
      shift 2
      ;;
    --input)
      [ "$#" -ge 2 ] || die "--input requires a path or comma-separated paths"
      INPUT="$2"
      shift 2
      ;;
    --work-dir)
      [ "$#" -ge 2 ] || die "--work-dir requires a directory"
      WORK_DIR="$2"
      shift 2
      ;;
    --model-run-tool)
      [ "$#" -ge 2 ] || die "--model-run-tool requires a path"
      MODEL_RUN_TOOL="$2"
      shift 2
      ;;
    --data-proc-tool)
      [ "$#" -ge 2 ] || die "--data-proc-tool requires a path"
      DATA_PROC_TOOL="$2"
      shift 2
      ;;
    --output-name)
      [ "$#" -ge 2 ] || die "--output-name requires a value"
      OUTPUT_NAME="$2"
      shift 2
      ;;
    --output-type)
      [ "$#" -ge 2 ] || die "--output-type requires a value"
      OUTPUT_TYPE="$2"
      shift 2
      ;;
    --target-soc)
      [ "$#" -ge 2 ] || die "--target-soc requires a SoC value"
      TARGET_SOC="$2"
      shift 2
      ;;
    --skip-soc-check)
      CHECK_SOC=0
      shift
      ;;
    --times)
      [ "$#" -ge 2 ] || die "--times requires a value"
      TIMES="$2"
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
    --extra-run-arg)
      [ "$#" -ge 2 ] || die "--extra-run-arg requires a value"
      EXTRA_RUN_ARGS="${EXTRA_RUN_ARGS}
$2"
      shift 2
      ;;
    --profile-dir)
      [ "$#" -ge 2 ] || die "--profile-dir requires a name"
      PROFILE_DIR_HINT="$2"
      shift 2
      ;;
    --run-id)
      [ "$#" -ge 2 ] || die "--run-id requires a value"
      RUN_ID="$2"
      shift 2
      ;;
    --no-clean)
      CLEAN=0
      shift
      ;;
    --no-data-proc)
      RUN_DATA_PROC=0
      shift
      ;;
    --no-archive)
      ARCHIVE=0
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

[ -n "$OMC" ] || die "--omc is required"
[ -n "$INPUT" ] || die "--input is required"

case "$TIMES" in
  ''|*[!0-9]*)
    die "--times must be an integer"
    ;;
esac

OMC="$(abs_path "$OMC")"
WORK_DIR="$(abs_path "$WORK_DIR")"
MODEL_RUN_TOOL="$(abs_path "$MODEL_RUN_TOOL")"
DATA_PROC_TOOL="$(abs_path "$DATA_PROC_TOOL")"

[ -f "$OMC" ] || die "OMC file not found: $OMC"
[ -f "$MODEL_RUN_TOOL" ] || die "model_run_tool not found: $MODEL_RUN_TOOL"
[ -x "$MODEL_RUN_TOOL" ] || chmod 755 "$MODEL_RUN_TOOL" 2>/dev/null || true
[ -x "$MODEL_RUN_TOOL" ] || die "model_run_tool is not executable: $MODEL_RUN_TOOL"

if [ -z "$RUN_ID" ]; then
  RUN_ID="$(safe_date)"
fi

RUN_DIR="$WORK_DIR/$RUN_ID"
mkdir -p "$RUN_DIR" || die "failed to create run directory: $RUN_DIR"

TARGET_INFO="$RUN_DIR/target-info.txt"
RUNNER_HELP="$RUN_DIR/model_run_tool-help.txt"
DATA_PROC_HELP="$RUN_DIR/data_proc_tool-help.txt"
RUN_LOG="$RUN_DIR/model_run_tool-run.log"
DATA_PROC_LOG="$RUN_DIR/data_proc_tool-run.log"
COMMAND_FILE="$RUN_DIR/command.txt"
MANIFEST="$RUN_DIR/manifest.env"
FILES_BEFORE="$RUN_DIR/files-before.txt"
FILES_AFTER="$RUN_DIR/files-after.txt"
HASHES="$RUN_DIR/hashes.txt"
PROFILE_CANDIDATES_BEFORE="$RUN_DIR/profile-candidates-before.txt"
PROFILE_CANDIDATES_ALL="$RUN_DIR/profile-candidates-all.txt"
PROFILE_CANDIDATES_NEW="$RUN_DIR/profile-candidates-new.txt"
PROFILE_CANDIDATES_CHANGED="$RUN_DIR/profile-candidates-changed.txt"
PROFILE_CANDIDATES="$RUN_DIR/profile-candidates.txt"
PROFILE_CANDIDATE_SNAPSHOT_BEFORE="$RUN_DIR/profile-candidate-snapshot-before.txt"
PROFILE_CANDIDATE_SNAPSHOT_AFTER="$RUN_DIR/profile-candidate-snapshot-after.txt"
PROFILE_ARTIFACTS_DIR="$RUN_DIR/profile-artifacts"
PROFILE_ARTIFACTS_LOG="$RUN_DIR/profile-artifacts.txt"
PROFILE_SEARCH_LOG="$RUN_DIR/profile-search.txt"
INPUT_LIST="$RUN_DIR/input-list.txt"
INPUT_ABS_LIST="$RUN_DIR/input-absolute-list.txt"
SOC_CHECK_LOG="$RUN_DIR/soc-check.log"

write_target_info "$TARGET_INFO"

TARGET_SOC_NORMALIZED=""
DEVICE_SOC_VERSIONS=""
SOC_CHECK_RESULT="SKIPPED"
if [ "$CHECK_SOC" -eq 1 ] && [ -n "$TARGET_SOC" ]; then
  TARGET_SOC_NORMALIZED="$(normalize_soc_token "$TARGET_SOC")" || die "--target-soc must look like a Kirin target, for example Kirin9020 or Kirin9030"
  DEVICE_SOC_VERSIONS="$(extract_soc_versions_from_file "$TARGET_INFO" || true)"
  {
    echo "check_soc=$CHECK_SOC"
    echo "target_soc=$TARGET_SOC"
    echo "target_soc_normalized=$TARGET_SOC_NORMALIZED"
    echo "device_soc_versions=$DEVICE_SOC_VERSIONS"
  } > "$SOC_CHECK_LOG"
  if [ -n "$DEVICE_SOC_VERSIONS" ]; then
    if soc_sets_intersect "$TARGET_SOC_NORMALIZED" "$DEVICE_SOC_VERSIONS"; then
      SOC_CHECK_RESULT="PASS"
      echo "result=$SOC_CHECK_RESULT" >> "$SOC_CHECK_LOG"
    else
      SOC_CHECK_RESULT="MISMATCH"
      echo "result=$SOC_CHECK_RESULT" >> "$SOC_CHECK_LOG"
      die "target SoC assertion does not match device params: target=$TARGET_SOC_NORMALIZED device=$DEVICE_SOC_VERSIONS"
    fi
  else
    SOC_CHECK_RESULT="UNKNOWN_DEVICE_SOC"
    echo "result=$SOC_CHECK_RESULT" >> "$SOC_CHECK_LOG"
    warn "target SoC assertion was provided, but device params did not expose a Kirin SoC"
  fi
else
  DEVICE_SOC_VERSIONS="$(extract_soc_versions_from_file "$TARGET_INFO" || true)"
  {
    echo "check_soc=$CHECK_SOC"
    echo "target_soc=$TARGET_SOC"
    echo "target_soc_normalized=$TARGET_SOC_NORMALIZED"
    echo "device_soc_versions=$DEVICE_SOC_VERSIONS"
    echo "result=$SOC_CHECK_RESULT"
  } > "$SOC_CHECK_LOG"
fi

probe_tool "$MODEL_RUN_TOOL" "$RUNNER_HELP"

if [ -f "$DATA_PROC_TOOL" ]; then
  [ -x "$DATA_PROC_TOOL" ] || chmod 755 "$DATA_PROC_TOOL" 2>/dev/null || true
  probe_tool "$DATA_PROC_TOOL" "$DATA_PROC_HELP"
else
  echo "data_proc_tool not found: $DATA_PROC_TOOL" > "$DATA_PROC_HELP"
fi

DETECTED_PROFILING_ARG="$(detect_profile_arg "$RUNNER_HELP")" || exit 1
ADD_TIMES=0
if detect_times_supported "$RUNNER_HELP"; then
  ADD_TIMES=1
else
  warn "model_run_tool --help does not mention --times; running without --times"
fi

if [ "$PROFILE_MODE" = "auto" ] && [ -z "$DETECTED_PROFILING_ARG" ]; then
  warn "no known profiling switch found in model_run_tool --help; relying on tool defaults plus --times when supported"
fi

INPUT_ABS=""
printf '%s\n' "$INPUT" | tr ',' '\n' > "$INPUT_LIST"
: > "$INPUT_ABS_LIST"
while IFS= read -r input_path; do
  [ -n "$input_path" ] || continue
  input_path="$(abs_path "$input_path")"
  [ -f "$input_path" ] || die "input file not found: $input_path"
  printf '%s\n' "$input_path" >> "$INPUT_ABS_LIST"
  if [ -z "$INPUT_ABS" ]; then
    INPUT_ABS="$input_path"
  else
    INPUT_ABS="$INPUT_ABS,$input_path"
  fi
done < "$INPUT_LIST"
[ -n "$INPUT_ABS" ] || die "--input resolved to no files"

if [ "$CLEAN" -eq 1 ]; then
  rm -f "$RUN_DIR/$OUTPUT_NAME" 2>/dev/null || true
  rm -rf "${RUN_DIR:?}/$PROFILE_DIR_HINT" "${RUN_DIR:?}/prof_data" "${RUN_DIR:?}/profile" "${RUN_DIR:?}/profiling" "${RUN_DIR:?}"/PROF_* 2>/dev/null || true
fi

find_profile_candidates > "$PROFILE_CANDIDATES_BEFORE"
write_profile_candidate_snapshot "$PROFILE_CANDIDATES_BEFORE" "$PROFILE_CANDIDATE_SNAPSHOT_BEFORE"

{
  echo "### run dir before model_run_tool"
  ls -la "$RUN_DIR" 2>&1 || true
  echo
  echo "### model and inputs"
  ls -l "$OMC" 2>&1 || true
  while IFS= read -r input_path; do
    ls -l "$input_path" 2>&1 || true
  done < "$INPUT_ABS_LIST"
} > "$FILES_BEFORE"

set -- "$MODEL_RUN_TOOL" "--model=$OMC" "--input=$INPUT_ABS" "--output_dir=$RUN_DIR/"
if [ -n "$OUTPUT_TYPE" ]; then
  set -- "$@" "--output_type=$OUTPUT_TYPE"
fi
if [ -n "$DETECTED_PROFILING_ARG" ]; then
  set -- "$@" "$DETECTED_PROFILING_ARG"
fi
if [ "$ADD_TIMES" -eq 1 ]; then
  set -- "$@" "--times=$TIMES"
fi
if [ -n "$EXTRA_RUN_ARGS" ]; then
  printf '%s\n' "$EXTRA_RUN_ARGS" | while IFS= read -r extra_arg; do
    [ -n "$extra_arg" ] || continue
    printf '%s\n' "$extra_arg" >> "$RUN_DIR/extra-run-args.applied"
  done
  for extra_arg in $EXTRA_RUN_ARGS; do
    [ -n "$extra_arg" ] || continue
    set -- "$@" "$extra_arg"
  done
fi

{
  echo "run_dir=$RUN_DIR"
  echo "cwd=$RUN_DIR"
  echo "model_run_tool=$MODEL_RUN_TOOL"
  echo "data_proc_tool=$DATA_PROC_TOOL"
  echo "omc=$OMC"
  echo "input=$INPUT_ABS"
  echo "output_dir=$RUN_DIR/"
  echo "output_name=$OUTPUT_NAME"
  echo "output_type=$OUTPUT_TYPE"
  echo "check_soc=$CHECK_SOC"
  echo "target_soc=$TARGET_SOC"
  echo "target_soc_normalized=$TARGET_SOC_NORMALIZED"
  echo "soc_check_result=$SOC_CHECK_RESULT"
  echo "times=$TIMES"
  echo "profile_mode=$PROFILE_MODE"
  echo "profiling_arg=$DETECTED_PROFILING_ARG"
  echo "add_times=$ADD_TIMES"
  echo
  echo "command:"
  shell_join_command "$@"
} > "$COMMAND_FILE"

if [ "$DRY_RUN" -eq 1 ]; then
  cat "$COMMAND_FILE"
  exit 0
fi

log "run directory: $RUN_DIR"
log "running model_run_tool"
(
  cd "$RUN_DIR" || exit 1
  "$@"
) > "$RUN_LOG" 2>&1
RUN_STATUS=$?

{
  echo "### run dir after model_run_tool"
  ls -la "$RUN_DIR" 2>&1 || true
  echo
  echo "### newest files"
  # ls sorting is the most portable concise view available on HarmonyOS shells.
  # shellcheck disable=SC2012
  ls -lt "$RUN_DIR" 2>&1 | head -80 || true
} > "$FILES_AFTER"

: > "$HASHES"
if [ -f "$RUN_DIR/$OUTPUT_NAME" ]; then
  {
    printf 'output_size_bytes='
    file_size "$RUN_DIR/$OUTPUT_NAME"
    hash_file_if_possible "$RUN_DIR/$OUTPUT_NAME"
  } >> "$HASHES"
fi
hash_file_if_possible "$OMC" >> "$HASHES"
while IFS= read -r input_path; do
  hash_file_if_possible "$input_path" >> "$HASHES"
done < "$INPUT_ABS_LIST"

find_profile_candidates > "$PROFILE_CANDIDATES_ALL"
write_profile_candidate_snapshot "$PROFILE_CANDIDATES_ALL" "$PROFILE_CANDIDATE_SNAPSHOT_AFTER"
write_new_profile_candidates "$PROFILE_CANDIDATES_BEFORE" "$PROFILE_CANDIDATES_ALL" "$PROFILE_CANDIDATES_NEW"
write_changed_profile_candidates "$PROFILE_CANDIDATES_BEFORE" "$PROFILE_CANDIDATES_ALL" "$PROFILE_CANDIDATE_SNAPSHOT_BEFORE" "$PROFILE_CANDIDATE_SNAPSHOT_AFTER" "$PROFILE_CANDIDATES_CHANGED"
merge_profile_candidates "$PROFILE_CANDIDATES" "$PROFILE_CANDIDATES_NEW" "$PROFILE_CANDIDATES_CHANGED"
write_profile_search_log

DATA_PROC_STATUS="SKIPPED"
DATA_PROC_RESULT_PATH=""
if [ "$RUN_DATA_PROC" -eq 1 ]; then
  if [ ! -f "$DATA_PROC_TOOL" ]; then
    warn "data_proc_tool not found; raw profiling data, if generated, remains in $RUN_DIR"
    DATA_PROC_STATUS="MISSING_TOOL"
  elif [ ! -x "$DATA_PROC_TOOL" ]; then
    warn "data_proc_tool is not executable; raw profiling data, if generated, remains in $RUN_DIR"
    DATA_PROC_STATUS="NOT_EXECUTABLE"
  elif [ ! -s "$PROFILE_CANDIDATES" ]; then
    warn "no new or changed profile candidate directory found before data_proc_tool"
    DATA_PROC_STATUS="NO_PROFILE_DIR"
  else
    DATA_PROC_RESULT_PATH="$(sed -n '1p' "$PROFILE_CANDIDATES")"
    log "running data_proc_tool on $DATA_PROC_RESULT_PATH"
    run_data_proc_tool "$DATA_PROC_RESULT_PATH"
    DATA_PROC_STATUS=$?
  fi
fi

copy_profile_artifacts "$PROFILE_CANDIDATES" "$PROFILE_ARTIFACTS_DIR" "$PROFILE_ARTIFACTS_LOG"
find_profile_candidates > "$PROFILE_CANDIDATES_ALL"
write_profile_candidate_snapshot "$PROFILE_CANDIDATES_ALL" "$PROFILE_CANDIDATE_SNAPSHOT_AFTER"
write_profile_search_log

write_manifest() {
  {
    echo "RUN_ID=$RUN_ID"
    echo "RUN_DIR=$RUN_DIR"
    echo "ARCHIVE_PATH=$ARCHIVE_PATH"
    echo "MODEL_RUN_TOOL=$MODEL_RUN_TOOL"
    echo "DATA_PROC_TOOL=$DATA_PROC_TOOL"
    echo "OMC=$OMC"
    echo "INPUT=$INPUT_ABS"
    echo "OUTPUT_NAME=$OUTPUT_NAME"
    echo "OUTPUT_TYPE=$OUTPUT_TYPE"
    echo "CHECK_SOC=$CHECK_SOC"
    echo "TARGET_SOC=$TARGET_SOC"
    echo "TARGET_SOC_NORMALIZED=$TARGET_SOC_NORMALIZED"
    echo "SOC_CHECK_RESULT=$SOC_CHECK_RESULT"
    echo "TIMES=$TIMES"
    echo "PROFILE_MODE=$PROFILE_MODE"
    echo "PROFILING_ARG=$DETECTED_PROFILING_ARG"
    echo "ADD_TIMES=$ADD_TIMES"
    echo "RUN_STATUS=$RUN_STATUS"
    echo "DATA_PROC_STATUS=$DATA_PROC_STATUS"
    echo "DATA_PROC_RESULT_PATH=$DATA_PROC_RESULT_PATH"
    echo "OUTPUT_PATH=$RUN_DIR/$OUTPUT_NAME"
    echo "PROFILE_CANDIDATES_FILE=$PROFILE_CANDIDATES"
    echo "PROFILE_CANDIDATES_ALL_FILE=$PROFILE_CANDIDATES_ALL"
    echo "PROFILE_CANDIDATES_NEW_FILE=$PROFILE_CANDIDATES_NEW"
    echo "PROFILE_CANDIDATES_CHANGED_FILE=$PROFILE_CANDIDATES_CHANGED"
    echo "PROFILE_CANDIDATE_SNAPSHOT_BEFORE_FILE=$PROFILE_CANDIDATE_SNAPSHOT_BEFORE"
    echo "PROFILE_CANDIDATE_SNAPSHOT_AFTER_FILE=$PROFILE_CANDIDATE_SNAPSHOT_AFTER"
    echo "PROFILE_ARTIFACTS_DIR=$PROFILE_ARTIFACTS_DIR"
    echo "PROFILE_ARTIFACTS_LOG=$PROFILE_ARTIFACTS_LOG"
    echo "PROFILE_SEARCH_FILE=$PROFILE_SEARCH_LOG"
  } > "$MANIFEST"
}

ARCHIVE_PATH=""
write_manifest
if [ "$ARCHIVE" -eq 1 ]; then
  ARCHIVE_PATH="$(archive_run_dir)"
  write_manifest
  if [ -n "$ARCHIVE_PATH" ]; then
    archive_run_dir >/dev/null
  fi
fi

log "model_run_tool status: $RUN_STATUS"
log "data_proc_tool status: $DATA_PROC_STATUS"
if [ -n "$ARCHIVE_PATH" ]; then
  log "archive: $ARCHIVE_PATH"
else
  log "run directory ready: $RUN_DIR"
fi

exit "$RUN_STATUS"
