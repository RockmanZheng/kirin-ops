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

#ifndef HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_OPTIONS_H
#define HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_OPTIONS_H

#include "neural_network_runtime/neural_network_core.h"

#include "acl/acl_base.h"
#include "hiai_foundation/hiai_options.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HiAI_NativeHandle HiAI_NativeHandle;

typedef void (*onAllocate) (void* userData, uint32_t requiredSize, HiAI_NativeHandle* handles[], size_t* handlesSize);

typedef void (*onFree) (void* userData, HiAI_NativeHandle* handles[], size_t handlesSize);

OH_NN_ReturnCode HMS_HiAIOptions_SetAsyncModeEnable(OH_NNCompilation* compilation, bool isEnable);

bool HMS_HiAIOptions_GetAsyncModeEnable(const OH_NNCompilation* compilation);

typedef struct HiAI_NativeHandle HiAI_NativeHandle;

typedef void (*onAllocate) (void* userData, uint32_t requiredSize, HiAI_NativeHandle* handles[], size_t* handlesSize);

typedef void (*onFree) (void* userData, HiAI_NativeHandle* handles[], size_t handlesSize);

HiAI_NativeHandle* HMS_HiAINativeHandle_Create(int fd, int size, int offset);

void HMS_HiAINativeHandle_Destroy(HiAI_NativeHandle** handle);

OH_NN_ReturnCode HMS_HiAIOptions_SetAllocate(OH_NNCompilation* compilation, onAllocate allocateFunc);

OH_NN_ReturnCode HMS_HiAIOptions_SetFree(OH_NNCompilation* compilation, onFree freeFunc);

OH_NN_ReturnCode HMS_HiAIOptions_SetUserData(OH_NNCompilation* compilation, void* userData);

OH_NN_ReturnCode HMS_HiAIOptions_SetCustomOP(OH_NNCompilation* compilation, const char* customPath);

#ifdef __cplusplus
}
#endif

namespace hiai {

aclError HIAI_NDK_HiAIOptions_SetInputTensorShapes(OH_NNCompilation* compilation,
    NN_TensorDesc* inputTensorDescs[], size_t shapeCount);

aclError HIAI_NDK_HiAIOptions_SetFormatMode(OH_NNCompilation* compilation, HiAI_FormatMode formatMode);

aclError HIAI_NDK_HiAIOptions_SetDynamicShapeStatus(OH_NNCompilation* compilation, HiAI_DynamicShapeStatus status);

aclError HIAI_NDK_HiAIOptions_SetDynamicShapeMaxCache(OH_NNCompilation* compilation, size_t maxCacheCount);

aclError HIAI_NDK_HiAIOptions_SetDynamicShapeCacheMode(OH_NNCompilation* compilation, HiAI_DynamicShapeCacheMode mode);

aclError HIAI_NDK_HiAIOptions_SetOperatorDeviceOrder(OH_NNCompilation* compilation,
    const char* operatorName, HiAI_ExecuteDevice* executeDevices, size_t deviceCount);

aclError HIAI_NDK_HiAIOptions_SetModelDeviceOrder(OH_NNCompilation* compilation, 
    HiAI_ExecuteDevice* executeDevices, size_t deviceCount);    

aclError HIAI_NDK_HiAIOptions_SetFallbackMode(OH_NNCompilation* compilation, HiAI_FallbackMode fallbackMode);

aclError HIAI_NDK_HiAIOptions_SetDeviceMemoryReusePlan(OH_NNCompilation* compilation, 
    HiAI_DeviceMemoryReusePlan deviceMemoryReusePlan);

aclError HIAI_NDK_HiAIOptions_SetTuningStrategy(OH_NNCompilation* compilation, HiAI_TuningStrategy tuningStrategy);

aclError HIAI_NDK_HiAIOptions_SetQuantConfig(OH_NNCompilation* compilation, void* data, size_t size);

aclError HIAI_NDK_HiAIOptions_SetTuningMode(OH_NNCompilation* compilation, HiAI_TuningMode tuningMode);

aclError HIAI_NDK_HiAIOptions_SetTuningCacheDir(OH_NNCompilation* compilation, const char* cacheDir);

aclError HIAI_NDK_HiAIOptions_SetBandMode(OH_NNCompilation* nnCompilation, HiAI_BandMode bandMode);

HiAI_BandMode HIAI_NDK_HiAIOptions_GetBandMode(const OH_NNCompilation* nnCompilation);

aclError HIAI_NDK_HiAIOptions_SetAsyncModeEnable(OH_NNCompilation* nnCompilation, bool isEnable);

bool HIAI_NDK_HiAIOptions_GetAsyncModeEnable(const OH_NNCompilation* nnCompilation);

aclError HIAI_NDK_HiAIOptions_SetCustomOpPath(OH_NNCompilation* compilation, const char* customPath);

aclError HIAI_NDK_HiAIOptions_SetAllocate(OH_NNCompilation* compilation, onAllocate allocateFunc);

aclError HIAI_NDK_HiAIOptions_SetFree(OH_NNCompilation* compilation, onFree freeFunc);

aclError HIAI_NDK_HiAIOptions_SetUserData(OH_NNCompilation* compilation, void* userData);

}
#endif