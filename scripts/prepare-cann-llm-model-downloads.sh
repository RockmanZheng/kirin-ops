#!/usr/bin/env bash
# Prepare Hugging Face model download commands for the CANN LLM Engine demo.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MODEL="${MODEL:-qwen25-1b5}"
OUT_DIR="${OUT_DIR:-${ROOT}/artifacts/cann-llm/models}"
CACHE_DIR="${CACHE_DIR:-}"
REVISION="${REVISION:-main}"
HF_BIN="${HF_BIN:-}"
MAX_WORKERS="${MAX_WORKERS:-8}"
COMMAND_FILE="${COMMAND_FILE:-}"
EXECUTE=0
PRINT_TOOLCHAIN_LINKS=1
WRITE_CHECKSUMS=1

usage() {
  cat <<'USAGE'
usage: scripts/prepare-cann-llm-model-downloads.sh [options]

Prints Hugging Face model download commands for CANN LLM Engine reproduction.
The default mode is dry-run: it does not download anything. Pass --execute only
when you are ready to fetch the model files.

Models:
  qwen25-1b5       Qwen/Qwen2.5-1.5B
  deepseek-1b5     deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B
  glm-1b5          zai-org/glm-edge-1.5b-chat
  qwen25-7b        Qwen/Qwen2.5-7B-Instruct
  qwen3-8b         Qwen/Qwen3-8B
  all              All models listed above

Options:
  --model NAME         Model alias to prepare. Default: qwen25-1b5.
  --out-dir DIR        Local model root. Default: artifacts/cann-llm/models.
  --cache-dir DIR      Optional Hugging Face cache directory.
  --revision REV       Hugging Face revision. Default: main.
  --hf-bin PATH        hf or huggingface-cli binary. Auto-detected when executing.
  --max-workers N      Hugging Face parallel download workers. Default: 8.
  --command-file PATH  Write the generated download commands to a shell file.
  --no-checksums       Do not generate SHA256SUMS after --execute downloads.
  --no-toolchain-links Do not print CANN Kit / HiAI DDK manual download links.
  --execute            Actually run downloads. Omit this to stay in dry-run mode.
  -h, --help           Show this help.

Examples:
  scripts/prepare-cann-llm-model-downloads.sh
  scripts/prepare-cann-llm-model-downloads.sh --model all --command-file /tmp/cann-llm-downloads.sh
  HF_TOKEN=... scripts/prepare-cann-llm-model-downloads.sh --model qwen25-1b5 --execute
USAGE
}

log() {
  printf '[cann-llm-downloads] %s\n' "$*"
}

warn() {
  printf '[cann-llm-downloads] WARN: %s\n' "$*" >&2
}

die() {
  printf '[cann-llm-downloads] ERROR: %s\n' "$*" >&2
  exit 1
}

shell_quote() {
  printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
}

detect_hf_bin() {
  if [ -n "${HF_BIN}" ]; then
    printf '%s\n' "${HF_BIN}"
    return 0
  fi
  if command -v hf >/dev/null 2>&1; then
    command -v hf
    return 0
  fi
  if command -v huggingface-cli >/dev/null 2>&1; then
    command -v huggingface-cli
    return 0
  fi
  if [ "${EXECUTE}" -eq 1 ]; then
    die "neither hf nor huggingface-cli was found in PATH"
  fi
  printf 'hf\n'
}

model_repo() {
  case "$1" in
    qwen25-1b5) printf '%s\n' 'Qwen/Qwen2.5-1.5B' ;;
    deepseek-1b5) printf '%s\n' 'deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B' ;;
    glm-1b5) printf '%s\n' 'zai-org/glm-edge-1.5b-chat' ;;
    qwen25-7b) printf '%s\n' 'Qwen/Qwen2.5-7B-Instruct' ;;
    qwen3-8b) printf '%s\n' 'Qwen/Qwen3-8B' ;;
    *) die "unknown model alias: $1" ;;
  esac
}

model_dir_name() {
  case "$1" in
    qwen25-1b5) printf '%s\n' 'Qwen2.5-1.5B' ;;
    deepseek-1b5) printf '%s\n' 'DeepSeek-R1-Distill-Qwen-1.5B' ;;
    glm-1b5) printf '%s\n' 'glm-edge-1.5b-chat' ;;
    qwen25-7b) printf '%s\n' 'Qwen2.5-7B-Instruct' ;;
    qwen3-8b) printf '%s\n' 'Qwen3-8B' ;;
    *) die "unknown model alias: $1" ;;
  esac
}

append_command_line() {
  local command_line="$1"
  if [ -n "${COMMAND_FILE}" ]; then
    printf '%s\n' "${command_line}" >> "${COMMAND_FILE}"
  fi
}

build_command_line() {
  local bin="$1"
  local repo="$2"
  local local_dir="$3"
  local line

  line="$(shell_quote "${bin}") download --repo-type model --revision $(shell_quote "${REVISION}") --local-dir $(shell_quote "${local_dir}") --max-workers $(shell_quote "${MAX_WORKERS}")"
  if [ -n "${CACHE_DIR}" ]; then
    line="${line} --cache-dir $(shell_quote "${CACHE_DIR}")"
  fi
  line="${line} $(shell_quote "${repo}")"
  printf '%s\n' "${line}"
}

