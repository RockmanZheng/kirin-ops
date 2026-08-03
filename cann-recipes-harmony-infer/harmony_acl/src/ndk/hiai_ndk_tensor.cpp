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

#include "utils/assert.h"

#include "hiai_ndk_tensor.h"

#include "ndk_proxy.h"

namespace hiai {
aclError HIAI_NDK_HiAITensor_SetAippParams(std::vector<NN_Tensor*> tensor, std::vector<HiAI_AippParam*> aippParams)
{
    ACL_EXPECT_TRUE(!tensor.empty());

    auto setAippParamsFunc = reinterpret_cast<decltype(HMS_HiAITensor_SetAippParams)*>(
        NDKProxy::GetSymbol("HMS_HiAITensor_SetAippParams"));
    ACL_EXPECT_NOT_NULL(setAippParamsFunc);
    OH_NN_ReturnCode retCode = setAippParamsFunc(tensor[0], aippParams.data(), aippParams.size());
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
} // namespace hiai

aclError HIAI_NDK_HiAITensor_SetCacheStatus(NN_Tensor* tensor, uint8_t cacheStatus)
{
    ACL_EXPECT_NOT_NULL(tensor);

    auto setCacheStatusFunc = reinterpret_cast<decltype(HMS_HiAITensor_SetCacheStatus)*>(
        NDKProxy::GetSymbol("HMS_HiAITensor_SetCacheStatus"));
    ACL_EXPECT_NOT_NULL(setCacheStatusFunc);
    ACL_EXPECT_TRUE(setCacheStatusFunc(tensor, cacheStatus) == OH_NN_SUCCESS);
    return ACL_SUCCESS;
}

aclError HIAI_NDK_HiAITensor_GetCacheStatus(NN_Tensor* tensor, uint8_t* cacheStatus)
{
    ACL_EXPECT_NOT_NULL(tensor);
    ACL_EXPECT_NOT_NULL(cacheStatus);

    auto getCacheStatusFunc = reinterpret_cast<decltype(HMS_HiAITensor_GetCacheStatus)*>(
        NDKProxy::GetSymbol("HMS_HiAITensor_GetCacheStatus"));
    ACL_EXPECT_NOT_NULL(getCacheStatusFunc);
    ACL_EXPECT_TRUE(getCacheStatusFunc(tensor, cacheStatus) == OH_NN_SUCCESS);
    return ACL_SUCCESS;
}
}