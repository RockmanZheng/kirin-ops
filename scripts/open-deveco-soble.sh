#!/usr/bin/env bash
# Open the existing HarmonyOS sample project in DevEco Studio.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREBUILT_PROJECT_DIR="${ROOT}/artifacts/prebuilt-demos/cannkit-codelab-sobeldemo-cpp/HDC_Sobel_Demo"
SOURCE_PROJECT_DIR="${ROOT}/cann-recipes-harmony-infer/harmony_infer/harmony_os_next/Soble"
PROJECT_DIR="${KIRIN_SOBLE_PROJECT_DIR:-}"
DEVECO_APP_DIR="${DEVECO_STUDIO_APP_DIR:-/Applications/DevEco-Studio.app}"

if [ -z "${PROJECT_DIR}" ]; then
  if [ -f "${PREBUILT_PROJECT_DIR}/entry/src/main/resources/rawfile/SobelCustom.omc" ]; then
    PROJECT_DIR="${PREBUILT_PROJECT_DIR}"
  else
    PROJECT_DIR="${SOURCE_PROJECT_DIR}"
  fi
fi

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
