# Copyright (c) 2025 Huawei Technologies Co., Ltd. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/bash
set -e

if [ ! $ASCEND_HOME_PATH ]; then
    ASCEND_HOME_PATH=/usr/local/Ascend/cann
    source $ASCEND_HOME_PATH/bin/setenv.bash
fi
export ASCEND_TENSOR_COMPILER_INCLUDE=$ASCEND_HOME_PATH/compiler/include
export LD_PRELOAD=${ASCEND_HOME_PATH}/x86_64-linux/lib64/libmmpa.so

if grep -q "/usr/local/Ascend/cann" "./CMakePresets.json"; then
    sed -i "s#/usr/local/Ascend/cann#$ASCEND_HOME_PATH#g" $(grep "/usr/local/Ascend/cann" -rl ./CMakePresets.json)
fi

if [ -z "$BASE_LIBS_PATH" ]; then
  if [ -z "$ASCEND_HOME_PATH" ]; then
    if [ -z "$ASCEND_AICPU_PATH" ]; then
      echo "please set env."
      exit 1
    else
      export ASCEND_HOME_PATH=$ASCEND_AICPU_PATH
    fi
  else
    export ASCEND_HOME_PATH=$ASCEND_HOME_PATH
  fi
else
  export ASCEND_HOME_PATH=$BASE_LIBS_PATH
fi
echo "using ASCEND_HOME_PATH: $ASCEND_HOME_PATH"
script_path=$(realpath $(dirname $0))

BUILD_DIR="build_out"
HOST_NATIVE_DIR="host_native_tiling"
mkdir -p build_out
rm -rf build_out/*

ENABLE_CROSS="-DENABLE_CROSS_COMPILE=True"
ENABLE_BINARY="-DENABLE_BINARY_PACKAGE=True"
ENABLE_LIBRARY="-DASCEND_PACK_SHARED_LIBRARY=True"
cmake_version=$(cmake --version | grep "cmake version" | awk '{print $3}')

target=package
if [ "$1"x != ""x ]; then target=$1; fi

cmake -S . -B "$BUILD_DIR" --preset=default
cmake --build "$BUILD_DIR" --target binary -j$(nproc)
cmake --build "$BUILD_DIR" --target $target -j$(nproc)

if [ $? -ne 0 ]; then
    echo "ERROR: build custom op run package failed!"
    return 1
fi

cd build_out
OS_ID=$(cat /etc/os-release | grep "^ID=" | awk -F= '{print $2}')
OS_ID=$(echo $OS_ID | sed -e 's/^"//' -e 's/"$//')
arch=$(uname -m)
./custom_opp_${OS_ID}_${arch}.run --quiet
if [ $? -ne 0 ]; then
    echo "ERROR: install custom op run package failed!"
    return 1
fi
echo "INFO: install custom op run package success!"