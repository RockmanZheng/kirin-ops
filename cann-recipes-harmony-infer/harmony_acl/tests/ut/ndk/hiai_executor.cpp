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
#include "neural_network_runtime/neural_network_runtime_type.h"

extern "C" {
OH_NN_ReturnCode HMS_HiAIExecutor_GetModelID(OH_NNExecutor* executor, uint32_t* modelId)
{
    ACL_EXPECT_TRUE_R(executor != nullptr, OH_NN_INVALID_PARAMETER);
    *modelId = 1;
    return OH_NN_SUCCESS;
}
}