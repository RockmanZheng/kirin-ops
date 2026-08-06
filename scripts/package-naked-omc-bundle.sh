#!/usr/bin/env bash
# Package a generic naked OMC bundle for HarmonyOS model_run_tool testing.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

NAME=""
DESCRIPTION=""
OMC=""
INPUT=""
GOLDEN=""
OUTPUT_NAME="output_0"
OUTPUT_TYPE=""
TARGET_SOC=""
COMPARE=""
COMPARE_SCRIPT=""
OUT_DIR="${ROOT}/artifacts/naked-omc"
RELEASE_DIR="${ROOT}/artifacts/releases"
FORCE=0

usage() {
  cat <<'USAGE'
usage: scripts/package-naked-omc-bundle.sh [options]

Creates a portable naked-OMC bundle:

  <name>/
    README.txt
    SHA256SUMS
    bundle.env
    <model.omc>
    <input files>
    [golden output]

and a zip under artifacts/releases/.

Options:
  --name NAME          Bundle directory/zip name, e.g. kirin9030-gelu-fp16-2026-08-04.
  --description TEXT   Short description written into README.txt and bundle.env.
  --omc PATH           Local .omc model file.
  --input PATHS        Local input .bin path, or comma-separated local input paths.
  --golden PATH        Optional golden output .bin.
  --output-name NAME   Device output file to pull. Default: output_0.
  --output-type TYPE   Device output tensor dtype, e.g. UINT8 for Sobel output_0.
  --target-soc SOC     Expected target SoC, e.g. kirin9030.
  --compare 0|1        Whether the runner should compare output with golden.
                       Default: 1 when --golden is present, otherwise 0.
  --compare-script PATH
                       Optional Python precision validator copied into the bundle.
  --out-dir DIR        Bundle root. Default: artifacts/naked-omc.
  --release-dir DIR    Zip output root. Default: artifacts/releases.
  --force              Replace an existing bundle directory/zip.
  -h, --help           Show this help.

Example:
  scripts/package-naked-omc-bundle.sh \
    --name kirin9030-gelu-fp16-2026-08-04 \
    --description "GELU FP16 prebuilt OMC for Kirin9030 naked model_run_tool test" \
    --omc ./gelu_fp16.omc \
    --input ./gelu_fp16_input.bin \
    --target-soc kirin9030
USAGE
}

die() {
  printf '[package-naked-omc] ERROR: %s\n' "$*" >&2
  exit 1
}

log() {
  printf '[package-naked-omc] %s\n' "$*"
}

sha256_file() {
  local file="$1"

  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "${file}"
  elif command -v sha256sum >/dev/null 2>&1; then
    sha256sum "${file}"
  else
    die "shasum/sha256sum not found"
  fi
}

manifest_value() {
  printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g'
}

copy_file_once() {
  local source="$1"
  local dest_dir="$2"
  local base
  local dest

  [ -f "${source}" ] || die "file not found: ${source}"
  base="$(basename "${source}")"
  dest="${dest_dir}/${base}"
  if [ -e "${dest}" ] && [ "$(cd "$(dirname "${source}")" && pwd -P)/${base}" != "${dest}" ]; then
    die "duplicate bundle filename would overwrite ${base}; rename one input first"
  fi
  cp -p "${source}" "${dest}"
  printf '%s\n' "${base}"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --name)
      [ "$#" -ge 2 ] || die "--name requires a value"
      NAME="$2"
      shift 2
      ;;
    --description)
      [ "$#" -ge 2 ] || die "--description requires a value"
      DESCRIPTION="$2"
      shift 2
      ;;
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
    --golden)
      [ "$#" -ge 2 ] || die "--golden requires a path"
      GOLDEN="$2"
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
      [ "$#" -ge 2 ] || die "--target-soc requires a value"
      TARGET_SOC="$2"
      shift 2
      ;;
    --compare)
      [ "$#" -ge 2 ] || die "--compare requires 0 or 1"
      COMPARE="$2"
      shift 2
      ;;
    --compare-script)
      [ "$#" -ge 2 ] || die "--compare-script requires a Python script path"
      COMPARE_SCRIPT="$2"
      shift 2
      ;;
    --compare-mode|--compare-validator)
      die "$1 is retired; use --compare-script for a Python precision validator"
      ;;
    --out-dir)
      [ "$#" -ge 2 ] || die "--out-dir requires a directory"
      OUT_DIR="$2"
      shift 2
      ;;
    --release-dir)
      [ "$#" -ge 2 ] || die "--release-dir requires a directory"
      RELEASE_DIR="$2"
      shift 2
      ;;
    --force)
      FORCE=1
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

[ -n "${NAME}" ] || die "--name is required"
[ -n "${OMC}" ] || die "--omc is required"
[ -n "${INPUT}" ] || die "--input is required"
[ -f "${OMC}" ] || die "OMC file not found: ${OMC}"

case "${NAME}" in
  *[!A-Za-z0-9._-]*|'')
    die "--name may only contain letters, numbers, dot, underscore, and dash"
    ;;
esac

if [ -z "${COMPARE}" ]; then
  if [ -n "${GOLDEN}" ]; then
    COMPARE=1
  else
    COMPARE=0
  fi
fi

case "${COMPARE}" in
  0|1)
    ;;
  *)
    die "--compare must be 0 or 1"
    ;;
esac

