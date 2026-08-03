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

#ifndef HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_EXECUTOR_H
#define HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_EXECUTOR_H

#include "neural_network_runtime/neural_network_core.h"
#include "acl/acl_base.h"

namespace hiai {
aclError HIAI_NDK_HiAIExecutor_GetModelID(OH_NNExecutor *executor, uint32_t* modelId);
}

#endif // HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_EXECUTOR_H