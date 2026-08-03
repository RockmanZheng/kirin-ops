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

#include "neural_network_runtime/neural_network_core.h"

#include <thread>
#include <mutex>

#include "hiai_ndk_struct_def.h"
#include "utils/assert.h"

namespace {
NNCompilation* CreateNNCompilation()
{
    NNCompilation* nnCompilation = new (std::nothrow) NNCompilation();
    ACL_EXPECT_TRUE_R(nnCompilation != nullptr, nullptr);
    HiAIOptions* hiaiOptions = new (std::nothrow) HiAIOptions();
    if (hiaiOptions == nullptr) {
        delete nnCompilation;
        return nullptr;
    }
    nnCompilation->hiaiOptions = hiaiOptions;
    return nnCompilation;
}

void DestroyNNCompilation(NNCompilation* nnCompilation)
{
    if (nnCompilation->hiaiOptions != nullptr) {
        delete nnCompilation->hiaiOptions;
        nnCompilation->hiaiOptions = nullptr;
    }
    delete nnCompilation;
}
}

static std::mutex runMutex;

OH_NNCompilation* OH_NNCompilation_Construct(const OH_NNModel* model)
{
    (void) model;
    return nullptr;
}

OH_NNCompilation* OH_NNCompilation_ConstructWithOfflineModelFile(const char* modelPath)
{
    ACL_EXPECT_TRUE_R(modelPath != nullptr, nullptr);
    NNCompilation* nnCompilation = CreateNNCompilation();
    ACL_EXPECT_TRUE_R(nnCompilation != nullptr, nullptr);
    nnCompilation->modelPath = modelPath;
    nnCompilation->modelBuffer = static_cast<void*>(new uint8_t[nnCompilation->modelSize]);
    return reinterpret_cast<OH_NNCompilation*>(nnCompilation);
}

OH_NNCompilation* OH_NNCompilation_ConstructWithOfflineModelBuffer(const void* modelBuffer, size_t modelSize)
{
    ACL_EXPECT_TRUE_R(modelBuffer != nullptr && modelSize != 0, nullptr);
    NNCompilation* nnCompilation = CreateNNCompilation();
    ACL_EXPECT_TRUE_R(nnCompilation != nullptr, nullptr);
    nnCompilation->modelBuffer = modelBuffer;
    nnCompilation->modelSize = modelSize;
    return reinterpret_cast<OH_NNCompilation*>(nnCompilation);
}

OH_NNCompilation* OH_NNCompilation_ConstructForCache()
{
    NNCompilation* nnCompilation = CreateNNCompilation();
    ACL_EXPECT_TRUE_R(nnCompilation != nullptr, nullptr);
    return reinterpret_cast<OH_NNCompilation*>(nnCompilation);
}

