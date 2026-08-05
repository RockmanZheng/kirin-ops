#!/usr/bin/env bash
# Build, convert, run, and compare the vendored SobelCustom op on Ascend 910B2.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REMOTE_HOST="${KIRIN_NPU_HOST:-npu-group-3}"
CONTAINER="${SOBEL_910B2_CONTAINER:-z84378291-kirin-cann90}"
REMOTE_REPO="${KIRIN_REMOTE_REPO:-/data1/z84378291/kirin-ops}"
REMOTE_ARTIFACT_ROOT="${KIRIN_REMOTE_ARTIFACT_ROOT:-/data1/z84378291/artifacts}"
CANN_HOME="${SOBEL_910B2_CANN_HOME:-/usr/local/Ascend/cann-9.0.0-beta.2}"
BUILD_NAME="sobel910b2_verify_$(date +%Y%m%d_%H%M%S)"
FIXTURES_DIR="${SOBEL_910B2_FIXTURES_DIR:-}"
DEVICE_ID="${ASCEND_DEVICE_ID:-1}"
BUILD_JOBS="${KIRIN_BUILD_JOBS:-8}"
MAX_ABS_DIFF="${SOBEL_MAX_ABS_DIFF:-1}"
MAX_DIFF_RATE="${SOBEL_MAX_DIFF_RATE:-}"
KERNEL_MODE="${SOBEL_910B2_KERNEL_MODE:-vector-fixed}"
PULL_TO_DIR=""
RUN_ACL=1
RUN_COMPARE=1

usage() {
  cat <<'USAGE'
usage: scripts/build-sobel-910b2-verify.sh [options]

Builds the vendored SobelCustom Ascend C operator for Ascend910B2, converts the
SobelCustom ONNX model to .om, runs it with the Python ACL runtime, and compares
output_0.bin against y.bin.

The 910B2 validation build defaults to a fixed version of the vendored tiled
vector kernel. A scalar correctness kernel is available as a diagnostic fallback
through --kernel-mode scalar-correctness.

Defaults target the known-good 910B2 validation container on npu-group-3:
  host:      npu-group-3
  container: z84378291-kirin-cann90
  CANN:      /usr/local/Ascend/cann-9.0.0-beta.2
  compile:   ASCEND_COMPUTE_UNIT=ascend910b
  ATC:       --soc_version=Ascend910B2

Options:
  --remote-host HOST       SSH host. Default: npu-group-3.
  --container NAME         Docker container. Default: z84378291-kirin-cann90.
  --remote-repo PATH       Remote kirin-ops checkout. Default: /data1/z84378291/kirin-ops.
  --artifact-root PATH     Remote artifact root. Default: /data1/z84378291/artifacts.
  --cann-home PATH         CANN path in the container. Default: /usr/local/Ascend/cann-9.0.0-beta.2.
  --build-name NAME        Remote artifact directory name. Default: timestamped.
  --fixtures-dir PATH      Remote dir containing SobelCustom.onnx, x.bin, and y.bin.
                           If omitted, the script first tries the known generated fixture artifact.
  --device-id N            Ascend device id for ACL execution. Default: 1.
  --jobs N                 Build jobs in the container. Default: 8.
  --kernel-mode MODE       vector-fixed or scalar-correctness. Default: vector-fixed.
  --max-abs-diff N         Compare tolerance. Default: 1.
  --max-diff-rate R        Optional mismatch-rate tolerance, e.g. 0.01.
  --pull-to-dir DIR        Pull .om, input, golden, output, logs, and manifest locally.
  --no-run                 Stop after ATC conversion.
  --no-compare             Run ACL but skip accuracy compare.
  -h, --help               Show this help.

Example:
  scripts/build-sobel-910b2-verify.sh \
    --fixtures-dir /data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test \
    --pull-to-dir artifacts/remote-pulled/sobel910b2-verify
USAGE
}

log() {
  printf '[sobel910b2] %s\n' "$*"
}

