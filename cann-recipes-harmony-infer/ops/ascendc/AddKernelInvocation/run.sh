# Copyright (c) 2025, HUAWEI CORPORATION.  All rights reserved.
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
export PRINT_TIK_MEM_ACCESS=FALSE

CURRENT_DIR=$(
    cd $(dirname ${BASH_SOURCE:-$0})
    pwd
); cd $CURRENT_DIR

declare -A VersionMap
#kirin
VersionMap["KirinX90"]="KirinX90"
VersionMap["Kirin9030"]="Kirin9030"
#迁移算子修改点：FILE_NAME控制编译获得的二进制名
FILE_NAME="add"

SHORT=r:,v:,i:,
LONG=run-mode:,soc-version:,install-path:,
OPTS=$(getopt -a --options $SHORT --longoptions $LONG -- "$@")
eval set -- "$OPTS"

while :; do
    case "$1" in
    -r | --run-mode)
        RUN_MODE="$2"
        shift 2
        ;;
    -v | --soc-version)
        SOC_VERSION="$2"
        shift 2
        ;;
    -i | --install-path)
        ASCEND_INSTALL_PATH="$2"
        shift 2
        ;;
    --)
        shift
        break
        ;;
    *)
        echo "[ERROR] Unexpected option: $1"
        break
        ;;
    esac
done

if [ -n "$ASCEND_TOOLKIT_HOME" ]; then
    _ASCEND_INSTALL_PATH=$ASCEND_TOOLKIT_HOME
elif [ -n "$ASCEND_HOME_PATH" ]; then
    _ASCEND_INSTALL_PATH=$ASCEND_HOME_PATH
else
    if [ -d "$HOME/Ascend/ascend-toolkit/latest" ]; then
        _ASCEND_INSTALL_PATH=$HOME/Ascend/ascend-toolkit/latest
    else
        _ASCEND_INSTALL_PATH=/usr/local/Ascend/ascend-toolkit/latest
    fi
fi


if [[ " ${!VersionMap[*]} " != *" $SOC_VERSION "* ]]; then
    echo "ERROR: SOC_VERSION should be in [${!VersionMap[*]}]"
    exit -1
fi
_SOC_VERSION=${VersionMap[$SOC_VERSION]}

if  [ $_SOC_VERSION"x" = "KirinX90x" ] || [ $_SOC_VERSION"x" = "Kirin9030x" ]; then
    CORE_TYPE="AiCore"
fi

RUN_MODE_LIST="cpu sim npu"
if [[ " $RUN_MODE_LIST " != *" $RUN_MODE "* ]]; then
    echo "ERROR: RUN_MODE error, This sample only support specify cpu, sim or npu!"
    exit -1
fi

#cpu暂不支持
if [ "${RUN_MODE}" = "cpu" ] && {  [ $_SOC_VERSION"x" = "KirinX90x" ] || [ $_SOC_VERSION"x" = "Kirin9030x" ];}; then
    echo "Kirin Soc Currently not support cpu!"
    exit -1
fi


# in case of running op in simulator, use stub so instead
if [ "${RUN_MODE}" = "sim" ]; then
    export LD_LIBRARY_PATH=$_ASCEND_INSTALL_PATH/x86_64-linux/simulator/${_SOC_VERSION}/lib:$LD_LIBRARY_PATH
    if [ ! $CAMODEL_LOG_PATH ]; then
        export CAMODEL_LOG_PATH=./sim_log
    fi
    rm -rf $CAMODEL_LOG_PATH
    mkdir -p $CAMODEL_LOG_PATH
fi

set -e
rm -rf build *_cpu *_sim *_npu cceprint npuchk *.vcd

rm -rf build
mkdir -p build
cmake -B build \
    -Dsmoke_testcase=${FILE_NAME} \
    -DASCEND_PRODUCT_TYPE=${_SOC_VERSION} \
    -DASCEND_CORE_TYPE=${CORE_TYPE} \
    -DASCEND_RUN_MODE=${RUN_MODE} \
    -DASCEND_INSTALL_PATH=${_ASCEND_INSTALL_PATH}
cmake --build build --target ${FILE_NAME}_${RUN_MODE}
if [ $? -ne 0 ]; then
    echo "ERROR: compile op on failed!"
    exit -1
fi
echo "INFO: compile op on ${RUN_MODE} succeed!"

rm -rf input/*.bin output/*.bin
python3 scripts/gen_data.py

if   [ $_SOC_VERSION"x" = "KirinX90x" ] || [ $_SOC_VERSION"x" = "Kirin9030x" ]; then
#kirin dependent libascendcl already in simulator/${_SOC_VERSION}/lib
     ./${FILE_NAME}_${RUN_MODE}
else
#ascend dependent libascendcl in public dir,placeholder
    (export LD_LIBRARY_PATH=${_ASCEND_INSTALL_PATH}/x86_64-linux/lib64:$LD_LIBRARY_PATH && ./${FILE_NAME}_${RUN_MODE})
fi

if [ $? -ne 0 ]; then
    echo "ERROR: execute op on ${RUN_MODE} failed!"
    exit -1
fi
echo "INFO: execute op on ${RUN_MODE} succeed!"
python3 scripts/verify_result.py output/output_z.bin output/golden.bin


#如果要保留仿真性能结果文件夹，请注释删除下面这行；
rm -rf *log *.vcd