run_download() {
  local bin="$1"
  local repo="$2"
  local local_dir="$3"
  local cmd

  cmd=("${bin}" download --repo-type model --revision "${REVISION}" --local-dir "${local_dir}" --max-workers "${MAX_WORKERS}")
  if [ -n "${CACHE_DIR}" ]; then
    cmd+=(--cache-dir "${CACHE_DIR}")
  fi
  cmd+=("${repo}")

  mkdir -p "${local_dir}"
  log "downloading ${repo} -> ${local_dir}"
  "${cmd[@]}"

  if [ "${WRITE_CHECKSUMS}" -eq 1 ]; then
    log "writing ${local_dir}/SHA256SUMS"
    (
      cd "${local_dir}"
      checksums_tmp="$(mktemp "${TMPDIR:-/tmp}/cann-llm-checksums.XXXXXX")"
      find . -type f ! -name SHA256SUMS -print0 \
        | LC_ALL=C sort -z \
        | xargs -0 shasum -a 256 > "${checksums_tmp}"
      mv "${checksums_tmp}" SHA256SUMS
    )
  fi
}

while [ "$#" -gt 0 ]; do
  case "$1" in
    --model)
      [ "$#" -ge 2 ] || die "--model requires a value"
      MODEL="$2"
      shift 2
      ;;
    --out-dir)
      [ "$#" -ge 2 ] || die "--out-dir requires a path"
      OUT_DIR="$2"
      shift 2
      ;;
    --cache-dir)
      [ "$#" -ge 2 ] || die "--cache-dir requires a path"
      CACHE_DIR="$2"
      shift 2
      ;;
    --revision)
      [ "$#" -ge 2 ] || die "--revision requires a value"
      REVISION="$2"
      shift 2
      ;;
    --hf-bin)
      [ "$#" -ge 2 ] || die "--hf-bin requires a path"
      HF_BIN="$2"
      shift 2
      ;;
    --max-workers)
      [ "$#" -ge 2 ] || die "--max-workers requires a value"
      MAX_WORKERS="$2"
      shift 2
      ;;
    --command-file)
      [ "$#" -ge 2 ] || die "--command-file requires a path"
      COMMAND_FILE="$2"
      shift 2
      ;;
    --no-checksums)
      WRITE_CHECKSUMS=0
      shift
      ;;
    --no-toolchain-links)
      PRINT_TOOLCHAIN_LINKS=0
      shift
      ;;
    --execute)
      EXECUTE=1
      shift
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

case "${MAX_WORKERS}" in
  ''|*[!0-9]*) die "--max-workers must be a positive integer" ;;
esac
[ "${MAX_WORKERS}" -gt 0 ] || die "--max-workers must be greater than zero"

if [ "${MODEL}" = "all" ]; then
  MODELS=(qwen25-1b5 deepseek-1b5 glm-1b5 qwen25-7b qwen3-8b)
else
  MODELS=("${MODEL}")
fi

HF_BIN_RESOLVED="$(detect_hf_bin)"

if [ -n "${COMMAND_FILE}" ]; then
  mkdir -p "$(dirname "${COMMAND_FILE}")"
  {
    printf '#!/usr/bin/env bash\n'
    printf 'set -euo pipefail\n\n'
    printf '# Generated by scripts/prepare-cann-llm-model-downloads.sh\n'
    printf '# Set HF_TOKEN in the environment first if a model requires authentication.\n\n'
  } > "${COMMAND_FILE}"
fi

log "mode: $([ "${EXECUTE}" -eq 1 ] && printf 'execute' || printf 'dry-run')"
log "hf binary: ${HF_BIN_RESOLVED}"
log "model root: ${OUT_DIR}"
log "revision: ${REVISION}"

for alias in "${MODELS[@]}"; do
  repo="$(model_repo "${alias}")"
  local_dir="${OUT_DIR}/$(model_dir_name "${alias}")"
  command_line="$(build_command_line "${HF_BIN_RESOLVED}" "${repo}" "${local_dir}")"

  log "${alias}: ${repo}"
  printf '  %s\n' "${command_line}"
  append_command_line "${command_line}"

  if [ "${EXECUTE}" -eq 1 ]; then
    run_download "${HF_BIN_RESOLVED}" "${repo}" "${local_dir}"
  fi
done

if [ -n "${COMMAND_FILE}" ]; then
  chmod 755 "${COMMAND_FILE}"
  log "wrote command file: ${COMMAND_FILE}"
fi

if [ "${PRINT_TOOLCHAIN_LINKS}" -eq 1 ]; then
  cat <<'LINKS'

Manual CANN / HiAI package download links:
  CANN Kit preparations:
    https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/cannkit-preparations
  LLM Engine headers and libhiai_llm_engine.so:
    https://developer.huawei.com/consumer/cn/doc/hiai-Library/ddk-download-0000001053590180

These vendor packages are not downloaded by this script because access, version
selection, and license flow may be account/browser gated.
LINKS
fi

if [ "${EXECUTE}" -eq 0 ]; then
  warn "dry-run only; no model files were downloaded"
fi