die() {
  printf '[sobel910b2] ERROR: %s\n' "$*" >&2
  exit 1
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
    --device-id)
      [ "$#" -ge 2 ] || die "--device-id requires a value"
      DEVICE_ID="$2"
      shift 2
      ;;
    --jobs)
      [ "$#" -ge 2 ] || die "--jobs requires a value"
      BUILD_JOBS="$2"
      shift 2
      ;;
    --kernel-mode)
      [ "$#" -ge 2 ] || die "--kernel-mode requires a value"
      KERNEL_MODE="$2"
      shift 2
      ;;
    --max-abs-diff)
      [ "$#" -ge 2 ] || die "--max-abs-diff requires a value"
      MAX_ABS_DIFF="$2"
      shift 2
      ;;
    --max-diff-rate)
      [ "$#" -ge 2 ] || die "--max-diff-rate requires a value"
      MAX_DIFF_RATE="$2"
      shift 2
      ;;
    --pull-to-dir)
      [ "$#" -ge 2 ] || die "--pull-to-dir requires a value"
      PULL_TO_DIR="$2"
      shift 2
      ;;
    --no-run)
      RUN_ACL=0
      shift
      ;;
    --no-compare)
      RUN_COMPARE=0
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

case "${BUILD_NAME}" in
  *[!A-Za-z0-9._-]*|'')
    die "--build-name may only contain letters, numbers, dot, underscore, and dash"
    ;;
esac
case "${DEVICE_ID}" in
  ''|*[!0-9]*)
    die "--device-id must be a non-negative integer"
    ;;
esac
case "${BUILD_JOBS}" in
  ''|*[!0-9]*|0)
    die "--jobs must be a positive integer"
    ;;
esac
case "${MAX_ABS_DIFF}" in
  ''|*[!0-9]*)
    die "--max-abs-diff must be a non-negative integer"
    ;;
esac
case "${KERNEL_MODE}" in
  vector-fixed|scalar-correctness)
    ;;
  *)
    die "--kernel-mode must be vector-fixed or scalar-correctness"
    ;;
esac
if [ -n "${MAX_DIFF_RATE}" ]; then
  case "${MAX_DIFF_RATE}" in
    *[!0-9.]*|.*.*|.)
      die "--max-diff-rate must be a decimal value"
      ;;
  esac
fi

REMOTE_ARTIFACT="${REMOTE_ARTIFACT_ROOT%/}/${BUILD_NAME}"
VENDOR_NAME="$(printf '%s' "sobel_${BUILD_NAME}" | tr '[:upper:]' '[:lower:]' | tr -c 'a-z0-9_' '_' | cut -c1-63)"

for helper in scripts/run-om-acl.py scripts/compare-sobel-output.py scripts/validate-sobel-baseline.py scripts/templates/sobel_custom_scalar_910b2.cpp; do
  [ -f "${ROOT}/${helper}" ] || die "helper not found: ${helper}"
done

log "remote host: ${REMOTE_HOST}"
log "container: ${CONTAINER}"
log "remote artifact: ${REMOTE_ARTIFACT}"
log "vendor: ${VENDOR_NAME}"
log "kernel mode: ${KERNEL_MODE}"

# shellcheck disable=SC2029
ssh "${REMOTE_HOST}" "rm -rf $(shell_quote "${REMOTE_ARTIFACT}") && mkdir -p $(shell_quote "${REMOTE_ARTIFACT}/scripts")"
scp -q \
  "${ROOT}/scripts/run-om-acl.py" \
  "${ROOT}/scripts/compare-sobel-output.py" \
  "${ROOT}/scripts/validate-sobel-baseline.py" \
  "${ROOT}/scripts/templates/sobel_custom_scalar_910b2.cpp" \
  "${REMOTE_HOST}:${REMOTE_ARTIFACT}/scripts/"