OH_NN_ReturnCode OH_NNCompilation_ExportCacheToBuffer(OH_NNCompilation* compilation, const void* buffer, size_t length, size_t* modelSize)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && buffer != nullptr && length != 0 && modelSize != nullptr, OH_NN_FAILED);
    size_t size = GetType<NNCompilation>(compilation)->modelSize;
    ACL_EXPECT_TRUE_R(size >= length, OH_NN_FAILED);
    (void) memcpy(const_cast<void*>(buffer), GetType<NNCompilation>(compilation)->modelBuffer, size);
    *modelSize = size;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNCompilation_ImportCacheFromBuffer(OH_NNCompilation* compilation, const void* buffer, size_t modelSize)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr && buffer != nullptr && modelSize != 0, OH_NN_FAILED);
    GetType<NNCompilation>(compilation)->modelBuffer = buffer;
    GetType<NNCompilation>(compilation)->modelSize = modelSize;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNCompilation_Build(OH_NNCompilation* compilation)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr, OH_NN_FAILED);
    HiAIOptions* hiaiOptions = GetType<NNCompilation>(compilation)->hiaiOptions;
    ACL_EXPECT_TRUE_R(hiaiOptions != nullptr, OH_NN_FAILED);
    if (!hiaiOptions->opDeviceOrder.empty() && hiaiOptions->tuningMode != HIAI_TUNING_MODE_UNSET) {
        return OH_NN_FAILED;
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNCompilation_EnableFloat16(OH_NNCompilation* compilation, bool enableFloat16)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr, OH_NN_FAILED);
    GetType<NNCompilation>(compilation)->enableFloat16 = enableFloat16;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNCompilation_SetPriority(OH_NNCompilation *compilation, OH_NN_Priority priority)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr, OH_NN_FAILED);
    GetType<NNCompilation>(compilation)->priority = priority;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNCompilation_SetDevice(OH_NNCompilation *compilation, size_t deviceID)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr, OH_NN_FAILED);
    GetType<NNCompilation>(compilation)->deviceID = deviceID;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNExecutor_GetInputCount(const OH_NNExecutor *executor, size_t *inputCount)
{
    ACL_EXPECT_TRUE_R(executor != nullptr, OH_NN_FAILED);
    ACL_EXPECT_TRUE_R(inputCount != nullptr, OH_NN_FAILED);
    *inputCount = GetType<NNExecutor>(executor)->inputCount;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNExecutor_GetOutputCount(const OH_NNExecutor *executor, size_t *outputCount)
{
    ACL_EXPECT_TRUE_R(executor != nullptr, OH_NN_FAILED);
    ACL_EXPECT_TRUE_R(outputCount != nullptr, OH_NN_FAILED);
    *outputCount = GetType<NNExecutor>(executor)->outputCount;
    return OH_NN_SUCCESS;
}

void OH_NNCompilation_Destroy(OH_NNCompilation** compilation)
{
    if (compilation != nullptr && *compilation != nullptr) {
        NNCompilation* nnCompilation = reinterpret_cast<NNCompilation*>(*compilation);
        DestroyNNCompilation(nnCompilation);
        *compilation = nullptr;
    }
}

NN_TensorDesc* OH_NNTensorDesc_Create()
{
    NNTensorDesc* nnTensorDesc = new (std::nothrow) NNTensorDesc();
    ACL_EXPECT_TRUE_R(nnTensorDesc != nullptr, nullptr);
    return reinterpret_cast<NN_TensorDesc*>(nnTensorDesc);
}

OH_NN_ReturnCode OH_NNTensorDesc_Destroy(NN_TensorDesc** tensorDesc)
{
    if (tensorDesc != nullptr && *tensorDesc != nullptr) {
        NNTensorDesc* nnTensorDesc = reinterpret_cast<NNTensorDesc*>(*tensorDesc);
        delete nnTensorDesc;
        *tensorDesc = nullptr;
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_SetName(NN_TensorDesc* tensorDesc, const char* name)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && name != nullptr, OH_NN_FAILED);
    GetType<NNTensorDesc>(tensorDesc)->name = name;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_GetName(const NN_TensorDesc* tensorDesc, const char** name)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && name != nullptr, OH_NN_FAILED);
    *name = GetType<NNTensorDesc>(tensorDesc)->name;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_SetDataType(NN_TensorDesc* tensorDesc, OH_NN_DataType dataType)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr, OH_NN_FAILED);
    GetType<NNTensorDesc>(tensorDesc)->dataType = dataType;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_GetDataType(const NN_TensorDesc* tensorDesc, OH_NN_DataType* dataType)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && dataType != nullptr, OH_NN_FAILED);
    *dataType = GetType<NNTensorDesc>(tensorDesc)->dataType;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_SetShape(NN_TensorDesc* tensorDesc, const int32_t* shape, size_t shapeLength)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && shape != nullptr && shapeLength != 0, OH_NN_FAILED);
    std::vector<int32_t> tmpShape(shape, shape + shapeLength);
    GetType<NNTensorDesc>(tensorDesc)->shape = tmpShape;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_GetShape(const NN_TensorDesc* tensorDesc, int32_t** shape, size_t* shapeLength)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && shape != nullptr && shapeLength != nullptr, OH_NN_FAILED);
    *shape = const_cast<int32_t*>(GetType<NNTensorDesc>(tensorDesc)->shape.data());
    *shapeLength = GetType<NNTensorDesc>(tensorDesc)->shape.size();
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_SetFormat(NN_TensorDesc* tensorDesc, OH_NN_Format format)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr , OH_NN_FAILED);
    GetType<NNTensorDesc>(tensorDesc)->format = format;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_GetFormat(const NN_TensorDesc* tensorDesc, OH_NN_Format* format)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && format != nullptr, OH_NN_FAILED);
    *format = GetType<NNTensorDesc>(tensorDesc)->format;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_GetElementCount(const NN_TensorDesc* tensorDesc, size_t* elementCount)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && elementCount != nullptr, OH_NN_FAILED);
    *elementCount = GetType<NNTensorDesc>(tensorDesc)->elementCount;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensorDesc_GetByteSize(const NN_TensorDesc* tensorDesc, size_t* byteSize)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && byteSize != nullptr, OH_NN_FAILED);
    *byteSize = GetType<NNTensorDesc>(tensorDesc)->byteSize;
    return OH_NN_SUCCESS;
}

