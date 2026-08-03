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

#include "hiai_foundation/hiai_options.h"
#include <string>
#include "hiai_ndk_struct_def.h"
#include "utils/assert.h"

namespace {
HiAIOptions* GetHiAIOptions(OH_NNCompilation* compilation)
{
    return reinterpret_cast<NNCompilation*>(compilation)->hiaiOptions;
}

const HiAIOptions* GetHiAIOptions(const OH_NNCompilation* compilation)
{
    return reinterpret_cast<const NNCompilation*>(compilation)->hiaiOptions;
}
}

OH_NN_ReturnCode HMS_HiAIOptions_SetInputTensorShapes(OH_NNCompilation* compilation, NN_TensorDesc* inputTensorDescs[] , size_t shapeCount)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && inputTensorDescs != nullptr && shapeCount > 0, OH_NN_INVALID_PARAMETER);
    for (size_t i = 0; i < shapeCount; i++) {
        ACL_EXPECT_TRUE_R(inputTensorDescs[i] != nullptr, OH_NN_INVALID_PARAMETER);
    }
    GetHiAIOptions(compilation)->inputTensorDescs = inputTensorDescs;
    GetHiAIOptions(compilation)->shapeCount = shapeCount;
    return OH_NN_SUCCESS;
}

size_t HMS_HiAIOptions_GetInputTensorShapeSize(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, 0);
    return GetHiAIOptions(compilation)->shapeCount;
}

NN_TensorDesc* HMS_HiAIOptions_GetInputTensorShape(const OH_NNCompilation* compilation, size_t index)
{
    ACL_EXPECT_NOT_NULL_R(compilation, nullptr);
    return GetHiAIOptions(compilation)->inputTensorDescs[index];
}

OH_NN_ReturnCode HMS_HiAIOptions_SetFormatMode(OH_NNCompilation* compilation, HiAI_FormatMode formatMode)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && formatMode >= HIAI_FORMAT_MODE_NCHW &&
        formatMode <= HIAI_FORMAT_MODE_ORIGIN, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->formatMode = formatMode;
    return OH_NN_SUCCESS;
}

HiAI_FormatMode HMS_HiAIOptions_GetFormatMode(OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_FORMAT_MODE_NCHW);
    return GetHiAIOptions(compilation)->formatMode;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetDynamicShapeStatus(OH_NNCompilation* compilation, HiAI_DynamicShapeStatus status)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && status >= HIAI_DYNAMIC_SHAPE_DISABLED &&
        status <= HIAI_DYNAMIC_SHAPE_ENABLED, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->status = status;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetDynamicShapeMaxCache(OH_NNCompilation* compilation, size_t maxCacheCount)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && maxCacheCount >= 1 &&
        maxCacheCount <= 10, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->maxCacheCount = maxCacheCount;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetDynamicShapeCacheMode(OH_NNCompilation* compilation, HiAI_DynamicShapeCacheMode mode)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && mode >= HIAI_DYNAMIC_SHAPE_CACHE_BUILT_MODEL &&
        mode <= HIAI_DYNAMIC_SHAPE_CACHE_LOADED_MODEL, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->mode = mode;
    return OH_NN_SUCCESS;
}

HiAI_DynamicShapeStatus HMS_HiAIOptions_GetDynamicShapeStatus(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_DYNAMIC_SHAPE_DISABLED);
    return GetHiAIOptions(compilation)->status;
}

size_t HMS_HiAIOptions_GetDynamicShapeMaxCache(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, 0);
    return GetHiAIOptions(compilation)->maxCacheCount;
}

HiAI_DynamicShapeCacheMode HMS_HiAIOptions_GetDynamicShapeCacheMode(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_DYNAMIC_SHAPE_CACHE_BUILT_MODEL);
    return GetHiAIOptions(compilation)->mode;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetOperatorDeviceOrder(OH_NNCompilation* compilation, const char* operatorName,
    HiAI_ExecuteDevice* executeDevices, size_t deviceCount)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && operatorName != nullptr && std::string(operatorName) != "" &&
        executeDevices != nullptr && deviceCount != 0, OH_NN_INVALID_PARAMETER);
    for (size_t i = 0; i < deviceCount; i++) {
        ACL_EXPECT_TRUE_R(executeDevices[i] > HIAI_EXECUTE_DEVICE_NPU, OH_NN_INVALID_PARAMETER);
    }
    OpDeviceOrder opDeviceOrder;
    opDeviceOrder.operatorName = operatorName;
    opDeviceOrder.executeDevices = executeDevices;
    opDeviceOrder.deviceCount = deviceCount;
    GetHiAIOptions(compilation)->opDeviceOrder.push_back(opDeviceOrder);
    return OH_NN_SUCCESS;
}

