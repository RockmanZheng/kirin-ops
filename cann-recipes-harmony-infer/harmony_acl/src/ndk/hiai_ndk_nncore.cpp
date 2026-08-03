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

#include "hiai_ndk_nncore.h"

#include "utils/assert.h"
#include "utils/log.h"
#include "acl/acl_base.h"
#include "ndk_proxy.h"

namespace hiai {
static size_t g_deviceID = 0;
aclError HIAI_NDK_NNCompilation_SetPriority(OH_NNCompilation* nnCompilation, OH_NN_Priority priority)
{
    auto setPriorityFunc = reinterpret_cast<decltype(OH_NNCompilation_SetPriority)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_SetPriority"));
    ACL_EXPECT_NOT_NULL(setPriorityFunc);
    OH_NN_ReturnCode retCode = setPriorityFunc(nnCompilation, priority);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNCompilation_SetPerformanceMode(OH_NNCompilation* nnCompilation, OH_NN_PerformanceMode PerformanceMode)
{
    auto setPerformanceModeFunc = reinterpret_cast<decltype(OH_NNCompilation_SetPerformanceMode)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_SetPerformanceMode"));
    ACL_EXPECT_NOT_NULL(setPerformanceModeFunc);
    OH_NN_ReturnCode retCode = setPerformanceModeFunc(nnCompilation, PerformanceMode);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

OH_NNCompilation* HIAI_NDK_NNCompilation_ConstructForCache()
{
    auto constructForCacheFunc = reinterpret_cast<decltype(OH_NNCompilation_ConstructForCache)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_ConstructForCache"));
    return constructForCacheFunc == nullptr ? nullptr : constructForCacheFunc();
}

aclError HIAI_NDK_NNCompilation_SetCache(OH_NNCompilation* nnCompilation, const char* cachePath, uint32_t version)
{
    auto setCacheFunc = reinterpret_cast<decltype(OH_NNCompilation_SetCache)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_SetCache"));
    ACL_EXPECT_NOT_NULL(setCacheFunc);
    OH_NN_ReturnCode retCode = setCacheFunc(nnCompilation, cachePath, version);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

OH_NNExecutor* HIAI_NDK_NNExecutor_Construct(OH_NNCompilation* nnCompilation)
{
    auto constructFunc = reinterpret_cast<decltype(OH_NNExecutor_Construct)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_Construct"));
    return constructFunc == nullptr ? nullptr : constructFunc(nnCompilation);
}

aclError HIAI_NDK_NNExecutor_RunSync(OH_NNExecutor* executor, std::vector<NN_Tensor*> inputs,
    std::vector<NN_Tensor*> outputs)
{
    auto RunSyncFunc = reinterpret_cast<decltype(OH_NNExecutor_RunSync)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_RunSync"));
    ACL_EXPECT_NOT_NULL(RunSyncFunc);
    OH_NN_ReturnCode retCode = RunSyncFunc(executor, inputs.data(), inputs.size(), outputs.data(), outputs.size());
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNExecutor_RunAsync(OH_NNExecutor* executor, std::vector<NN_Tensor*> inputs,
    std::vector<NN_Tensor*> outputs, int32_t timeout, void* userData)
{
    auto RunAsyncFunc = reinterpret_cast<decltype(OH_NNExecutor_RunAsync)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_RunAsync"));
    ACL_EXPECT_NOT_NULL(RunAsyncFunc);
    OH_NN_ReturnCode retCode = RunAsyncFunc(executor, inputs.data(), inputs.size(), outputs.data(), outputs.size(),
        timeout, userData);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNCompilation_ExportCacheToBuffer(OH_NNCompilation* nnCompilation, const void* buffer, 
    size_t length, size_t* modelSize)
{
    auto ExportCacheToBufferFunc = reinterpret_cast<decltype(OH_NNCompilation_ExportCacheToBuffer)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_ExportCacheToBuffer"));
    ACL_EXPECT_NOT_NULL(ExportCacheToBufferFunc);
    OH_NN_ReturnCode retCode = ExportCacheToBufferFunc(nnCompilation, buffer, length, modelSize);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

void HIAI_NDK_NNCompilation_Destroy(OH_NNCompilation** compilation)
{
    auto DestroyFunc = reinterpret_cast<decltype(OH_NNCompilation_Destroy)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_Destroy"));
    if (DestroyFunc != nullptr) {
        DestroyFunc(compilation);
        compilation = nullptr;
    }
}

size_t HIAI_NDK_GetDeviceID()
{
    if (g_deviceID != 0)
    {
        return g_deviceID;
    }
    size_t deviceID = 0;
    auto getAllDeviceIdFunc = reinterpret_cast<decltype(OH_NNDevice_GetAllDevicesID)*>(
        NDKProxy::GetSymbol("OH_NNDevice_GetAllDevicesID"));
    auto getNameFunc = reinterpret_cast<decltype(OH_NNDevice_GetName)*>(NDKProxy::GetSymbol("OH_NNDevice_GetName"));
    if (getAllDeviceIdFunc == nullptr || getNameFunc == nullptr)
    {
        ACL_LOGE("getAllDeviceIdFunc or getNameFunc is nullptr");
        return deviceID;
    }
    
    const size_t* allDevicesID = nullptr;
    uint32_t deviceCount = 0;
    OH_NN_ReturnCode ret = getAllDeviceIdFunc(&allDevicesID, &deviceCount);
    if (ret != OH_NN_SUCCESS)
    {
        ACL_LOGE("OH_NNDevice_GetAllDevicesID failed");
        return deviceID;
    }
    if (deviceCount == 0)
    {
        return deviceID;
    }
    
    for (uint32_t i = 0; i < deviceCount; i++)
    {
        const char* name = nullptr;
        ret = getNameFunc(allDevicesID[i], &name);
        if (ret != OH_NN_SUCCESS)
        {
            ACL_LOGE("OH_NNDevice_GetName failed");
            return deviceID;
        }
        if (name != nullptr && std::string(name) == "HIAI_F")
        {
            deviceID = allDevicesID[i];
            break;
        }
    }
    g_deviceID = deviceID;
    return deviceID;
}

aclError HIAI_NDK_NNCompilation_SetDevice(OH_NNCompilation* nnCompilation)
{
    size_t deviceID = HIAI_NDK_GetDeviceID();
    ACL_EXPECT_TRUE(deviceID != 0);

    auto setDeviceFunc = reinterpret_cast<decltype(OH_NNCompilation_SetDevice)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_SetDevice"));
    ACL_EXPECT_NOT_NULL(setDeviceFunc);
    OH_NN_ReturnCode retCode = setDeviceFunc(nnCompilation, deviceID);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

OH_NNCompilation* HIAI_NDK_NNCompilation_ConstructWithOfflineModelBuffer(const void* modelBuffer, size_t modelSize)
{
    auto constructCompilationFunc = reinterpret_cast<decltype(OH_NNCompilation_ConstructWithOfflineModelBuffer)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_ConstructWithOfflineModelBuffer"));
    return constructCompilationFunc == nullptr ? nullptr : constructCompilationFunc(modelBuffer, modelSize);
}

aclError HIAI_NDK_NNCompilation_AddExtensionConfig(OH_NNCompilation *compilation,
    const char *configName, const void *configValue, const size_t configValueSize)
{
    auto addExtensionConfigFunc = reinterpret_cast<decltype(OH_NNCompilation_AddExtensionConfig)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_AddExtensionConfig"));
    ACL_EXPECT_NOT_NULL(addExtensionConfigFunc);
    OH_NN_ReturnCode retCode = addExtensionConfigFunc(compilation, configName, configValue, configValueSize);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNCompilation_Build(OH_NNCompilation* compilation)
{
    ACL_EXPECT_EXEC(HIAI_NDK_NNCompilation_SetDevice(compilation));

    auto buildFunc = reinterpret_cast<decltype(OH_NNCompilation_Build)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_Build"));
    ACL_EXPECT_NOT_NULL(buildFunc);
    OH_NN_ReturnCode retCode = buildFunc(compilation);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

OH_NNCompilation* HIAI_NDK_NNCompilation_ConstructWithOfflineModelFile(const char* modelPath)
{
    auto constructCompilationFunc = reinterpret_cast<decltype(OH_NNCompilation_ConstructWithOfflineModelFile)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_ConstructWithOfflineModelFile"));
    return constructCompilationFunc == nullptr ? nullptr : constructCompilationFunc(modelPath);
}

aclError HIAI_NDK_NNTensorDesc_GetShape(const NN_TensorDesc* tensorDesc, int32_t** shape, size_t* shapeLength)
{
    auto GetShapeFunc = reinterpret_cast<decltype(OH_NNTensorDesc_GetShape)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_GetShape"));
    ACL_EXPECT_NOT_NULL(GetShapeFunc);
    OH_NN_ReturnCode retCode = GetShapeFunc(tensorDesc, shape, shapeLength);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNTensorDesc_GetDataType(const NN_TensorDesc* tensorDesc, OH_NN_DataType* dataType)
{
    auto GetDataTypeFunc = reinterpret_cast<decltype(OH_NNTensorDesc_GetDataType)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_GetDataType"));
    ACL_EXPECT_NOT_NULL(GetDataTypeFunc);
    OH_NN_ReturnCode retCode = GetDataTypeFunc(tensorDesc, dataType);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNTensorDesc_GetFormat(const NN_TensorDesc* tensorDesc, OH_NN_Format* format)
{
    auto GetFormatFunc = reinterpret_cast<decltype(OH_NNTensorDesc_GetFormat)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_GetFormat"));
    ACL_EXPECT_NOT_NULL(GetFormatFunc);
    OH_NN_ReturnCode retCode = GetFormatFunc(tensorDesc, format);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNTensorDesc_GetName(const NN_TensorDesc* tensorDesc, const char** Name)
{
    auto GetNameFunc = reinterpret_cast<decltype(OH_NNTensorDesc_GetName)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_GetName"));
    ACL_EXPECT_NOT_NULL(GetNameFunc);
    OH_NN_ReturnCode retCode = GetNameFunc(tensorDesc, Name);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNExecutor_GetInputCount(const OH_NNExecutor* executor, size_t* inputCount)
{
    auto GetInputCountFunc = reinterpret_cast<decltype(OH_NNExecutor_GetInputCount)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_GetInputCount"));
    ACL_EXPECT_NOT_NULL(GetInputCountFunc);
    OH_NN_ReturnCode retCode = GetInputCountFunc(executor, inputCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNExecutor_GetOutputCount(const OH_NNExecutor* executor, size_t* outputCount)
{
    auto GetOutputCountFunc = reinterpret_cast<decltype(OH_NNExecutor_GetOutputCount)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_GetOutputCount"));
    ACL_EXPECT_NOT_NULL(GetOutputCountFunc);
    OH_NN_ReturnCode retCode = GetOutputCountFunc(executor, outputCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

void HIAI_NDK_NNExecutor_Destroy(OH_NNExecutor** executor)
{
    if (executor != nullptr)
    {
        auto executorDestroyFunc = reinterpret_cast<decltype(OH_NNExecutor_Destroy)*>(
            NDKProxy::GetSymbol("OH_NNExecutor_Destroy"));
        if (executorDestroyFunc != nullptr)
        {
            executorDestroyFunc(executor);
            executor = nullptr;
        }
    }
}

NN_TensorDesc* HIAI_NDK_NNExecutor_CreateInputTensorDesc(const OH_NNExecutor* executor, size_t index)
{
    auto CreateInputTensorDescFunc = reinterpret_cast<decltype(OH_NNExecutor_CreateInputTensorDesc)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_CreateInputTensorDesc"));
    return CreateInputTensorDescFunc != nullptr ? CreateInputTensorDescFunc(executor, index) : nullptr;
}

NN_TensorDesc* HIAI_NDK_NNExecutor_CreateOutputTensorDesc(const OH_NNExecutor* executor, size_t index)
{
    auto CreateOutputTensorDescFunc = reinterpret_cast<decltype(OH_NNExecutor_CreateOutputTensorDesc)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_CreateOutputTensorDesc"));
    return CreateOutputTensorDescFunc != nullptr ? CreateOutputTensorDescFunc(executor, index) : nullptr;
}

aclError HIAI_NDK_NNExecutor_SetOnRunDone(OH_NNExecutor* executor, NN_OnRunDone onRunDone)
{
    auto SetOnRunDoneFunc = reinterpret_cast<decltype(OH_NNExecutor_SetOnRunDone)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_SetOnRunDone"));
    ACL_EXPECT_NOT_NULL(SetOnRunDoneFunc);
    OH_NN_ReturnCode retCode = SetOnRunDoneFunc(executor, onRunDone);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNExecutor_SetOnServiceDied(OH_NNExecutor* executor, NN_OnServiceDied onServiceDied)
{
    auto SetOnServiceDiedFunc = reinterpret_cast<decltype(OH_NNExecutor_SetOnServiceDied)*>(
        NDKProxy::GetSymbol("OH_NNExecutor_SetOnServiceDied"));
    ACL_EXPECT_NOT_NULL(SetOnServiceDiedFunc);
    OH_NN_ReturnCode retCode = SetOnServiceDiedFunc(executor, onServiceDied);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNDevice_GetAllDevicesID(const size_t** allDevicesID, uint32_t* deviceCount)
{
    auto GetAllDevicesIDFunc = reinterpret_cast<decltype(OH_NNDevice_GetAllDevicesID)*>(
        NDKProxy::GetSymbol("OH_NNDevice_GetAllDevicesID"));
    ACL_EXPECT_NOT_NULL(GetAllDevicesIDFunc);
    OH_NN_ReturnCode retCode = GetAllDevicesIDFunc(allDevicesID, deviceCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNDevice_GetName(size_t deviceID, const char** name)
{
    auto GetNameFunc = reinterpret_cast<decltype(OH_NNDevice_GetName)*>(
        NDKProxy::GetSymbol("OH_NNDevice_GetName"));
    ACL_EXPECT_NOT_NULL(GetNameFunc);
    OH_NN_ReturnCode retCode = GetNameFunc(deviceID, name);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

void HIAI_NDK_NNTensorDesc_Destroy(NN_TensorDesc** TensorDesc)
{

    auto TensorDescDestroyFunc = reinterpret_cast<decltype(OH_NNTensorDesc_Destroy)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_Destroy"));
    if (TensorDescDestroyFunc != nullptr)
    {
        TensorDescDestroyFunc(TensorDesc);
        TensorDesc = nullptr;
    }
}

NN_TensorDesc* HIAI_NDK_NNTensorDesc_Create()
{
    auto TensorDescCreateFunc = reinterpret_cast<decltype(OH_NNTensorDesc_Create)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_Create"));
    ACL_EXPECT_NOT_NULL_R(TensorDescCreateFunc, nullptr);
    NN_TensorDesc* tensorDesc = TensorDescCreateFunc();
    return tensorDesc;
}

aclError HIAI_NDK_NNTensorDesc_SetShape(NN_TensorDesc* TensorDesc, const int32_t* shape, size_t shapeLength)
{
    auto SetShapeFunc = reinterpret_cast<decltype(OH_NNTensorDesc_SetShape)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_SetShape"));
    ACL_EXPECT_NOT_NULL(SetShapeFunc);
    OH_NN_ReturnCode retCode = SetShapeFunc(TensorDesc, shape, shapeLength);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNTensorDesc_SetDataType(NN_TensorDesc* TensorDesc, OH_NN_DataType dataType)
{
    auto SetDataTypeFunc = reinterpret_cast<decltype(OH_NNTensorDesc_SetDataType)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_SetDataType"));
    ACL_EXPECT_NOT_NULL(SetDataTypeFunc);
    OH_NN_ReturnCode retCode = SetDataTypeFunc(TensorDesc, dataType);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNTensorDesc_SetFormat(NN_TensorDesc* TensorDesc, OH_NN_Format Format)
{
    auto SetFormatFunc = reinterpret_cast<decltype(OH_NNTensorDesc_SetFormat)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_SetFormat"));
    ACL_EXPECT_NOT_NULL(SetFormatFunc);
    OH_NN_ReturnCode retCode = SetFormatFunc(TensorDesc, Format);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNCompilation_EnableFloat16(OH_NNCompilation* compilation, bool enableFloat16)
{
    auto EnableFloat16Func = reinterpret_cast<decltype(OH_NNCompilation_EnableFloat16)*>(
        NDKProxy::GetSymbol("OH_NNCompilation_EnableFloat16"));
    ACL_EXPECT_NOT_NULL(EnableFloat16Func);
    OH_NN_ReturnCode retCode = EnableFloat16Func(compilation, enableFloat16);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

NN_Tensor* HIAI_NDK_NNTensor_Create(NN_TensorDesc* tensorDesc)
{
    size_t deviceID = HIAI_NDK_GetDeviceID();
    ACL_EXPECT_TRUE_R(deviceID != 0, nullptr);

    auto TensorCreateFunc = reinterpret_cast<decltype(OH_NNTensor_Create)*>(
        NDKProxy::GetSymbol("OH_NNTensor_Create"));
    return TensorCreateFunc == nullptr ? nullptr : TensorCreateFunc(deviceID, tensorDesc);
}

NN_Tensor* HIAI_NDK_NNTensor_CreateWithSize(NN_TensorDesc* tensorDesc, size_t size)
{
    size_t deviceID = HIAI_NDK_GetDeviceID();
    ACL_EXPECT_TRUE_R(deviceID != 0, nullptr);

    auto TensorCreateWithSizeFunc = reinterpret_cast<decltype(OH_NNTensor_CreateWithSize)*>(
        NDKProxy::GetSymbol("OH_NNTensor_CreateWithSize"));
    return TensorCreateWithSizeFunc == nullptr ? nullptr : TensorCreateWithSizeFunc(deviceID, tensorDesc, size);
}

NN_Tensor* HIAI_NDK_NNTensor_CreateWithFd(NN_TensorDesc* tensorDesc, int fd, size_t size, size_t offset)
{
    size_t deviceID = HIAI_NDK_GetDeviceID();
    ACL_EXPECT_TRUE_R(deviceID != 0, nullptr);

    auto TensorCreateWithFdFunc = reinterpret_cast<decltype(OH_NNTensor_CreateWithFd)*>(
        NDKProxy::GetSymbol("OH_NNTensor_CreateWithFd"));
    return TensorCreateWithFdFunc == nullptr ? nullptr : TensorCreateWithFdFunc(deviceID, tensorDesc, fd, size, offset);
}


void HIAI_NDK_NNTensor_Destroy(NN_Tensor** nnTensor)
{
    auto destroyNNTensorFunc = reinterpret_cast<decltype(OH_NNTensor_Destroy)*>(
        NDKProxy::GetSymbol("OH_NNTensor_Destroy"));
    if (destroyNNTensorFunc != nullptr)
    {
        destroyNNTensorFunc(nnTensor);
        nnTensor = nullptr;
    }
}

void* HIAI_NDK_NNTensor_GetDataBuffer(NN_Tensor* nnTensor)
{
    auto getDataBufferFunc = reinterpret_cast<decltype(OH_NNTensor_GetDataBuffer)*>(
        NDKProxy::GetSymbol("OH_NNTensor_GetDataBuffer"));
    return getDataBufferFunc == nullptr ? nullptr : getDataBufferFunc(nnTensor);
}

aclError HIAI_NDK_NNTensor_GetSize(NN_Tensor* nnTensor, size_t* size)
{
    auto GetSizeFunc = reinterpret_cast<decltype(OH_NNTensor_GetSize)*>(
        NDKProxy::GetSymbol("OH_NNTensor_GetSize"));
    ACL_EXPECT_NOT_NULL(GetSizeFunc);
    OH_NN_ReturnCode retCode = GetSizeFunc(nnTensor, size);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_NNTensorDesc_GetByteSize(const NN_TensorDesc* tensorDesc, size_t *byteSize)
{
    auto GetByteSizeFunc = reinterpret_cast<decltype(OH_NNTensorDesc_GetByteSize)*>(
        NDKProxy::GetSymbol("OH_NNTensorDesc_GetByteSize"));
    ACL_EXPECT_NOT_NULL(GetByteSizeFunc);
    OH_NN_ReturnCode retCode = GetByteSizeFunc(tensorDesc, byteSize);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

}