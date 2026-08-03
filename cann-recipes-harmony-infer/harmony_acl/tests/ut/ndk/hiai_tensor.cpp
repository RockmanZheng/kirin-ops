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

#include "hiai_foundation/hiai_tensor.h"
#include "utils/assert.h"
#include "hiai_ndk_struct_def.h"

static size_t TENSOR_SIZE = 100;
size_t HMS_HiAITensor_GetSizeWithImageFormat(NN_TensorDesc* desc, HiAI_ImageFormat format)
{
    (void) format;
    ACL_EXPECT_TRUE_R(desc != nullptr, 0);
    return TENSOR_SIZE;
}

OH_NN_ReturnCode HMS_HiAITensor_SetAippParams(NN_Tensor* tensor, HiAI_AippParam* aippParams[], size_t aippNum)
{
    ACL_EXPECT_TRUE_R(tensor != nullptr && aippParams != nullptr && aippNum != 0, OH_NN_FAILED);
    NNTensor* nnTensor = reinterpret_cast<NNTensor*>(tensor);
    nnTensor->aippParams.clear();
    for (size_t i = 0; i < aippNum; ++i) {
        nnTensor->aippParams.push_back(aippParams[i]);
    }
    return OH_NN_SUCCESS;
}