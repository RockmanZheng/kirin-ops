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

#include "hiai_ndk_options.h"

#include "utils/assert.h"
#include "utils/log.h"
#include "ndk_proxy.h"

namespace hiai {
aclError HIAI_NDK_HiAIOptions_SetInputTensorShapes(OH_NNCompilation* compilation, 
    NN_TensorDesc* inputTensorDescs[], size_t shapeCount)
{
    auto setInputTensorShapesFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetInputTensorShapes)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetInputTensorShapes"));
    ACL_EXPECT_NOT_NULL(setInputTensorShapesFunc);
    OH_NN_ReturnCode retCode = setInputTensorShapesFunc(compilation, inputTensorDescs, shapeCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetFormatMode(OH_NNCompilation* compilation, HiAI_FormatMode formatMode)
{
    auto setFormatModeFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetFormatMode)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetFormatMode"));
    ACL_EXPECT_NOT_NULL(setFormatModeFunc);
    OH_NN_ReturnCode retCode = setFormatModeFunc(compilation, formatMode);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetDynamicShapeStatus(OH_NNCompilation* compilation, HiAI_DynamicShapeStatus status)
{
    auto setDynamicShapeStatusFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetDynamicShapeStatus)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetDynamicShapeStatus"));
    ACL_EXPECT_NOT_NULL(setDynamicShapeStatusFunc);
    OH_NN_ReturnCode retCode = setDynamicShapeStatusFunc(compilation, status);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetDynamicShapeMaxCache(OH_NNCompilation* compilation, size_t maxCacheCount)
{
    auto setDynamicShapeMaxCacheFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetDynamicShapeMaxCache)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetDynamicShapeMaxCache"));
    ACL_EXPECT_NOT_NULL(setDynamicShapeMaxCacheFunc);
    OH_NN_ReturnCode retCode = setDynamicShapeMaxCacheFunc(compilation, maxCacheCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetDynamicShapeCacheMode(OH_NNCompilation* compilation, HiAI_DynamicShapeCacheMode cacheMode)
{
    auto setDynamicShapeCacheModeFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetDynamicShapeCacheMode)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetDynamicShapeCacheMode"));
    ACL_EXPECT_NOT_NULL(setDynamicShapeCacheModeFunc);
    OH_NN_ReturnCode retCode = setDynamicShapeCacheModeFunc(compilation, cacheMode);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE; 
}

aclError HIAI_NDK_HiAIOptions_SetOperatorDeviceOrder(OH_NNCompilation* compilation,
    const char* operatorName, HiAI_ExecuteDevice* executeDevices, size_t deviceCount)
{
    auto setOperatorDeviceOrderFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetOperatorDeviceOrder)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetOperatorDeviceOrder"));
    ACL_EXPECT_NOT_NULL(setOperatorDeviceOrderFunc);
    OH_NN_ReturnCode retCode = setOperatorDeviceOrderFunc(compilation, operatorName, executeDevices, deviceCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE; 
}

aclError HIAI_NDK_HiAIOptions_SetModelDeviceOrder(OH_NNCompilation* compilation,
    HiAI_ExecuteDevice* executeDevices, size_t deviceCount)
{
    auto setModelDeviceOrderFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetModelDeviceOrder)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetModelDeviceOrder"));
    ACL_EXPECT_NOT_NULL(setModelDeviceOrderFunc);
    OH_NN_ReturnCode retCode = setModelDeviceOrderFunc(compilation, executeDevices, deviceCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE; 
}

aclError HIAI_NDK_HiAIOptions_SetFallbackMode(OH_NNCompilation* compilation, HiAI_FallbackMode fallbackMode)
{
    auto setFallbackModeFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetFallbackMode)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetFallbackMode"));
    ACL_EXPECT_NOT_NULL(setFallbackModeFunc);
    OH_NN_ReturnCode retCode = setFallbackModeFunc(compilation, fallbackMode);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE; 
}

aclError HIAI_NDK_HiAIOptions_SetDeviceMemoryReusePlan(OH_NNCompilation* compilation,
    HiAI_DeviceMemoryReusePlan deviceMemoryReusePlan)
{
    auto setDeviceMemoryReusePlanFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetDeviceMemoryReusePlan)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetDeviceMemoryReusePlan"));
    ACL_EXPECT_NOT_NULL(setDeviceMemoryReusePlanFunc);
    OH_NN_ReturnCode retCode = setDeviceMemoryReusePlanFunc(compilation, deviceMemoryReusePlan);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetTuningStrategy(OH_NNCompilation* compilation, HiAI_TuningStrategy tuningStrategy)
{
    auto setTuningStrategyFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetTuningStrategy)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetTuningStrategy"));
    ACL_EXPECT_NOT_NULL(setTuningStrategyFunc);
    OH_NN_ReturnCode retCode = setTuningStrategyFunc(compilation, tuningStrategy);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetQuantConfig(OH_NNCompilation* compilation, void* data, size_t size)
{
    auto setQuantConfigFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetQuantConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetQuantConfig"));
    ACL_EXPECT_NOT_NULL(setQuantConfigFunc);
    OH_NN_ReturnCode retCode = setQuantConfigFunc(compilation, data, size);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetTuningMode(OH_NNCompilation* compilation, HiAI_TuningMode tuningMode)
{
    auto setTuningModeFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetTuningMode)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetTuningMode"));
    ACL_EXPECT_NOT_NULL(setTuningModeFunc);
    OH_NN_ReturnCode retCode = setTuningModeFunc(compilation, tuningMode);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetTuningCacheDir(OH_NNCompilation* compilation, const char* cacheDir)
{
    auto setTuningCacheDirFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetTuningCacheDir)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetTuningCacheDir"));
    ACL_EXPECT_NOT_NULL(setTuningCacheDirFunc);
    OH_NN_ReturnCode retCode = setTuningCacheDirFunc(compilation, cacheDir);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetBandMode(OH_NNCompilation* nnCompilation, HiAI_BandMode bandMode)
{
    auto setBandModeFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetBandMode)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetBandMode"));
    ACL_EXPECT_NOT_NULL(setBandModeFunc);
    OH_NN_ReturnCode retCode = setBandModeFunc(nnCompilation, bandMode);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

HiAI_BandMode HIAI_NDK_HiAIOptions_GetBandMode(const OH_NNCompilation* nnCompilation)
{
    auto getBandModeFunc = reinterpret_cast<decltype(HMS_HiAIOptions_GetBandMode)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_GetBandMode"));
    return getBandModeFunc == nullptr ? HIAI_BANDMODE_UNSET : getBandModeFunc(nnCompilation);
}

aclError HIAI_NDK_HiAIOptions_SetAsyncModeEnable(OH_NNCompilation* nnCompilation, bool isEnable)
{
    auto setAsyncModeEnableFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetAsyncModeEnable)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetAsyncModeEnable"));
    ACL_EXPECT_NOT_NULL(setAsyncModeEnableFunc);
    OH_NN_ReturnCode retCode = setAsyncModeEnableFunc(nnCompilation, isEnable);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

bool HIAI_NDK_HiAIOptions_GetAsyncModeEnable(const OH_NNCompilation* nnCompilation)
{
    auto getAsyncModeEnableFunc = reinterpret_cast<decltype(HMS_HiAIOptions_GetAsyncModeEnable)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_GetAsyncModeEnable"));
    return getAsyncModeEnableFunc == nullptr ? false : getAsyncModeEnableFunc(nnCompilation);
}

aclError HIAI_NDK_HiAIOptions_SetCustomOpPath(OH_NNCompilation* compilation, const char* customPath)
{
    auto setCustomOpPathFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetCustomOP)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetCustomOP"));
    ACL_EXPECT_NOT_NULL(setCustomOpPathFunc);
    OH_NN_ReturnCode retCode = setCustomOpPathFunc(compilation, customPath);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIOptions_SetAllocate(OH_NNCompilation* compilation, onAllocate allocateFunc)
{
    auto setAllocateFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetAllocate)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetAllocate"));
    ACL_EXPECT_NOT_NULL(setAllocateFunc);
    ACL_EXPECT_TRUE(setAllocateFunc(compilation, allocateFunc) == OH_NN_SUCCESS);
    return ACL_SUCCESS;
}

aclError HIAI_NDK_HiAIOptions_SetFree(OH_NNCompilation* compilation, onFree freeFunc)
{
    auto setFreeFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetFree)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetFree"));
    ACL_EXPECT_NOT_NULL(setFreeFunc);
    ACL_EXPECT_TRUE(setFreeFunc(compilation, freeFunc) == OH_NN_SUCCESS);
    return ACL_SUCCESS;
}

aclError HIAI_NDK_HiAIOptions_SetUserData(OH_NNCompilation* compilation, void* userData)
{
    auto setUserDataFunc = reinterpret_cast<decltype(HMS_HiAIOptions_SetUserData)*>(
        NDKProxy::GetSymbol("HMS_HiAIOptions_SetUserData"));
    ACL_EXPECT_NOT_NULL(setUserDataFunc);
    ACL_EXPECT_TRUE(setUserDataFunc(compilation, userData) == OH_NN_SUCCESS);
    return ACL_SUCCESS;
}

} // namespace hiai