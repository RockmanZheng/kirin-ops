#!/usr/bin/env bash
# Copy one file from one HarmonyOS target to another through the host.

set -euo pipefail

SOURCE_TARGET="${SOURCE_TARGET:-}"
DEST_TARGET="${DEST_TARGET:-}"
SOURCE_PATH="${SOURCE_PATH:-}"
DEST_PATH="${DEST_PATH:-}"
LOCAL_PATH="${LOCAL_PATH:-}"
HDC_BIN="${HDC_BIN:-}"
CHMOD_MODE="${CHMOD_MODE:-}"
REQUIRE_NON_EMPTY=0
DRY_RUN=0

usage() {
  cat <<'USAGE'
usage: scripts/copy-target-file.sh --source-target SRC --dest-target DST --source-path PATH --dest-path PATH [options]

Copies one file from one HarmonyOS target to another through the host:

  source target:/path/to/file
    -> host:/tmp/kirin-target-file.<basename>.<SRC>
    -> dest target:/path/to/file

Options:
  --source-target TARGET   HDC target id that has the source file.
  --dest-target TARGET     HDC target id that receives the file.
  --source-path PATH       Source target file path.
  --dest-path PATH         Destination target file path.
  --local-path PATH        Host temp path. Default:
                           /tmp/kirin-target-file.<source-basename>.<source-target>
  --chmod MODE             Run chmod MODE on the destination file after copy.
  --executable             Shortcut for --chmod 755.
  --no-chmod               Do not chmod the destination file.
  --require-non-empty      Fail if the pulled host file is empty.
  --hdc PATH               hdc binary path. Default: first hdc in PATH.
  --dry-run                Print resolved commands and exit.
  -h, --help               Show this help.

Example:
  scripts/copy-target-file.sh \
    --source-target SH236HS0488 \
    --dest-target SH25BHS4036 \
    --source-path /data/local/tmp/libfoo.so \
    --dest-path /data/local/tmp/z84378291/libfoo.so

Executable example:
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
  printf '[kirin-target-file-copy] %s\n' "$*"
}

warn() {
  printf '[kirin-target-file-copy] WARN: %s\n' "$*" >&2
}

die() {
  printf '[kirin-target-file-copy] ERROR: %s\n' "$*" >&2
  exit 1
}

remote_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

host_sha256() {
  local file="$1"

  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "${file}"
  elif command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "${file}"
  else
    warn "sha256sum/shasum not found"
  fi
}

remote_file_metadata() {
  local target="$1"
  local path="$2"
  local label="$3"
  local cmd

  log "${label} metadata: ${target}:${path}"
  cmd="echo '### file'; ls -l $(remote_quote "${path}") 2>&1 || exit 125; echo; echo '### hash if available'; if command -v sha256sum >/dev/null 2>&1; then sha256sum $(remote_quote "${path}") 2>&1 || true; elif command -v md5sum >/dev/null 2>&1; then md5sum $(remote_quote "${path}") 2>&1 || true; else echo 'no sha256sum/md5sum on target'; fi"
  "${HDC_BIN}" -t "${target}" shell "${cmd}" | sed 's/^/[kirin-target-file-copy]   /'
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
    --chmod)
      [ "$#" -ge 2 ] || die "--chmod requires a mode"
      CHMOD_MODE="$2"
      shift 2
      ;;
    --executable)
      CHMOD_MODE=755
      shift
      ;;
    --no-chmod)
      CHMOD_MODE=""
      shift
      ;;
    --require-non-empty)
      REQUIRE_NON_EMPTY=1
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
[ -n "${SOURCE_PATH}" ] || die "--source-path is required"
[ -n "${DEST_PATH}" ] || die "--dest-path is required"

case "${CHMOD_MODE}" in
  ''|[0-7][0-7][0-7]|[0-7][0-7][0-7][0-7])
    ;;
  *)
    die "--chmod must be an octal mode such as 644 or 755"
    ;;
esac

if [ -z "${HDC_BIN}" ]; then
  HDC_BIN="$(command -v hdc || true)"
  if [ -z "${HDC_BIN}" ] && [ "${DRY_RUN}" -eq 1 ]; then
    HDC_BIN="hdc"
  fi
