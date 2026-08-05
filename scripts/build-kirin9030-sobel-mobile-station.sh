#!/usr/bin/env bash
# Build SobelCustom for Kirin9030 with the x86 mobile-station toolkit on npu-group-3.

set -euo pipefail

REMOTE_HOST="${KIRIN_NPU_HOST:-npu-group-3}"
CONTAINER="${KIRIN_MOBILE_STATION_CONTAINER:-z84378291-kirin-mobile-x86}"
REMOTE_REPO="${KIRIN_REMOTE_REPO:-/data1/z84378291/kirin-ops}"
REMOTE_ARTIFACT_ROOT="${KIRIN_REMOTE_ARTIFACT_ROOT:-/data1/z84378291/artifacts}"
CANN_HOME="${KIRIN_MOBILE_STATION_CANN_HOME:-/opt/Ascend/cann-9.0.0}"
BUILD_NAME="kirin9030_sobel_mobile_x86_$(date +%Y%m%d_%H%M%S)"
FIXTURES_DIR=""
PULL_TO_DIR=""
BUILD_JOBS="${KIRIN_BUILD_JOBS:-8}"

usage() {
  cat <<'USAGE'
usage: scripts/build-kirin9030-sobel-mobile-station.sh [options]

Builds a Kirin9030 SobelCustom OMC in the x86 mobile-station container on npu-group-3.
The vendored Sobel source is copied to a remote scratch artifact before patching.

Options:
  --remote-host HOST       SSH host. Default: npu-group-3.
  --container NAME         Docker container. Default: z84378291-kirin-mobile-x86.
  --remote-repo PATH       Remote kirin-ops checkout. Default: /data1/z84378291/kirin-ops.
  --artifact-root PATH     Remote artifact root. Default: /data1/z84378291/artifacts.
  --cann-home PATH         Mobile-station CANN path. Default: /opt/Ascend/cann-9.0.0.
  --build-name NAME        Remote artifact directory name.
  --fixtures-dir PATH      Remote directory containing SobelCustom.onnx, x.bin, and y.bin.
                           If omitted, the script runs the source test generators in the scratch copy.
  --pull-to-dir DIR        Pull SobelCustom_kirin9030.omc, SobelCustom.onnx, x.bin, and y.bin locally.
  --jobs N                 Build jobs inside the x86 container. Default: 8.
  -h, --help               Show this help.

Example:
  scripts/build-kirin9030-sobel-mobile-station.sh \
    --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
    --pull-to-dir artifacts/remote-pulled/kirin9030-sobel-2026-08-04
USAGE
}

die() {
  printf '[kirin9030-sobel-build] ERROR: %s\n' "$*" >&2
  exit 1
}

log() {
  printf '[kirin9030-sobel-build] %s\n' "$*"
}

shell_quote() {
  printf '%q' "$1"
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --remote-host)
      [ "$#" -ge 2 ] || die "--remote-host requires a value"
      REMOTE_HOST="$2"
      shift 2
      ;;
    --container)
      [ "$#" -ge 2 ] || die "--container requires a value"
      CONTAINER="$2"
      shift 2
      ;;
    --remote-repo)
      [ "$#" -ge 2 ] || die "--remote-repo requires a value"
      REMOTE_REPO="$2"
      shift 2
      ;;
    --artifact-root)
      [ "$#" -ge 2 ] || die "--artifact-root requires a value"
      REMOTE_ARTIFACT_ROOT="$2"
      shift 2
      ;;
    --cann-home)
      [ "$#" -ge 2 ] || die "--cann-home requires a value"
      CANN_HOME="$2"
      shift 2
      ;;
    --build-name)
      [ "$#" -ge 2 ] || die "--build-name requires a value"
      BUILD_NAME="$2"
      shift 2
      ;;
    --fixtures-dir)
      [ "$#" -ge 2 ] || die "--fixtures-dir requires a value"
      FIXTURES_DIR="$2"
      shift 2
      ;;
    --pull-to-dir)
      [ "$#" -ge 2 ] || die "--pull-to-dir requires a value"
      PULL_TO_DIR="$2"
      shift 2
      ;;
    --jobs)
      [ "$#" -ge 2 ] || die "--jobs requires a value"
      BUILD_JOBS="$2"
      shift 2
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

