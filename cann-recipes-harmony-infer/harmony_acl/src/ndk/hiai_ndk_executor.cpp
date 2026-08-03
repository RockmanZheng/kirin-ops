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
 
#include "hiai_ndk_executor.h"

#include "utils/log.h"
#include "utils/assert.h"
#include "ndk_proxy.h"

extern "C" {
OH_NN_ReturnCode HMS_HiAIExecutor_GetModelID(OH_NNExecutor *executor, uint32_t* modelId);
}

namespace hiai {
aclError HIAI_NDK_HiAIExecutor_GetModelID(OH_NNExecutor *executor, uint32_t* modelId) {
    ACL_EXPECT_NOT_NULL(executor);
    ACL_EXPECT_NOT_NULL(modelId);
    auto getModelIDFunc = reinterpret_cast<decltype(HMS_HiAIExecutor_GetModelID)*>(
        NDKProxy::GetSymbol("HMS_HiAIExecutor_GetModelID"));
    ACL_EXPECT_NOT_NULL(getModelIDFunc);
    OH_NN_ReturnCode retCode = getModelIDFunc(executor, modelId);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}
} // namespace hiai