NN_Tensor* OH_NNTensor_Create(size_t deviceID, NN_TensorDesc* tensorDesc)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr, nullptr);
    NNTensor* nnTensor = new (std::nothrow) NNTensor();
    ACL_EXPECT_TRUE_R(nnTensor != nullptr, nullptr);
    nnTensor->deviceID = deviceID;
    nnTensor->tensorDesc = tensorDesc;
    nnTensor->tensorData.resize(nnTensor->tensorSize);
    return reinterpret_cast<NN_Tensor*>(nnTensor);
}

NN_Tensor* OH_NNTensor_CreateWithSize(size_t deviceID, NN_TensorDesc* tensorDesc, size_t size)
{
    ACL_EXPECT_TRUE_R(tensorDesc != nullptr && size != 0, nullptr);
    NNTensor* nnTensor = new (std::nothrow) NNTensor();
    ACL_EXPECT_TRUE_R(nnTensor != nullptr, nullptr);
    nnTensor->deviceID = deviceID;
    nnTensor->tensorDesc = tensorDesc;
    nnTensor->tensorSize = size;
    nnTensor->tensorData.resize(nnTensor->tensorSize);
    return reinterpret_cast<NN_Tensor*>(nnTensor);
}

OH_NN_ReturnCode OH_NNTensor_Destroy(NN_Tensor** tensor)
{
    if (tensor != nullptr && *tensor != nullptr) {
        NNTensor* nnTensor = reinterpret_cast<NNTensor*>(*tensor);
        nnTensor->tensorData.clear();
        delete nnTensor;
        *tensor = nullptr;
    }
    return OH_NN_SUCCESS;
}

NN_TensorDesc* OH_NNTensor_GetTensorDesc(const NN_Tensor* tensor)
{
    ACL_EXPECT_TRUE_R(tensor != nullptr, nullptr);
    return GetType<NNTensor>(tensor)->tensorDesc;
}

void* OH_NNTensor_GetDataBuffer(const NN_Tensor* tensor)
{
    ACL_EXPECT_TRUE_R(tensor != nullptr, nullptr);
    return reinterpret_cast<void*>(const_cast<uint8_t*>(GetType<NNTensor>(tensor)->tensorData.data()));
}

