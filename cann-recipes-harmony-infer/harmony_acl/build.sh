# Copyright (C) 2020-2026. Huawei Technologies Co., Ltd. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the Apache License Version 2.0.
# You may not use this file except in compliance with the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# Apache License for more details at http://www.apache.org/licenses/LICENSE-2.0

#! /bin/bash
# 注意：脚本正常执行需下载一些依赖头文件，脚本自动进行下载，如果下载配置失败可以参考README.md手动下载

set -e

auto_download_header_with_git() {
    if [ -d headers_clone_dir ]; then
        return
    fi
    mkdir headers_clone_dir
    cd headers_clone_dir

    # clone runtime仓头文件
    git clone --filter=blob:none --sparse https://gitcode.com/cann/runtime.git runtime_inc_clone
    cd runtime_inc_clone
    git sparse-checkout set include/external/acl
    git checkout
    cd ..

    # clone ge仓头文件
    git clone --filter=blob:none --sparse https://gitcode.com/cann/ge.git ge_inc_clone
    cd ge_inc_clone
    git sparse-checkout set inc
    git checkout
    cd ..

    # clone metadef仓头文件
    git clone --filter=blob:none --sparse https://gitcode.com/cann/metadef.git metadef_inc_clone
    cd metadef_inc_clone
    git sparse-checkout set inc
    git checkout
    cd ..

    # clone json仓
    git clone https://gitcode.com/GitHub_Trending/js/json.git json_headers_clone
    cd ..
}

deal_after_download() {
    cd depends
    mkdir -p ../include/acl
    mkdir -p ../include/graph

    # 处理runtime仓
    if [ -d runtime-master-include-external-acl ]; then
        rm -rf runtime-master-include-external-acl
    fi
    if [ -f runtime-master-include-external-acl.zip ]; then
        unzip runtime-master-include-external-acl.zip
        cp -rf runtime-master-include-external-acl/include/external/acl/* ../include/acl
    else
        cp -rf ../headers_clone_dir/runtime_inc_clone/include/external/acl/* ../include/acl
    fi

    # 处理ge仓
    if [ -d ge-master-inc ]; then
        rm -rf ge-master-inc
    fi
    if [ -f ge-master-inc.zip ]; then
        unzip ge-master-inc.zip
        dir_cpy="ge-master-inc"
    else
        dir_cpy="../headers_clone_dir/ge_inc_clone"
    fi
    cp ${dir_cpy}/inc/external/acl/acl_base_mdl.h ../include/acl
    cp ${dir_cpy}/inc/external/acl/acl_mdl.h ../include/acl
    cp ${dir_cpy}/inc/external/acl/acl_op.h ../include/acl
    cp ${dir_cpy}/inc/external/ge/ge_ir_build.h ../include/graph
    cp ${dir_cpy}/inc/graph_metadef/external/graph/gnode.h ../include/graph
    cp ${dir_cpy}/inc/graph_metadef/external/graph/resource_context.h ../include/graph
    cp ${dir_cpy}/inc/graph_metadef/external/graph/inference_context.h ../include/graph
    cp ${dir_cpy}/inc/graph_metadef/external/graph/operator.h ../include/graph
    cp ${dir_cpy}/inc/graph_metadef/external/graph/graph.h ../include/graph

    # 处理metadef仓
    if [ -d metadef-master-inc ]; then
        rm -rf metadef-master-inc
    fi
    if [ -f metadef-master-inc.zip ]; then
        unzip metadef-master-inc.zip
        dir_cpy="metadef-master-inc"
    else
        dir_cpy="../headers_clone_dir/metadef_inc_clone"
    fi
    cp ${dir_cpy}/inc/external/graph/types.h ../include/graph
    cp ${dir_cpy}/inc/external/graph/c_types.h ../include/graph
    cp ${dir_cpy}/inc/external/graph/ge_error_codes.h ../include/graph
    cp ${dir_cpy}/inc/external/graph/tensor.h ../include/graph
    cp ${dir_cpy}/inc/external/graph/ascend_string.h ../include/graph

    # 处理json头文件
    if [ -d json-v3.12.0 -o -d nlohmann ]; then
        rm -rf json-v3.12.0 nlohmann
    fi
    if [ -f json-v3.12.0.zip ]; then
        unzip json-v3.12.0.zip
        dir_cpy="json-v3.12.0"
    else
        dir_cpy="../headers_clone_dir/json_headers_clone"
    fi
    cp -rf ${dir_cpy}/include/nlohmann ./

    cd ..
}

# 这里替换为command-line-tools在自己环境上的目录
CMD_LINE_TOOLS_DIR=/home/sizel/test/command-line-tools

OHOS_SDK=${CMD_LINE_TOOLS_DIR}/sdk/default/openharmony/
OHOS_INC_DIR=${OHOS_SDK}/native/sysroot/usr/include/
HMS_INC_DIR=${CMD_LINE_TOOLS_DIR}/sdk/default/hms/native/sysroot/usr/include/
CURRENT_DIR=`pwd` # 这里填上仓cann-recipes-harmony-infer源码目录下的harmony_acl目录
cd ${CURRENT_DIR}

if [ -d include/acl ]; then
    rm -rf include/acl
fi

if [ -d include/graph ]; then
    rm -rf include/graph
fi

if [ -d include/base ]; then
    rm -rf include/base
fi

mkdir -p include
mkdir -p depends
auto_download_header_with_git
deal_after_download

if [ -d build ]; then
    rm -rf build
fi

mkdir build
cd build

# build src for ohos
cmake -D OHOS_INC_DIR=${OHOS_INC_DIR} -D HMS_INC_DIR=${HMS_INC_DIR} -D OHOS_STL=c++_shared -D OHOS_ARCH=arm64-v8a -D OHOS_PLATFORM=OHOS -D CMAKE_TOOLCHAIN_FILE=${OHOS_SDK}/native/build/cmake/ohos.toolchain.cmake ../scripts
cmake --build . -j

# build ut for linux
mkdir ut
cd ut
cmake ../../scripts
cmake --build . -j
