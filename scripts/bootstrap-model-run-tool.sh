#!/usr/bin/env bash
# Compatibility wrapper for copying model_run_tool between HarmonyOS targets.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SOURCE_TARGET="${SOURCE_TARGET:-}"
DEST_TARGET="${DEST_TARGET:-}"
SOURCE_PATH="${SOURCE_PATH:-/data/local/tmp/model_run_tool}"
DEST_PATH="${DEST_PATH:-/data/local/tmp/z84378291/model_run_tool}"
LOCAL_PATH="${LOCAL_PATH:-}"
HDC_BIN="${HDC_BIN:-}"
PROBE_RUNNER=1
DRY_RUN=0
RUNNER_LAUNCH_ERROR_RE="${RUNNER_LAUNCH_ERROR_RE:-inaccessible or not found|no such file|not found|permission denied|exec format error|cannot execute binary file|cannot link executable|bad elf|invalid elf|library .*not found|linker .*not found}"

usage() {
  cat <<'USAGE'
usage: scripts/bootstrap-model-run-tool.sh --source-target SRC --dest-target DST [options]

Compatibility wrapper for copying a native model_run_tool from one HarmonyOS
target to another. For generic files, use scripts/copy-target-file.sh instead.

Options:
  --source-target TARGET   HDC target id that already has a working runner.
  --dest-target TARGET     HDC target id that needs the runner.
  --source-path PATH       Source target runner path. Default: /data/local/tmp/model_run_tool
  --dest-path PATH         Destination target runner path.
                           Default: /data/local/tmp/z84378291/model_run_tool
  --local-path PATH        Host temp path. Default: /tmp/model_run_tool.<source-target>
  --no-probe              Do not run --version/--help probe before and after copy.
  --hdc PATH              hdc binary path. Default: first hdc in PATH.
  --dry-run               Print resolved commands and exit.
  -h, --help              Show this help.

Example:
  scripts/bootstrap-model-run-tool.sh \
    --source-target SH236HS0488 \
    --dest-target SH25BHS4036

Equivalent generic command:
  scripts/copy-target-file.sh \
    --source-target SH236HS0488 \
    --dest-target SH25BHS4036 \
    --source-path /data/local/tmp/model_run_tool \
    --dest-path /data/local/tmp/z84378291/model_run_tool \
    --executable \
    --require-non-empty
USAGE
}

log() {
  printf '[kirin-runner-bootstrap] %s\n' "$*"
}

warn() {
  printf '[kirin-runner-bootstrap] WARN: %s\n' "$*" >&2
}

die() {
  printf '[kirin-runner-bootstrap] ERROR: %s\n' "$*" >&2
  exit 1
}

remote_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

probe_runner() {
  local target="$1"
  local path="$2"
  local label="$3"
  local cmd
  local status
  local probe_log

  [ "${PROBE_RUNNER}" -eq 1 ] || return 0

  log "probing ${label}: ${target}:${path}"
  cmd="echo '### runner path'; ls -l $(remote_quote "${path}") 2>&1 || exit 125; echo; echo '### runner launch probe'; $(remote_quote "${path}") --version 2>&1 || $(remote_quote "${path}") --help 2>&1 || true"
  probe_log="$(mktemp "${TMPDIR:-/tmp}/kirin-runner-probe.XXXXXX")"

  set +e
  "${HDC_BIN}" -t "${target}" shell "${cmd}" > "${probe_log}" 2>&1
  status=$?
  set -e

  sed 's/^/[kirin-runner-bootstrap]   /' "${probe_log}" || true

  if [ "${status}" -ne 0 ]; then
    rm -f "${probe_log}"
    die "${label} runner probe failed on ${target}:${path}"
  fi
  if grep -Eiq "${RUNNER_LAUNCH_ERROR_RE}" "${probe_log}"; then
    rm -f "${probe_log}"
    die "${label} runner exists but cannot launch cleanly on ${target}:${path}"
  fi
  if ! grep -Eiq 'model_run_tool|usage|version' "${probe_log}"; then
    warn "${label} runner probe produced no recognizable usage/version output; continuing because the file exists and no launch error was detected"
  fi
  rm -f "${probe_log}"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --source-target)
      [ "$#" -ge 2 ] || die "--source-target requires a target id"
      SOURCE_TARGET="$2"
      shift 2
      ;;
    --dest-target|--target)
      [ "$#" -ge 2 ] || die "--dest-target requires a target id"
      DEST_TARGET="$2"
      shift 2
      ;;
    --source-path)
      [ "$#" -ge 2 ] || die "--source-path requires a path"
      SOURCE_PATH="$2"
      shift 2
      ;;
    --dest-path)
      [ "$#" -ge 2 ] || die "--dest-path requires a path"
      DEST_PATH="$2"
      shift 2
      ;;
    --local-path)
      [ "$#" -ge 2 ] || die "--local-path requires a path"
      LOCAL_PATH="$2"
      shift 2
      ;;
    --no-probe)
      PROBE_RUNNER=0
      shift
      ;;
    --hdc)
      [ "$#" -ge 2 ] || die "--hdc requires a path"
      HDC_BIN="$2"
      shift 2
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

[ -n "${SOURCE_TARGET}" ] || die "--source-target is required"
[ -n "${DEST_TARGET}" ] || die "--dest-target is required"

if [ -z "${HDC_BIN}" ]; then
  HDC_BIN="$(command -v hdc || true)"
  if [ -z "${HDC_BIN}" ] && [ "${DRY_RUN}" -eq 1 ]; then
    HDC_BIN="hdc"
  fi
fi
[ -n "${HDC_BIN}" ] || die "hdc not found in PATH"

if [ -z "${LOCAL_PATH}" ]; then
  LOCAL_PATH="/tmp/model_run_tool.${SOURCE_TARGET}"
fi

COPY_ARGS=(
  "${ROOT}/scripts/copy-target-file.sh"
  --source-target "${SOURCE_TARGET}"
  --dest-target "${DEST_TARGET}"
  --source-path "${SOURCE_PATH}"
  --dest-path "${DEST_PATH}"
  --local-path "${LOCAL_PATH}"
  --executable
  --require-non-empty
  --hdc "${HDC_BIN}"
)

if [ "${DRY_RUN}" -eq 1 ]; then
  "${COPY_ARGS[@]}" --dry-run
  if [ "${PROBE_RUNNER}" -eq 1 ]; then
    cat <<EOF

Runner probe commands:
  ${HDC_BIN} -t ${SOURCE_TARGET} shell "${SOURCE_PATH} --version || ${SOURCE_PATH} --help"
  ${HDC_BIN} -t ${DEST_TARGET} shell "${DEST_PATH} --version || ${DEST_PATH} --help"
EOF
  fi
  exit 0
fi

probe_runner "${SOURCE_TARGET}" "${SOURCE_PATH}" "source"
"${COPY_ARGS[@]}"
probe_runner "${DEST_TARGET}" "${DEST_PATH}" "destination"

DEST_DIR="${DEST_PATH%/*}"
log "runner bootstrap complete"
log "use --model-run-tool ${DEST_PATH} --device-dir ${DEST_DIR} in scripts/test-naked-omc-vetest.sh"