set +e
# shellcheck disable=SC2029
ssh "${REMOTE_HOST}" \
  "docker exec -i \
    --env KIRIN_REMOTE_REPO=$(shell_quote "${REMOTE_REPO}") \
    --env KIRIN_ARTIFACT=$(shell_quote "${REMOTE_ARTIFACT}") \
    --env KIRIN_CANN_HOME=$(shell_quote "${CANN_HOME}") \
    --env KIRIN_FIXTURES_DIR=$(shell_quote "${FIXTURES_DIR}") \
    --env KIRIN_VENDOR_NAME=$(shell_quote "${VENDOR_NAME}") \
    --env KIRIN_BUILD_JOBS=$(shell_quote "${BUILD_JOBS}") \
    --env KIRIN_KERNEL_MODE=$(shell_quote "${KERNEL_MODE}") \
    --env KIRIN_DEVICE_ID=$(shell_quote "${DEVICE_ID}") \
    --env KIRIN_MAX_ABS_DIFF=$(shell_quote "${MAX_ABS_DIFF}") \
    --env KIRIN_MAX_DIFF_RATE=$(shell_quote "${MAX_DIFF_RATE}") \
    --env KIRIN_RUN_ACL=$(shell_quote "${RUN_ACL}") \
    --env KIRIN_RUN_COMPARE=$(shell_quote "${RUN_COMPARE}") \
    $(shell_quote "${CONTAINER}") bash -s" <<'REMOTE_SCRIPT'
set -euo pipefail

log() {
  printf '[sobel910b2:remote] %s\n' "$*"
}

die() {
  printf '[sobel910b2:remote] ERROR: %s\n' "$*" >&2
  exit 1
}

have_fixtures() {
  [ -f "$1/SobelCustom.onnx" ] && [ -f "$1/x.bin" ] && [ -f "$1/y.bin" ]
}

SRC="${KIRIN_REMOTE_REPO}/cann-recipes-harmony-infer/ops/ascendc/src/sobel_custom"
ART="${KIRIN_ARTIFACT}"
WORK="${ART}/source/sobel_custom"
MODEL_DIR="${ART}/model_910b2"
SCRIPT_DIR="${ART}/scripts"
LOG_DIR="${ART}/logs"

[ -d "${SRC}" ] || die "Sobel source not found: ${SRC}"
[ -d "${KIRIN_CANN_HOME}" ] || die "CANN home not found: ${KIRIN_CANN_HOME}"
[ -f "${SCRIPT_DIR}/run-om-acl.py" ] || die "run-om-acl.py was not copied to ${SCRIPT_DIR}"
[ -f "${SCRIPT_DIR}/compare-sobel-output.py" ] || die "compare-sobel-output.py was not copied to ${SCRIPT_DIR}"
[ -f "${SCRIPT_DIR}/validate-sobel-baseline.py" ] || die "validate-sobel-baseline.py was not copied to ${SCRIPT_DIR}"
[ -f "${SCRIPT_DIR}/sobel_custom_scalar_910b2.cpp" ] || die "sobel_custom_scalar_910b2.cpp was not copied to ${SCRIPT_DIR}"

mkdir -p "${WORK%/*}" "${MODEL_DIR}" "${LOG_DIR}"
cp -a "${SRC}" "${WORK}"
cd "${WORK}"