[ -z "${COMPARE_SCRIPT}" ] || [ -f "${COMPARE_SCRIPT}" ] || die "compare script not found: ${COMPARE_SCRIPT}"

mkdir -p "${OUT_DIR}" "${RELEASE_DIR}"
OUT_DIR="$(cd "${OUT_DIR}" && pwd -P)"
RELEASE_DIR="$(cd "${RELEASE_DIR}" && pwd -P)"

BUNDLE_DIR="${OUT_DIR}/${NAME}"
ZIP_PATH="${RELEASE_DIR}/${NAME}.zip"

if [ -e "${BUNDLE_DIR}" ] || [ -e "${ZIP_PATH}" ]; then
  [ "${FORCE}" -eq 1 ] || die "${BUNDLE_DIR} or ${ZIP_PATH} already exists; pass --force to replace"
  rm -rf "${BUNDLE_DIR}" "${ZIP_PATH}"
fi

mkdir -p "${BUNDLE_DIR}"

OMC_BASE="$(copy_file_once "${OMC}" "${BUNDLE_DIR}")"

INPUT_BASES=""
IFS=',' read -r -a INPUT_FILES <<< "${INPUT}"
for input_file in "${INPUT_FILES[@]}"; do
  input_file="$(printf '%s' "${input_file}" | sed -E 's/^[[:space:]]+//; s/[[:space:]]+$//')"
  [ -n "${input_file}" ] || continue
  input_base="$(copy_file_once "${input_file}" "${BUNDLE_DIR}")"
  if [ -z "${INPUT_BASES}" ]; then
    INPUT_BASES="${input_base}"
  else
    INPUT_BASES="${INPUT_BASES},${input_base}"
  fi
done

[ -n "${INPUT_BASES}" ] || die "no valid input files provided"

GOLDEN_BASE=""
if [ -n "${GOLDEN}" ]; then
  GOLDEN_BASE="$(copy_file_once "${GOLDEN}" "${BUNDLE_DIR}")"
fi

COMPARE_SCRIPT_BASE=""
if [ -n "${COMPARE_SCRIPT}" ]; then
  COMPARE_SCRIPT_BASE="$(copy_file_once "${COMPARE_SCRIPT}" "${BUNDLE_DIR}")"
fi

{
  printf 'NAME="%s"\n' "$(manifest_value "${NAME}")"
  printf 'DESCRIPTION="%s"\n' "$(manifest_value "${DESCRIPTION}")"
  printf 'OMC="%s"\n' "$(manifest_value "${OMC_BASE}")"
  printf 'INPUT="%s"\n' "$(manifest_value "${INPUT_BASES}")"
  if [ -n "${GOLDEN_BASE}" ]; then
    printf 'GOLDEN="%s"\n' "$(manifest_value "${GOLDEN_BASE}")"
  fi
  printf 'OUTPUT_NAME="%s"\n' "$(manifest_value "${OUTPUT_NAME}")"
  if [ -n "${OUTPUT_TYPE}" ]; then
    printf 'OUTPUT_TYPE="%s"\n' "$(manifest_value "${OUTPUT_TYPE}")"
  fi
  if [ -n "${TARGET_SOC}" ]; then
    printf 'TARGET_SOC="%s"\n' "$(manifest_value "${TARGET_SOC}")"
  fi
  printf 'COMPARE="%s"\n' "${COMPARE}"
  if [ -n "${COMPARE_SCRIPT_BASE}" ]; then
    printf 'COMPARE_SCRIPT="%s"\n' "$(manifest_value "${COMPARE_SCRIPT_BASE}")"
  fi
} > "${BUNDLE_DIR}/bundle.env"

{
  echo "${NAME}"
  echo
  if [ -n "${DESCRIPTION}" ]; then
    echo "${DESCRIPTION}"
    echo
  fi
  echo "Run:"
  echo "  scripts/test-naked-omc-vetest.sh --bundle-dir \"\$PWD/${NAME}\" --no-clear-logs"
  echo
  echo "Manifest:"
  sed 's/^/  /' "${BUNDLE_DIR}/bundle.env"
} > "${BUNDLE_DIR}/README.txt"

(
  cd "${BUNDLE_DIR}"
  : > SHA256SUMS
  for file in README.txt bundle.env "${OMC_BASE}"; do
    sha256_file "${file}" >> SHA256SUMS
  done
  IFS=',' read -r -a INPUT_BASE_ARRAY <<< "${INPUT_BASES}"
  for file in "${INPUT_BASE_ARRAY[@]}"; do
    sha256_file "${file}" >> SHA256SUMS
  done
  if [ -n "${GOLDEN_BASE}" ]; then
    sha256_file "${GOLDEN_BASE}" >> SHA256SUMS
  fi
  if [ -n "${COMPARE_SCRIPT_BASE}" ]; then
    sha256_file "${COMPARE_SCRIPT_BASE}" >> SHA256SUMS
  fi
)

if ! command -v zip >/dev/null 2>&1; then
  die "zip not found"
fi

(
  cd "${OUT_DIR}"
  zip -qr "${ZIP_PATH}" "${NAME}"
)

sha256_file "${ZIP_PATH}" > "${ZIP_PATH}.sha256"

log "bundle: ${BUNDLE_DIR}"
log "zip: ${ZIP_PATH}"
log "zip sha256: ${ZIP_PATH}.sha256"
