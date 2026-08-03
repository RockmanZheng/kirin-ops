#!/usr/bin/env bash
# Run a command inside the dedicated z84378291 CANN container.

set -euo pipefail

if [ "$#" -eq 0 ]; then
  echo "usage: $0 '<command to run in container>'" >&2
  exit 2
fi

HOST="${KIRIN_NPU_HOST:-npu-group-3}"
CONTAINER="${KIRIN_CANN_CONTAINER:-z84378291-kirin-cann91}"
REMOTE_WORKDIR="${KIRIN_REMOTE_WORKDIR:-/data1/z84378291/kirin-ops}"
PY_VENV="${KIRIN_PY_VENV:-/data1/z84378291/cache/py311-cann-harmony}"
CMD="$*"

exec ssh "${HOST}" \
  "docker exec ${CONTAINER} bash -lc 'cd ${REMOTE_WORKDIR}; source /usr/local/Ascend/cann/set_env.sh >/dev/null 2>&1 || true; source ${PY_VENV}/bin/activate; ${CMD}'"