perl -0pi -e 's/"value": "kirinx90"/"value": "ascend910b"/g; s/"value": "customize"/"value": "'"${KIRIN_VENDOR_NAME}"'"/g' CMakePresets.json
perl -0pi -e 's/AddConfig\("kirinx90"/AddConfig("ascend910b"/g' op_host/sobel_custom.cpp
case "${KIRIN_KERNEL_MODE}" in
  vector-fixed)
    perl -0pi -e 's/template <typename T, typename U>\n__aicore__ inline T CeilDiv\(T x, U y\)\n\{\n    return y == 0 \? x : \(1 \+ \(\(x - y\) \+ \(y - 2\) - 1\) \/ \(y - 2\)\);\n\}/template <typename T, typename U>\n__aicore__ inline T CeilDiv(T x, U y)\n{\n    return y == 0 ? x : (1 + ((x - y) + (y - 2) - 1) \/ (y - 2));\n}\n\n__aicore__ inline uint32_t DivCeil(uint32_t x, uint32_t y)\n{\n    return y == 0 ? x : (x + y - 1) \/ y;\n}\n\n__aicore__ inline uint32_t Ceil32Blocks(uint32_t bytes)\n{\n    return DivCeil(bytes, 32);\n}/g' op_kernel/sobel_custom_base.h
    perl -0pi -e 's/constexpr int32_t BUFFER_NUM = 2; \/\/ tensor num for each queue/constexpr int32_t BUFFER_NUM = 2; \/\/ tensor num for each queue\nconstexpr uint32_t TRANSPOSE_TMP_BYTES = 8192;/g' op_kernel/sobel_custom.cpp
    perl -0pi -e 's/pipe\.InitBuffer\(calcBuf, \(tileLength \* 4 \+ grayLength \* 14 \+ w \* 12\)\);/pipe.InitBuffer(calcBuf, (TRANSPOSE_TMP_BYTES + tileLength * 4 + grayLength * 14 + w * 12));/g' op_kernel/sobel_custom.cpp
    perl -0pi -e 's/calcBuf\.GetWithOffset<([^>]+)>\(([^,\n]+), ([^)]+)\)/calcBuf.GetWithOffset<$1>($2, TRANSPOSE_TMP_BYTES + $3)/g' op_kernel/sobel_custom.cpp
    perl -0pi -e 's/SobelCustom::CeilDiv\(([^,\n]+?)\s*,\s*32\)/SobelCustom::Ceil32Blocks($1)/g; s/uint32_t offset;\n/uint32_t offset = 0;\n/g; s/\(0 < j < cntW - 1\)/(0 < j \&\& j < cntW - 1)/g; s/AscendC::Transpose\(tempTensor0, xLocal, stackBuffer, transposeParams\);/#if __NPU_ARCH__ == 3113\n        AscendC::Transpose4DImpl(tempTensor0, xLocal, stackBuffer, transposeParams);\n#else\n        AscendC::Transpose(tempTensor0, xLocal, stackBuffer, transposeParams);\n#endif/g; s/^\s*AscendC::Cast\(newindex, index, AscendC::RoundMode::CAST_NONE, w - 2\);\n//mg; s/^\s*AscendC::Cast\(newindex1, index1, AscendC::RoundMode::CAST_NONE, w - 2\);\n//mg' op_kernel/sobel_custom.cpp
    perl -0pi -e 's/AscendC::Add\(tmpBuf0, dx, dy, w \* \(h - 2\)\);\n        \/\/ half->u8/AscendC::Add(tmpBuf0, dx, dy, w * (h - 2));\n        AscendC::Mins(tmpBuf0, tmpBuf0, half(255), w * (h - 2));\n        \/\/ half->u8/g' op_kernel/sobel_custom.cpp
    ;;
  scalar-correctness)
    cp "${SCRIPT_DIR}/sobel_custom_scalar_910b2.cpp" op_kernel/sobel_custom.cpp
    ;;
  *)
    die "unsupported KIRIN_KERNEL_MODE=${KIRIN_KERNEL_MODE}"
    ;;
esac
perl -0pi -e "s/-j\\\$\\(nproc\\)/-j${KIRIN_BUILD_JOBS}/g" build_and_install.sh
perl -0pi -e 's#^export LD_PRELOAD=.*\n##m' build_and_install.sh
perl -0pi -e 's/\ncd build_out\nOS_ID=/\nif [ "\${KIRIN_SKIP_OPP_INSTALL:-0}" = "1" ]; then\n    echo "INFO: skip custom op run package install"\n    exit 0\nfi\n\ncd build_out\nOS_ID=/g' build_and_install.sh

