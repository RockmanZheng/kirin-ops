/* Copyright (C) 2020-2026. Huawei Technologies Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Apache License Version 2.0.
 * You may not use this file except in compliance with the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Apache License for more details at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef HIAI_ACL_MANAGER_H
#define HIAI_ACL_MANAGER_H
#include <string>
#include <vector>
#include <stdlib.h>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>

#include "acl/acl_base.h"
#include "acl/acl_mdl.h"
#include "acl/acl_rt.h"
#include "acl/acl.h"

#include "utils/assert.h"
#include "utils/string_utils.h"

#include "graph/ge_ir_build.h"
#include "graph/ascend_string.h"

#include "ndk/dynamic_load_helper.h"
#include "ndk/hiai_ndk_options.h"
#include "ndk/hiai_ndk_nncore.h"
#include "ndk/hiai_ndk_executor.h"
#include "ndk/hiai_ndk_aipp.h"

#include "hiai_foundation/hiai_aipp_param.h"

using namespace ge;
using namespace hiai;
#define ACL_MAX_TENSOR_NAME_LEN 128
#define ACL_MAX_DIM_CNT 128
#define PATH_MAX 4096
constexpr int32_t UNDEFINED_CONFIG = -1;
struct aclmdlConfigHandle 
{
    int32_t priority = -1;
    size_t mdlLoadType = 0;
    const char *loadPath = nullptr;
    const void *mdlAddr = nullptr;
    size_t mdlSize = 0;
    int32_t prefMode = -1;
};

struct aclmdlDesc {
    std::vector<NN_TensorDesc*> inputDescs;
    std::vector<NN_TensorDesc*> outputDescs;
};

struct aclmdlDataset {
    std::vector<std::shared_ptr<aclDataBuffer>> buffers;
    std::vector<std::shared_ptr<HiAI_AippParam>> aippParas;
};

// 这边要删除，等图灵合入
typedef void* aclmdlModel;

class ACLManager {
public:
    ACLManager() = default;

    ACLManager(const ACLManager &) = delete;

    ~ACLManager();

    aclError BuildModel(OH_NNCompilation *compilation, 
        const std::map<ge::AscendString, ge::AscendString> &build_options);
    
    aclError SaveModel(const void *data, size_t size, size_t *realSize, const aclmdlModel *model);

    aclError SaveModel(const char_t *graph_file, const aclmdlModel *model);

    aclError LoadModel(OH_NNCompilation *compilation, uint32_t *modelId);

    aclError LoadModel(const aclmdlConfigHandle *handle, uint32_t *modelId);

    aclError RunModel(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output);

    aclError GetTensorDesc(aclmdlDesc *modelDesc, uint32_t modelId);

    aclError UnloadModel(uint32_t modelId);

    aclError SetInputDynamic(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlIODims *dims);

private:
    OH_NNExecutor* GetExecutorByModelId(uint32_t modelId);

private:
    std::unordered_map<uint32_t, std::vector<NN_Tensor*>> dynamicInput_;
    std::unordered_map<uint32_t, std::shared_ptr<OH_NNExecutor>> executorMap_;
    std::mutex mutex_;
};
#endif