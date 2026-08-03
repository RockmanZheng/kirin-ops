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

#ifndef HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_TENSOR_H
#define HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_TENSOR_H

#include <vector>

#include "hiai_foundation/hiai_tensor.h"
#include "acl/acl_base.h"

#ifdef __cplusplus
extern "C" {
#endif

OH_NN_ReturnCode HMS_HiAITensor_GetCacheStatus(NN_Tensor* tensor, uint8_t* cacheStatus);

OH_NN_ReturnCode HMS_HiAITensor_SetCacheStatus(NN_Tensor* tensor, uint8_t cacheStatus);

#ifdef __cplusplus
}
#endif

namespace hiai {

aclError HIAI_NDK_HiAITensor_SetAippParams(std::vector<NN_Tensor*> tensor, std::vector<HiAI_AippParam*> aippParams);

aclError HIAI_NDK_HiAITensor_SetCacheStatus(NN_Tensor* tensor, uint8_t cacheStatus);

aclError HIAI_NDK_HiAITensor_GetCacheStatus(NN_Tensor* tensor, uint8_t* cacheStatus);

}

#endif