compute_unit_block="$(grep -A4 '"ASCEND_COMPUTE_UNIT"' CMakePresets.json || true)"
vendor_block="$(grep -A4 '"vendor_name"' CMakePresets.json || true)"
grep -q '"value": "ascend910b"' <<<"${compute_unit_block}" || die "failed to set ASCEND_COMPUTE_UNIT=ascend910b"
grep -q '"value": "'"${KIRIN_VENDOR_NAME}"'"' <<<"${vendor_block}" || die "failed to set vendor_name=${KIRIN_VENDOR_NAME}"
grep -q 'AddConfig("ascend910b"' op_host/sobel_custom.cpp || die "failed to set AddConfig(ascend910b)"
case "${KIRIN_KERNEL_MODE}" in
  vector-fixed)
    grep -q 'Ceil32Blocks' op_kernel/sobel_custom_base.h || die "failed to add true 32-byte ceil helper"
    grep -q 'TRANSPOSE_TMP_BYTES = 8192' op_kernel/sobel_custom.cpp || die "failed to add dedicated transpose scratch bytes"
    grep -q 'TRANSPOSE_TMP_BYTES + tileLength \* 4' op_kernel/sobel_custom.cpp || die "failed to grow calcBuf for transpose scratch"
    grep -q 'GetWithOffset<T>(tileLength, TRANSPOSE_TMP_BYTES + tileLength \* 1)' op_kernel/sobel_custom.cpp || die "failed to move tempTensor0 behind transpose scratch"
    grep -q 'Transpose4DImpl' op_kernel/sobel_custom.cpp || die "failed to apply transpose compatibility patch"
    grep -q 'Mins(tmpBuf0, tmpBuf0, half(255)' op_kernel/sobel_custom.cpp || die "failed to apply uint8 clamp patch"
    grep -q 'cntH = SobelCustom::CeilDiv(this->H, h);' op_kernel/sobel_custom.cpp || die "unexpected output-height tile count patch"
    grep -q 'cntW = SobelCustom::CeilDiv(this->W, w);' op_kernel/sobel_custom.cpp || die "unexpected output-width tile count patch"
    grep -q 'Ceil32Blocks(w \* c \* sizeof(T)' op_kernel/sobel_custom.cpp || die "failed to patch CopyIn 32-byte stride helper"
    grep -q 'Ceil32Blocks(w \* sizeof(T)' op_kernel/sobel_custom.cpp || die "failed to patch CopyOut 32-byte stride helper"
    grep -q '0 < j && j < cntW - 1' op_kernel/sobel_custom.cpp || die "failed to patch chained j comparison"
    if grep -q 'Cast(newindex, index' op_kernel/sobel_custom.cpp || grep -q 'Cast(newindex1, index1' op_kernel/sobel_custom.cpp; then
      die "unsupported int32->uint32 index casts are still present"
    fi
    if grep -q 'CeilDiv(this->H - 2' op_kernel/sobel_custom.cpp || grep -q 'CeilDiv(this->W - 2' op_kernel/sobel_custom.cpp; then
      die "bad tile count patch is still present"
    fi
    ;;
  scalar-correctness)
    grep -q 'SOBEL_910B2_SCALAR_KERNEL' op_kernel/sobel_custom.cpp || die "failed to install scalar 910B2 kernel template"
    grep -q 'SetValue(outputOffset' op_kernel/sobel_custom.cpp || die "scalar 910B2 kernel template is missing direct output write"
    ;;
esac

set +e +u
source "${KIRIN_CANN_HOME}/set_env.sh" >"${LOG_DIR}/set_env.stdout" 2>"${LOG_DIR}/set_env.stderr"
source_status=$?
set -euo pipefail
[ "${source_status}" -eq 0 ] || die "failed to source ${KIRIN_CANN_HOME}/set_env.sh"
export ASCEND_HOME_PATH="${KIRIN_CANN_HOME}"

log "building custom OPP package without global install"
set +e
KIRIN_SKIP_OPP_INSTALL=1 bash build_and_install.sh package >"${LOG_DIR}/build_and_install.log" 2>&1
build_status=$?
set -e
printf 'build_status=%s\n' "${build_status}" | tee "${ART}/build.status"
if [ "${build_status}" -ne 0 ]; then
  tail -160 "${LOG_DIR}/build_and_install.log" >&2
  exit "${build_status}"
fi

CUSTOM_OPP="$(find "${WORK}/build_out/_CPack_Packages/Linux/External" -path "*/packages/vendors/${KIRIN_VENDOR_NAME}" -type d -print -quit)"
[ -n "${CUSTOM_OPP}" ] || die "custom OPP vendor package not found for ${KIRIN_VENDOR_NAME}"
RUN_PACKAGE="$(find "${WORK}/build_out" -maxdepth 1 -type f -name 'custom_opp_*.run' -print -quit)"
[ -n "${RUN_PACKAGE}" ] || die "custom_opp run package not found"

if [ -n "${KIRIN_FIXTURES_DIR}" ]; then
  FIXTURES_DIR="${KIRIN_FIXTURES_DIR}"
