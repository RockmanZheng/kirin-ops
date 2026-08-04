#!/usr/bin/env bash
# Run a command inside the dedicated x86 mobile-station container.

set -euo pipefail

if [ "$#" -eq 0 ]; then
  echo "usage: $0 '<command to run in mobile-station container>'" >&2
  exit 2
fi

HOST="${KIRIN_NPU_HOST:-npu-group-3}"
CONTAINER="${KIRIN_MOBILE_STATION_CONTAINER:-z84378291-kirin-mobile-x86}"
REMOTE_WORKDIR="${KIRIN_REMOTE_WORKDIR:-/data1/z84378291/kirin-ops}"
CANN_HOME="${KIRIN_MOBILE_STATION_CANN_HOME:-/opt/Ascend/cann-9.0.0}"
CMD="$*"

exec ssh "${HOST}" \
  "docker exec ${CONTAINER} bash -lc 'cd ${REMOTE_WORKDIR}; source ${CANN_HOME}/set_env.sh >/dev/null 2>&1 || true; export ASCEND_HOME_PATH=${CANN_HOME}; export ASCEND_CUSTOM_OPP_PATH=${CANN_HOME}/opp/vendors/customize; ${CMD}'"
