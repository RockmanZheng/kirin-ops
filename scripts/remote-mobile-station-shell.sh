#!/usr/bin/env bash
# Open an interactive shell inside the dedicated x86 mobile-station container.

set -euo pipefail

HOST="${KIRIN_NPU_HOST:-npu-group-3}"
CONTAINER="${KIRIN_MOBILE_STATION_CONTAINER:-z84378291-kirin-mobile-x86}"
REMOTE_WORKDIR="${KIRIN_REMOTE_WORKDIR:-/data1/z84378291/kirin-ops}"
CANN_HOME="${KIRIN_MOBILE_STATION_CANN_HOME:-/opt/Ascend/cann-9.0.0}"

exec ssh -t "${HOST}" \
  "docker exec -it ${CONTAINER} bash -lc 'cd ${REMOTE_WORKDIR}; source ${CANN_HOME}/set_env.sh >/dev/null 2>&1 || true; export ASCEND_HOME_PATH=${CANN_HOME}; export ASCEND_CUSTOM_OPP_PATH=${CANN_HOME}/opp/vendors/customize; exec bash'"
