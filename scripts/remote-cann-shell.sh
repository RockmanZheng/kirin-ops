#!/usr/bin/env bash
# Open an interactive shell inside the dedicated z84378291 CANN container.

set -euo pipefail

HOST="${KIRIN_NPU_HOST:-npu-group-3}"
CONTAINER="${KIRIN_CANN_CONTAINER:-z84378291-kirin-cann91}"
REMOTE_WORKDIR="${KIRIN_REMOTE_WORKDIR:-/data1/z84378291/kirin-ops}"
PY_VENV="${KIRIN_PY_VENV:-/data1/z84378291/cache/py311-cann-harmony}"

exec ssh -t "${HOST}" \
  "docker exec -it ${CONTAINER} bash -lc 'cd ${REMOTE_WORKDIR}; source /usr/local/Ascend/cann/set_env.sh >/dev/null 2>&1 || true; source ${PY_VENV}/bin/activate; exec bash'"