case "${BUILD_NAME}" in
  *[!A-Za-z0-9._-]*|'')
    die "--build-name may only contain letters, numbers, dot, underscore, and dash"
    ;;
esac

case "${BUILD_JOBS}" in
  ''|*[!0-9]*)
    die "--jobs must be a positive integer"
    ;;
  0)
    die "--jobs must be greater than zero"
    ;;
esac

REMOTE_ARTIFACT="${REMOTE_ARTIFACT_ROOT%/}/${BUILD_NAME}"

log "remote host: ${REMOTE_HOST}"
log "container: ${CONTAINER}"
log "remote artifact: ${REMOTE_ARTIFACT}"

# The --env values are intentionally expanded locally before ssh invokes docker.
# shellcheck disable=SC2029
ssh "${REMOTE_HOST}" \
  "docker exec -i \
    --env KIRIN_REMOTE_REPO=$(shell_quote "${REMOTE_REPO}") \
    --env KIRIN_ARTIFACT_ROOT=$(shell_quote "${REMOTE_ARTIFACT_ROOT}") \
    --env KIRIN_BUILD_NAME=$(shell_quote "${BUILD_NAME}") \
    --env KIRIN_CANN_HOME=$(shell_quote "${CANN_HOME}") \
    --env KIRIN_FIXTURES_DIR=$(shell_quote "${FIXTURES_DIR}") \
    --env KIRIN_BUILD_JOBS=$(shell_quote "${BUILD_JOBS}") \
    $(shell_quote "${CONTAINER}") bash -s" <<'REMOTE_SCRIPT'
set -euo pipefail

log() {
  printf '[kirin9030-sobel-build:remote] %s\n' "$*"
}

die() {
  printf '[kirin9030-sobel-build:remote] ERROR: %s\n' "$*" >&2
  exit 1
}

SRC="${KIRIN_REMOTE_REPO}/cann-recipes-harmony-infer/ops/ascendc/src/sobel_custom"
ART="${KIRIN_ARTIFACT_ROOT%/}/${KIRIN_BUILD_NAME}"
WORK="${ART}/sobel_custom"
MODEL_DIR="${ART}/model_conversion"

[ -d "${SRC}" ] || die "Sobel source not found: ${SRC}"
[ -d "${KIRIN_CANN_HOME}" ] || die "CANN home not found: ${KIRIN_CANN_HOME}"

rm -rf "${ART}"
mkdir -p "${ART}"
cp -a "${SRC}" "${WORK}"

cd "${WORK}"

perl -0pi -e 's/"value": "kirinx90"/"value": "kirin9030"/g; s#"value": "/usr/local/Ascend/cann"#"value": "'"${KIRIN_CANN_HOME}"'"#g' CMakePresets.json
perl -0pi -e 's/AddConfig\("kirinx90"/AddConfig("kirin9030"/g' op_host/sobel_custom.cpp
perl -0pi -e 's/-j\$\(nproc\)/-j\${KIRIN_BUILD_JOBS:-\$(nproc)}/g' build_and_install.sh

# dav_l311 provides Transpose4DImpl, while the public wrapper compiles a cSize==1
# branch that references TransposeUB2UBImpl, which is absent for this architecture.
perl -0pi -e 's/uint32_t offset;\n/uint32_t offset = 0;\n/g; s/\(0 < j < cntW - 1\)/(0 < j \&\& j < cntW - 1)/g; s/AscendC::Transpose\(tempTensor0, xLocal, stackBuffer, transposeParams\);/#if __NPU_ARCH__ == 3113\n        AscendC::Transpose4DImpl(tempTensor0, xLocal, stackBuffer, transposeParams);\n#else\n        AscendC::Transpose(tempTensor0, xLocal, stackBuffer, transposeParams);\n#endif/g' op_kernel/sobel_custom.cpp
perl -0pi -e 's/AscendC::Add\(tmpBuf0, dx, dy, w \* \(h - 2\)\);\n        \/\/ half->u8/AscendC::Add(tmpBuf0, dx, dy, w * (h - 2));\n        AscendC::Mins(tmpBuf0, tmpBuf0, half(255), w * (h - 2));\n        \/\/ half->u8/g' op_kernel/sobel_custom.cpp