OH_NN_ReturnCode OH_NNTensor_GetFd(const NN_Tensor* tensor, int* fd)
{
    ACL_EXPECT_TRUE_R(tensor != nullptr && fd != nullptr, OH_NN_FAILED);
    *fd = GetType<NNTensor>(tensor)->fd;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensor_GetSize(const NN_Tensor* tensor, size_t* size)
{
    ACL_EXPECT_TRUE_R(tensor != nullptr && size != nullptr, OH_NN_FAILED);
    *size = GetType<NNTensor>(tensor)->tensorSize;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensor_GetOffset(const NN_Tensor* tensor, size_t* offset)
{
    ACL_EXPECT_TRUE_R(tensor != nullptr && offset != nullptr, OH_NN_FAILED);
    *offset = GetType<NNTensor>(tensor)->offset;
    return OH_NN_SUCCESS;
}

OH_NNExecutor* OH_NNExecutor_Construct(OH_NNCompilation* compilation)
{
    ACL_EXPECT_TRUE_R(compilation != nullptr, nullptr);
    NNExecutor* nnExecutor = new (std::nothrow) NNExecutor();
    ACL_EXPECT_TRUE_R(nnExecutor != nullptr, nullptr);
    nnExecutor->compilation = compilation;
    return reinterpret_cast<OH_NNExecutor*>(nnExecutor);
}

void OH_NNExecutor_Destroy(OH_NNExecutor** executor)
{
    std::unique_lock<std::mutex>(runMutex);
    if (executor != nullptr && *executor != nullptr) {
        NNExecutor* nnExecutor = reinterpret_cast<NNExecutor*>(*executor);
        delete nnExecutor;
        *executor = nullptr;
    }
}

OH_NN_ReturnCode OH_NNExecutor_GetOutputShape(OH_NNExecutor* executor, uint32_t outputIndex, int32_t** shape, uint32_t* shapeLength)
{
    (void) executor;
    (void) outputIndex;
    (void) shape;
    (void) shapeLength;
    return OH_NN_UNSUPPORTED;
}

OH_NN_ReturnCode OH_NNTensor_GetInputCount(const OH_NNExecutor* executor, size_t* inputCount)
{
    ACL_EXPECT_TRUE_R(executor != nullptr && inputCount != nullptr, OH_NN_FAILED);
    *inputCount = GetType<NNExecutor>(executor)->inputCount;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNTensor_GetOutputCount(const OH_NNExecutor* executor, size_t* outputCount)
{
    ACL_EXPECT_TRUE_R(executor != nullptr && outputCount != nullptr, OH_NN_FAILED);
    *outputCount = GetType<NNExecutor>(executor)->outputCount;
    return OH_NN_SUCCESS;
}

NN_TensorDesc* OH_NNExecutor_CreateInputTensorDesc(const OH_NNExecutor* executor, size_t index)
{
    (void) index;
    ACL_EXPECT_TRUE_R(executor != nullptr, nullptr);
    NNTensorDesc* nnTensorDesc = new (std::nothrow) NNTensorDesc();
    ACL_EXPECT_TRUE_R(nnTensorDesc != nullptr, nullptr);
    return reinterpret_cast<NN_TensorDesc*>(nnTensorDesc);
}

NN_TensorDesc* OH_NNExecutor_CreateOutputTensorDesc(const OH_NNExecutor* executor, size_t index)
{
    (void) index;
    ACL_EXPECT_TRUE_R(executor != nullptr, nullptr);
    NNTensorDesc* nnTensorDesc = new (std::nothrow) NNTensorDesc();
    ACL_EXPECT_TRUE_R(nnTensorDesc != nullptr, nullptr);
    return reinterpret_cast<NN_TensorDesc*>(nnTensorDesc);
}

OH_NN_ReturnCode OH_NNExecutor_SetOnRunDone(OH_NNExecutor* executor, NN_OnRunDone onRunDone)
{
    ACL_EXPECT_TRUE_R(executor != nullptr && onRunDone != nullptr, OH_NN_FAILED);
    GetType<NNExecutor>(executor)->onRunDone = onRunDone;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNExecutor_SetOnServiceDied(OH_NNExecutor* executor, NN_OnServiceDied onServiceDied)
{
    ACL_EXPECT_TRUE_R(executor != nullptr && onServiceDied != nullptr, OH_NN_FAILED);
    GetType<NNExecutor>(executor)->onServiceDied = onServiceDied;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNExecutor_RunSync(OH_NNExecutor* executor, NN_Tensor* inputTensor[], size_t inputCount,
    NN_Tensor* outputTensor[], size_t outputCount)
{
    ACL_EXPECT_TRUE_R(executor != nullptr && inputTensor != nullptr && inputCount != 0 &&
        outputTensor != nullptr && outputCount != 0, OH_NN_FAILED);
    return OH_NN_SUCCESS;
}

static void onRunDoneFunc(OH_NNExecutor* executor, void* userData)
{
    std::unique_lock<std::mutex>(runMutex);
    GetType<NNExecutor>(executor)->onRunDone(userData, OH_NN_SUCCESS, nullptr, 0);
}

static void onServiceDiedFunc(OH_NNExecutor* executor, void* userData)
{
    std::unique_lock<std::mutex>(runMutex);
    GetType<NNExecutor>(executor)->onServiceDied(userData);
}

OH_NN_ReturnCode OH_NNExecutor_RunAsync(OH_NNExecutor* executor, NN_Tensor* inputTensor[], size_t inputCount,
    NN_Tensor* outputTensor[], size_t outputCount, int32_t timeout, void* userData)
{
    ACL_EXPECT_TRUE_R(executor != nullptr && inputTensor != nullptr && inputCount != 0 &&
        outputTensor != nullptr && outputCount != 0 && userData != nullptr, OH_NN_FAILED);
    ACL_EXPECT_TRUE_R(GetType<NNExecutor>(executor)->onRunDone != nullptr &&
        GetType<NNExecutor>(executor)->onServiceDied != nullptr, OH_NN_FAILED);

    std::thread onRunDoneThread(onRunDoneFunc, executor, userData);
    onRunDoneThread.join();
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNDevice_GetAllDevicesID(const size_t** allDevicesID, uint32_t* deviceCount)
{
    ACL_EXPECT_TRUE_R(allDevicesID != nullptr && deviceCount != 0, OH_NN_FAILED);
    *allDevicesID = allDevices.data();
    *deviceCount = 1;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNDevice_GetName(size_t deviceID, const char** name)
{
    (void) deviceID;
    ACL_EXPECT_TRUE_R(name != nullptr, OH_NN_FAILED);
    *name = "HIAI_F";
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode OH_NNDevice_GetType(size_t deviceID, OH_NN_DeviceType* deviceType)
{
    (void) deviceID;
    ACL_EXPECT_TRUE_R(deviceType != nullptr, OH_NN_FAILED);
    *deviceType = OH_NN_ACCELERATOR;
    return OH_NN_SUCCESS;
}