elif have_fixtures "${KIRIN_REMOTE_REPO}/cann-recipes-harmony-infer/ops/ascendc/src/sobel_custom/test"; then
  FIXTURES_DIR="${KIRIN_REMOTE_REPO}/cann-recipes-harmony-infer/ops/ascendc/src/sobel_custom/test"
elif have_fixtures "/data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test"; then
  FIXTURES_DIR="/data1/z84378291/artifacts/kirin9030_sobel_build_20260804_081850/sobel_custom/test"
else
  FIXTURES_DIR=""
fi

if [ -n "${FIXTURES_DIR}" ]; then
  log "using fixtures from ${FIXTURES_DIR}"
  cp "${FIXTURES_DIR}/SobelCustom.onnx" "${FIXTURES_DIR}/x.bin" "${FIXTURES_DIR}/y.bin" "${MODEL_DIR}/"
else
  log "generating fixtures from source test scripts"
  cd "${WORK}/test"
  if ! python3 - <<'PY' >"${LOG_DIR}/fixture_dependency_check.log" 2>&1
import cv2
import numpy
import onnx
PY
  then
    sed -n '1,120p' "${LOG_DIR}/fixture_dependency_check.log" >&2 || true
    die "fixture generation requires python3 modules: onnx, cv2, numpy; pass --fixtures-dir to reuse generated fixtures"
  fi
  python3 create_onnx.py >"${LOG_DIR}/create_onnx.log" 2>&1
  python3 gen_data.py >"${LOG_DIR}/gen_data.log" 2>&1
  cp SobelCustom.onnx x.bin y.bin "${MODEL_DIR}/"
fi

cd "${MODEL_DIR}"
log "validating golden/reference baseline with numpy"
set +e
python3 "${SCRIPT_DIR}/validate-sobel-baseline.py" --input ./x.bin --golden ./y.bin >"${LOG_DIR}/validate_baseline.log" 2>&1
baseline_status=$?
set -e
printf 'baseline_status=%s\n' "${baseline_status}" | tee "${ART}/baseline.status"
if [ "${baseline_status}" -ne 0 ]; then
  tail -160 "${LOG_DIR}/validate_baseline.log" >&2
  exit "${baseline_status}"
fi

export ASCEND_CUSTOM_OPP_PATH="${CUSTOM_OPP}"
export LD_LIBRARY_PATH="${CUSTOM_OPP}/op_api/lib:${LD_LIBRARY_PATH:-}"

log "running ATC for Ascend910B2"
set +e
atc --model=./SobelCustom.onnx --framework=5 --output=./SobelCustom_Ascend910B2 --soc_version=Ascend910B2 >"${LOG_DIR}/atc.stdout" 2>"${LOG_DIR}/atc.stderr"
atc_status=$?
set -e
printf 'atc_status=%s\n' "${atc_status}" | tee "${ART}/atc.status"
if [ "${atc_status}" -ne 0 ]; then
  sed -n '1,220p' "${LOG_DIR}/atc.stdout" >&2 || true
  sed -n '1,220p' "${LOG_DIR}/atc.stderr" >&2 || true
  exit "${atc_status}"
fi
[ -f "${MODEL_DIR}/SobelCustom_Ascend910B2.om" ] || die "ATC succeeded but .om was not produced"

compare_status="skipped"
run_status="skipped"
if [ "${KIRIN_RUN_ACL}" = "1" ]; then
  log "running OM on Ascend device ${KIRIN_DEVICE_ID}"
  rm -rf "${MODEL_DIR}/acl_outputs"
  mkdir -p "${MODEL_DIR}/acl_outputs"
  set +e
  python3 "${SCRIPT_DIR}/run-om-acl.py" \
    --device-id "${KIRIN_DEVICE_ID}" \
    --model "${MODEL_DIR}/SobelCustom_Ascend910B2.om" \
    --input "${MODEL_DIR}/x.bin" \
    --output-dir "${MODEL_DIR}/acl_outputs" >"${LOG_DIR}/acl_run.log" 2>&1
  run_status=$?
  set -e
  printf 'run_status=%s\n' "${run_status}" | tee "${ART}/run.status"
  if [ "${run_status}" -ne 0 ]; then
    tail -160 "${LOG_DIR}/acl_run.log" >&2
    exit "${run_status}"
  fi

  if [ "${KIRIN_RUN_COMPARE}" = "1" ]; then
    log "comparing ACL output against y.bin"
    compare_args=(--output "${MODEL_DIR}/acl_outputs/output_0.bin" --golden "${MODEL_DIR}/y.bin" --input "${MODEL_DIR}/x.bin" --max-abs-diff "${KIRIN_MAX_ABS_DIFF}")
    if [ -n "${KIRIN_MAX_DIFF_RATE}" ]; then
      compare_args+=(--max-diff-rate "${KIRIN_MAX_DIFF_RATE}")
    fi
    set +e
    python3 "${SCRIPT_DIR}/compare-sobel-output.py" "${compare_args[@]}" >"${LOG_DIR}/compare.log" 2>&1
    compare_status=$?
    set -e
    printf 'compare_status=%s\n' "${compare_status}" | tee "${ART}/compare.status"
  fi