compute_unit_block="$(grep -A4 '"ASCEND_COMPUTE_UNIT"' CMakePresets.json || true)"
grep -q '"value": "kirin9030"' <<<"${compute_unit_block}" || die "failed to set ASCEND_COMPUTE_UNIT"
grep -q 'AddConfig("kirin9030"' op_host/sobel_custom.cpp || die "failed to set AddConfig"
grep -q 'Transpose4DImpl' op_kernel/sobel_custom.cpp || die "failed to apply dav_l311 transpose patch"
grep -q 'Mins(tmpBuf0, tmpBuf0, half(255)' op_kernel/sobel_custom.cpp || die "failed to apply uint8 clamp patch"

set +e +u
source "${KIRIN_CANN_HOME}/set_env.sh" >"${ART}/set_env.stdout" 2>"${ART}/set_env.stderr"
source_status=$?
set -euo pipefail
if [ "${source_status}" -ne 0 ]; then
  die "failed to source ${KIRIN_CANN_HOME}/set_env.sh"
fi
export ASCEND_HOME_PATH="${KIRIN_CANN_HOME}"

log "building custom op package"
set +e
bash build_and_install.sh package >"${ART}/build_and_install.log" 2>&1
build_status=$?
set -e
printf 'build_status=%s\n' "${build_status}" | tee "${ART}/build.status"
if [ "${build_status}" -ne 0 ]; then
  tail -160 "${ART}/build_and_install.log" >&2
  exit "${build_status}"
fi

find "${WORK}/build_out" -maxdepth 3 -type f \
  \( -name 'custom_opp_*.run' -o -name '*.so' -o -name '*.json' \) \
  -print | sort >"${ART}/built_files.txt"
[ -s "${ART}/built_files.txt" ] || die "no build files recorded"
grep -q 'custom_opp_.*\.run' "${ART}/built_files.txt" || die "custom_opp run package not found"

mkdir -p "${MODEL_DIR}"
if [ -n "${KIRIN_FIXTURES_DIR}" ]; then
  log "using fixtures from ${KIRIN_FIXTURES_DIR}"
  cp "${KIRIN_FIXTURES_DIR}/SobelCustom.onnx" "${KIRIN_FIXTURES_DIR}/x.bin" "${KIRIN_FIXTURES_DIR}/y.bin" "${MODEL_DIR}/"
else
  log "generating fixtures from source test scripts"
  cd "${WORK}/test"
  python3 create_onnx.py
  python3 gen_data.py
  cp SobelCustom.onnx x.bin y.bin "${MODEL_DIR}/"
fi

cd "${MODEL_DIR}"
export ASCEND_CUSTOM_OPP_PATH="${KIRIN_CANN_HOME}/opp/vendors/customize"

log "running ATC for Kirin9030"
set +e
atc --model=./SobelCustom.onnx --framework=5 --output=./SobelCustom_kirin9030 --soc_version=Kirin9030 >atc.stdout 2>atc.stderr
atc_status=$?
set -e
printf 'atc_status=%s\n' "${atc_status}" | tee atc.status
if [ "${atc_status}" -ne 0 ]; then
  sed -n '1,220p' atc.stdout >&2
  sed -n '1,220p' atc.stderr >&2
  exit "${atc_status}"
fi

find "${MODEL_DIR}" -maxdepth 2 -type f -printf '%p %s bytes\n' | sort >converted_files.txt
[ -f "${MODEL_DIR}/SobelCustom_kirin9030.omc" ] || die "ATC succeeded but SobelCustom_kirin9030.omc was not produced"

log "artifact=${ART}"
log "omc=${MODEL_DIR}/SobelCustom_kirin9030.omc"
REMOTE_SCRIPT

if [ -n "${PULL_TO_DIR}" ]; then
  mkdir -p "${PULL_TO_DIR}"
  scp -q \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_conversion/SobelCustom_kirin9030.omc" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_conversion/SobelCustom.onnx" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_conversion/x.bin" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_conversion/y.bin" \
    "${PULL_TO_DIR}/"
  log "pulled model files to ${PULL_TO_DIR}"
  log "package with: scripts/package-naked-omc-bundle.sh --name kirin9030-sobel-custom-2026-08-04 --omc ${PULL_TO_DIR}/SobelCustom_kirin9030.omc --input ${PULL_TO_DIR}/x.bin --golden ${PULL_TO_DIR}/y.bin --target-soc kirin9030"
fi
