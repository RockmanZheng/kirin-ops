#!/usr/bin/env bash
# Source this file before running local HarmonyOS build diagnostics on macOS.

set -euo pipefail

SCRIPT_PATH="${BASH_SOURCE[0]:-${0}}"
SCRIPT_DIR="$(cd "$(dirname "${SCRIPT_PATH}")" && pwd)"
KIRIN_OPS_HOME="$(cd "${SCRIPT_DIR}/.." && pwd)"
COMMAND_LINE_TOOLS_DIR="${KIRIN_COMMAND_LINE_TOOLS_DIR:-${KIRIN_OPS_HOME}/command-line-tools}"
DEVECO_APP_DIR="${DEVECO_STUDIO_APP_DIR:-/Applications/DevEco-Studio.app}"

if [ -d /opt/homebrew/opt/openjdk@17/bin ]; then
  export JAVA_HOME="/opt/homebrew/opt/openjdk@17/libexec/openjdk.jdk/Contents/Home"
  export PATH="/opt/homebrew/opt/openjdk@17/bin:${PATH}"
  export CPPFLAGS="-I/opt/homebrew/opt/openjdk@17/include ${CPPFLAGS:-}"
fi

if [ -d /opt/homebrew/bin ]; then
  export PATH="/opt/homebrew/bin:${PATH}"
fi

if [ -d "${DEVECO_APP_DIR}" ]; then
  export PATH="${DEVECO_APP_DIR}/Contents/tools/ohpm/bin:${DEVECO_APP_DIR}/Contents/tools/node/bin:${DEVECO_APP_DIR}/Contents/sdk/default/openharmony/toolchains:${PATH}"
fi

if [ -d "${COMMAND_LINE_TOOLS_DIR}" ]; then
  export OHOS_SDK_HOME="${OHOS_SDK_HOME:-${COMMAND_LINE_TOOLS_DIR}/sdk/default/openharmony}"
  export HMS_SDK_HOME="${HMS_SDK_HOME:-${COMMAND_LINE_TOOLS_DIR}/sdk/default/hms}"
  export PATH="${COMMAND_LINE_TOOLS_DIR}/bin:${COMMAND_LINE_TOOLS_DIR}/ohpm/bin:${COMMAND_LINE_TOOLS_DIR}/hvigor/bin:${COMMAND_LINE_TOOLS_DIR}/tool/node/bin:${OHOS_SDK_HOME}/toolchains:${PATH}"
fi

echo "Local macOS HarmonyOS environment:"
echo "  KIRIN_OPS_HOME=${KIRIN_OPS_HOME}"
echo "  COMMAND_LINE_TOOLS_DIR=${COMMAND_LINE_TOOLS_DIR}"
echo "  DEVECO_APP_DIR=${DEVECO_APP_DIR}"
echo "  JAVA_HOME=${JAVA_HOME:-not set}"
echo "  OHOS_SDK_HOME=${OHOS_SDK_HOME:-not set}"
echo "  HMS_SDK_HOME=${HMS_SDK_HOME:-not set}"
echo "  java=$(command -v java || true)"
echo "  cmake=$(command -v cmake || true)"
echo "  ohpm=$(command -v ohpm || true)"
echo "  hvigorw=$(command -v hvigorw || true)"
echo "  hdc=$(command -v hdc || true)"