size_t HMS_HiAIOptions_GetOperatorDeviceCount(const OH_NNCompilation* compilation, const char* operatorName)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && operatorName != nullptr && std::string(operatorName) != "", 0);
    std::vector<OpDeviceOrder> opDeviceOrder = GetHiAIOptions(compilation)->opDeviceOrder;
    for (size_t i = 0; i < opDeviceOrder.size(); i++) {
        if (std::string(operatorName) == std::string(opDeviceOrder[i].operatorName)) {
            return opDeviceOrder[i].deviceCount;
        }
    }
    return 0;
}

HiAI_ExecuteDevice* HMS_HiAIOptions_GetOperatorDeviceOrder(const OH_NNCompilation* compilation, const char* operatorName)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && operatorName != nullptr && std::string(operatorName) != "", nullptr);
    std::vector<OpDeviceOrder> opDeviceOrder = GetHiAIOptions(compilation)->opDeviceOrder;
    for (size_t i = 0; i < opDeviceOrder.size(); i++) {
        if (std::string(operatorName) == std::string(opDeviceOrder[i].operatorName)) {
            return opDeviceOrder[i].executeDevices;
        }
    }
    return nullptr;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetModelDeviceOrder(OH_NNCompilation* compilation, HiAI_ExecuteDevice* executeDevices, size_t deviceCount)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && executeDevices != nullptr && deviceCount != 0, OH_NN_INVALID_PARAMETER);
    for (size_t i = 0; i < deviceCount; i++) {
        ACL_EXPECT_TRUE_R(executeDevices[i] > HIAI_EXECUTE_DEVICE_NPU, OH_NN_INVALID_PARAMETER);
    }
    GetHiAIOptions(compilation)->executeDevices = executeDevices;
    GetHiAIOptions(compilation)->deviceCount = deviceCount;
    return OH_NN_SUCCESS;
}

size_t HMS_HiAIOptions_GetModelDeviceCount(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, 0);
    return GetHiAIOptions(compilation)->deviceCount;
}

HiAI_ExecuteDevice* HMS_HiAIOptions_GetModelDeviceOrder(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, nullptr);
    return GetHiAIOptions(compilation)->executeDevices;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetFallbackMode(OH_NNCompilation* compilation, HiAI_FallbackMode fallbackMode)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && fallbackMode >= HIAI_FALLBACK_ENABLED && fallbackMode <= HIAI_FALLBACK_DISABLED,
        OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->fallbackMode = fallbackMode;
    return OH_NN_SUCCESS;
}

HiAI_FallbackMode HMS_HiAIOptions_GetFallbackMode(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_FALLBACK_ENABLED);
    return GetHiAIOptions(compilation)->fallbackMode;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetDeviceMemoryReusePlan(OH_NNCompilation* compilation, HiAI_DeviceMemoryReusePlan deviceMemoryReusePlan)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr &&
        deviceMemoryReusePlan >= HIAI_DEVICE_MEMORY_REUSE_PLAN_UNSET && deviceMemoryReusePlan <= HIAI_DEVICE_MEMORY_REUSE_PLAN_HIGH,
        OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->deviceMemoryReusePlan = deviceMemoryReusePlan;
    return OH_NN_SUCCESS;
}

HiAI_DeviceMemoryReusePlan HMS_HiAIOptions_GetDeviceMemoryReusePlan(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_DEVICE_MEMORY_REUSE_PLAN_UNSET);
    return GetHiAIOptions(compilation)->deviceMemoryReusePlan;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetTuningStrategy(OH_NNCompilation* compilation, HiAI_TuningStrategy tuningStrategy)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr &&
        tuningStrategy >= HIAI_TUNING_STRATEGY_OFF && tuningStrategy <= HIAI_TUNING_STRATEGY_ON_CLOUD_TUNING,
        OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->tuningStrategy = tuningStrategy;
    return OH_NN_SUCCESS;
}