fi
[ -n "${HDC_BIN}" ] || die "hdc not found in PATH"

if [ -z "${LOCAL_PATH}" ]; then
  SOURCE_BASENAME="$(basename "${SOURCE_PATH}")"
  [ -n "${SOURCE_BASENAME}" ] && [ "${SOURCE_BASENAME}" != "/" ] || die "invalid --source-path: ${SOURCE_PATH}"
  LOCAL_PATH="/tmp/kirin-target-file.${SOURCE_BASENAME}.${SOURCE_TARGET}"
fi

DEST_DIR="${DEST_PATH%/*}"
[ -n "${DEST_DIR}" ] && [ "${DEST_DIR}" != "${DEST_PATH}" ] || die "invalid --dest-path: ${DEST_PATH}"

if [ "${DRY_RUN}" -eq 1 ]; then
  cat <<EOF
HDC_BIN=${HDC_BIN}
SOURCE_TARGET=${SOURCE_TARGET}
DEST_TARGET=${DEST_TARGET}
SOURCE_PATH=${SOURCE_PATH}
DEST_PATH=${DEST_PATH}
DEST_DIR=${DEST_DIR}
LOCAL_PATH=${LOCAL_PATH}
CHMOD_MODE=${CHMOD_MODE}
REQUIRE_NON_EMPTY=${REQUIRE_NON_EMPTY}

Commands:
  ${HDC_BIN} -t ${SOURCE_TARGET} shell "ls -l ${SOURCE_PATH}"
  ${HDC_BIN} -t ${SOURCE_TARGET} file recv ${SOURCE_PATH} ${LOCAL_PATH}
  ${HDC_BIN} -t ${DEST_TARGET} shell "mkdir -p ${DEST_DIR}"
  ${HDC_BIN} -t ${DEST_TARGET} file send ${LOCAL_PATH} ${DEST_PATH}
EOF
  if [ -n "${CHMOD_MODE}" ]; then
    printf '  %s -t %s shell "chmod %s %s"\n' "${HDC_BIN}" "${DEST_TARGET}" "${CHMOD_MODE}" "${DEST_PATH}"
  fi
  exit 0
fi

log "source target: ${SOURCE_TARGET}"
log "dest target: ${DEST_TARGET}"
log "source path: ${SOURCE_PATH}"
log "dest path: ${DEST_PATH}"
log "host temp path: ${LOCAL_PATH}"

remote_file_metadata "${SOURCE_TARGET}" "${SOURCE_PATH}" "source"

log "pulling file to host"
mkdir -p "$(dirname "${LOCAL_PATH}")"
"${HDC_BIN}" -t "${SOURCE_TARGET}" file recv "${SOURCE_PATH}" "${LOCAL_PATH}"
[ -e "${LOCAL_PATH}" ] || die "pulled file is missing: ${LOCAL_PATH}"
if [ "${REQUIRE_NON_EMPTY}" -eq 1 ] && [ ! -s "${LOCAL_PATH}" ]; then
  die "pulled file is empty: ${LOCAL_PATH}"
fi

log "host file metadata"
ls -l "${LOCAL_PATH}"
host_sha256 "${LOCAL_PATH}" || true
if command -v file >/dev/null 2>&1; then
  file "${LOCAL_PATH}" || true
fi

log "creating destination dir on target"
"${HDC_BIN}" -t "${DEST_TARGET}" shell "mkdir -p $(remote_quote "${DEST_DIR}")"

log "pushing file to destination target"
"${HDC_BIN}" -t "${DEST_TARGET}" file send "${LOCAL_PATH}" "${DEST_PATH}"

if [ -n "${CHMOD_MODE}" ]; then
  log "setting destination mode: ${CHMOD_MODE}"
  "${HDC_BIN}" -t "${DEST_TARGET}" shell "chmod ${CHMOD_MODE} $(remote_quote "${DEST_PATH}")"
fi

remote_file_metadata "${DEST_TARGET}" "${DEST_PATH}" "destination"

log "file copy complete"
