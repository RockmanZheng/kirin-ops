#!/usr/bin/env bash
# Open the existing HarmonyOS sample project in DevEco Studio.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_DIR="${ROOT}/cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble"
DEVECO_APP_DIR="${DEVECO_STUDIO_APP_DIR:-/Applications/DevEco-Studio.app}"

if [ ! -d "${DEVECO_APP_DIR}" ]; then
  echo "DevEco Studio not found at ${DEVECO_APP_DIR}" >&2
  exit 1
fi

if [ ! -f "${PROJECT_DIR}/build-profile.json5" ] || [ ! -f "${PROJECT_DIR}/hvigorfile.ts" ] || [ ! -f "${PROJECT_DIR}/oh-package.json5" ]; then
  echo "HarmonyOS project markers are missing under ${PROJECT_DIR}" >&2
  exit 1
fi

echo "Opening DevEco project: ${PROJECT_DIR}"
open -a "${DEVECO_APP_DIR}" "${PROJECT_DIR}"