HiAI_TuningStrategy HMS_HiAIOptions_GetTuningStrategy(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_TUNING_STRATEGY_OFF);
    return GetHiAIOptions(compilation)->tuningStrategy;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetQuantConfig(OH_NNCompilation* compilation, void* data, size_t size)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && data != nullptr && size != 0, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->quantizeConfigData = data;
    GetHiAIOptions(compilation)->quantizeConfigSize = size;
    return OH_NN_SUCCESS;
}

void* HMS_HiAIOptions_GetQuantConfigData(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, nullptr);
    return GetHiAIOptions(compilation)->quantizeConfigData;
}

size_t HMS_HiAIOptions_GetQuantConfigSize(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, 0);
    return GetHiAIOptions(compilation)->quantizeConfigSize;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetTuningMode(OH_NNCompilation* compilation, HiAI_TuningMode tuningMode)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr &&
        tuningMode >= HIAI_TUNING_MODE_UNSET && tuningMode <= HIAI_TUNING_MODE_HETER, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->tuningMode = tuningMode;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetTuningCacheDir(OH_NNCompilation* compilation, const char* cacheDir)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr &&
        cacheDir != nullptr && std::string(cacheDir) != "", OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->cacheDir = cacheDir;
    return OH_NN_SUCCESS;
}

HiAI_TuningMode HMS_HiAIOptions_GetTuningMode(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_TUNING_MODE_UNSET);
    return GetHiAIOptions(compilation)->tuningMode;
}

const char* HMS_HiAIOptions_GetTuningCacheDir(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, nullptr);
    return GetHiAIOptions(compilation)->cacheDir;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetBandMode(OH_NNCompilation* compilation, HiAI_BandMode bandMode)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr &&
        bandMode >= HIAI_BANDMODE_UNSET && bandMode <= HIAI_BANDMODE_HIGH, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->bandMode = bandMode;
    return OH_NN_SUCCESS;
}

HiAI_BandMode HMS_HiAIOptions_GetBandMode(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, HIAI_BANDMODE_UNSET);
    return GetHiAIOptions(compilation)->bandMode;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetAsyncModeEnable(OH_NNCompilation* compilation, bool isEnable)
{
    ACL_EXPECT_NOT_NULL_R(compilation, OH_NN_FAILED);
    GetHiAIOptions(compilation)->isEnableAsync = isEnable;
    return OH_NN_SUCCESS;
}

bool HMS_HiAIOptions_GetAsyncModeEnable(const OH_NNCompilation* compilation)
{
    ACL_EXPECT_NOT_NULL_R(compilation, false);
    return GetHiAIOptions(compilation)->isEnableAsync;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetCustomOP(OH_NNCompilation* compilation, const char* customPath)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && customPath != nullptr, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->customPath = customPath;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetAllocate(OH_NNCompilation* compilation, onAllocate allocateFunc)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && allocateFunc != nullptr, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->allocateFunc = allocateFunc;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetFree(OH_NNCompilation* compilation, onFree freeFunc)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && freeFunc != nullptr, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->freeFunc = freeFunc;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIOptions_SetUserData(OH_NNCompilation* compilation, void* userData)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && userData != nullptr, OH_NN_INVALID_PARAMETER);
    GetHiAIOptions(compilation)->userData = userData;
    return OH_NN_SUCCESS;
}

HiAI_NativeHandle* HMS_HiAINativeHandle_Create(int fd, int size, int offset)
{
    HiAI_NativeHandle* handle = (HiAI_NativeHandle*) malloc(sizeof(HiAI_NativeHandle));
    ACL_EXPECT_NOT_NULL_R(handle, nullptr);
    (void) memset(handle, 0x0, sizeof(HiAI_NativeHandle));
    handle->fd = fd;
    handle->size = size;
    handle->offset = offset;
    return handle;
}

void HMS_HiAINativeHandle_Destroy(HiAI_NativeHandle** handle)
{
    if (handle == nullptr || *handle == nullptr) {
        return;
    }
    free(*handle);
    *handle = nullptr;
}