else
  printf 'run_status=skipped\n' | tee "${ART}/run.status"
fi

find "${ART}" -maxdepth 5 -type f -printf '%p %s bytes\n' | sort >"${ART}/files.txt"
{
  printf 'artifact=%s\n' "${ART}"
  printf 'source=%s\n' "${WORK}"
  printf 'cann_home=%s\n' "${KIRIN_CANN_HOME}"
  printf 'container=%s\n' "$(hostname)"
  printf 'compute_unit=ascend910b\n'
  printf 'soc_version=Ascend910B2\n'
  printf 'kernel_mode=%s\n' "${KIRIN_KERNEL_MODE}"
  printf 'vendor_name=%s\n' "${KIRIN_VENDOR_NAME}"
  printf 'custom_opp=%s\n' "${CUSTOM_OPP}"
  printf 'run_package=%s\n' "${RUN_PACKAGE}"
  printf 'fixtures_dir=%s\n' "${FIXTURES_DIR:-generated}"
  printf 'device_id=%s\n' "${KIRIN_DEVICE_ID}"
  printf 'om=%s\n' "${MODEL_DIR}/SobelCustom_Ascend910B2.om"
  printf 'input=%s\n' "${MODEL_DIR}/x.bin"
  printf 'golden=%s\n' "${MODEL_DIR}/y.bin"
  printf 'output=%s\n' "${MODEL_DIR}/acl_outputs/output_0.bin"
  printf 'build_status=%s\n' "${build_status}"
  printf 'baseline_status=%s\n' "${baseline_status}"
  printf 'atc_status=%s\n' "${atc_status}"
  printf 'run_status=%s\n' "${run_status}"
  printf 'compare_status=%s\n' "${compare_status}"
} >"${ART}/manifest.env"

log "artifact=${ART}"
log "om=${MODEL_DIR}/SobelCustom_Ascend910B2.om"
if [ "${compare_status}" != "skipped" ] && [ "${compare_status}" != "0" ]; then
  tail -220 "${LOG_DIR}/compare.log" >&2 || true
  exit "${compare_status}"
fi
REMOTE_SCRIPT
REMOTE_STATUS=$?
set -e

if [ -n "${PULL_TO_DIR}" ]; then
  mkdir -p "${PULL_TO_DIR}"
  scp -q \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/manifest.env" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/build.status" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/baseline.status" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/atc.status" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/run.status" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_910b2/SobelCustom_Ascend910B2.om" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_910b2/SobelCustom.onnx" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_910b2/x.bin" \
    "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_910b2/y.bin" \
    "${PULL_TO_DIR}/" || true
  scp -q "${REMOTE_HOST}:${REMOTE_ARTIFACT}/logs/"*.log "${PULL_TO_DIR}/" || true
  if [ "${RUN_ACL}" = "1" ]; then
    scp -q "${REMOTE_HOST}:${REMOTE_ARTIFACT}/model_910b2/acl_outputs/output_0.bin" "${PULL_TO_DIR}/" || true
  fi
  if [ "${RUN_COMPARE}" = "1" ]; then
    scp -q "${REMOTE_HOST}:${REMOTE_ARTIFACT}/compare.status" "${PULL_TO_DIR}/" || true
  fi
  log "pulled evidence to ${PULL_TO_DIR}"
fi

exit "${REMOTE_STATUS}"
