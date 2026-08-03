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

#include <cstring>
#include <vector>

#include "acl_manager.h"
#include "acl_data.h"
#include "utils/log.h"
#include "utils/assert.h"
#include "utils/securestl.h"

using namespace ge;
using namespace hiai;

std::map<OH_NN_DataType, aclDataType> formatMap = {
    {OH_NN_FLOAT16, ACL_FLOAT16},
    {OH_NN_INT8, ACL_INT8},
    {OH_NN_UINT8, ACL_UINT8},
    {OH_NN_INT16, ACL_INT16},
    {OH_NN_UINT16, ACL_UINT16},
    {OH_NN_INT32, ACL_INT32},
    {OH_NN_UINT32, ACL_UINT32},
    {OH_NN_INT64, ACL_INT64},
    {OH_NN_UINT64, ACL_UINT64},
    {OH_NN_BOOL, ACL_BOOL}
};

constexpr size_t DATA_MEMORY_ALIGN_SIZE = 32UL;
constexpr int32_t PRIORITY_MIN = 1;
constexpr int32_t PRIORITY_MAX = 3;
uint64_t aclInitRefCount = 0;
std::shared_ptr<ACLManager> gACLManager = nullptr;
std::mutex mutex;

aclError aclInit(const char *configPath)
{
    const std::lock_guard<std::mutex> locker(mutex);
    ACL_EXPECT_TRUE_R_WITH_LOG(aclInitRefCount == 0, ACL_ERROR_REPEAT_INITIALIZE, "repeatedly initialized");
    if (configPath == nullptr || strlen(configPath) == 0) { //目前端侧只支持configpath为空
        gACLManager = make_shared_nothrow<ACLManager>();
        aclInitRefCount = 1;
        ACL_LOGI("create ACLManager success");
    } else  {
        ACL_LOGE("only support nullptr config");
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

aclError aclFinalize()
{
    const std::lock_guard<std::mutex> locker(mutex);
    ACL_EXPECT_TRUE_R_WITH_LOG(aclInitRefCount == 1, ACL_ERROR_INVALID_PARAM, "repeatedly initialized");
    gACLManager.reset();

    aclInitRefCount = 0;
    return ACL_SUCCESS;
}

namespace ge {
graphStatus aclgrphBuildModelFromFile(const char_t *graph_file, const std::map<AscendString, AscendString> &build_options, aclmdlModel** model)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ge::GRAPH_FAILED, "ACLManager is not initialized");
    ACL_EXPECT_TRUE_R_WITH_LOG(graph_file != nullptr, ge::GRAPH_FAILED, "graph file is nullptr");
    OH_NNCompilation *compilation = HIAI_NDK_NNCompilation_ConstructWithOfflineModelFile(graph_file);
    auto ret = gACLManager->BuildModel(compilation, build_options);
    if (ret != ACL_SUCCESS) {
        HIAI_NDK_NNCompilation_Destroy(&compilation);
        return ge::GRAPH_FAILED;
    }
    **model = compilation;
    return ge::GRAPH_SUCCESS;
}

graphStatus aclgrphBuildModelFromData(const void *graph_data, size_t graph_size, const std::map<AscendString, AscendString> &build_options, aclmdlModel** model)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ge::GRAPH_FAILED, "not init aclmanager");
    ACL_EXPECT_TRUE_R_WITH_LOG(graph_data != nullptr, ge::GRAPH_FAILED, "graph_data is nullptr");
    ACL_EXPECT_TRUE_R_WITH_LOG(graph_size > 0, ge::GRAPH_FAILED, "graph_size is below zero");
    OH_NNCompilation *compilation = HIAI_NDK_NNCompilation_ConstructWithOfflineModelBuffer(graph_data, graph_size);
    auto ret = gACLManager->BuildModel(compilation, build_options);
    if (ret != ACL_SUCCESS)
    {
        HIAI_NDK_NNCompilation_Destroy(&compilation);
        return ge::GRAPH_FAILED;
    }
    **model = compilation;
    return ge::GRAPH_SUCCESS;
}

graphStatus aclgrphSaveModel(const char_t *output_file, const aclmdlModel* model)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ge::GRAPH_FAILED, "not init aclmanager");
    ACL_EXPECT_TRUE_R_WITH_LOG(model != nullptr, ge::GRAPH_FAILED, "model is nullptr");
    auto ret = gACLManager->SaveModel(output_file, model);
    return ret == ACL_SUCCESS ? ge::GRAPH_SUCCESS : ge::GRAPH_FAILED;
}

graphStatus aclgrphSaveModelToBuffer(ModelBufferData &model_buffer, const aclmdlModel *model)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ge::GRAPH_FAILED, "not init aclmanager");
    ACL_EXPECT_TRUE_R_WITH_LOG(model != nullptr, ge::GRAPH_FAILED, "model is nullptr");
    ACL_EXPECT_NOT_NULL_R(model_buffer.data, ge::GRAPH_FAILED);
    size_t realSize = 0;
    auto ret = gACLManager->SaveModel(model_buffer.data.get(), model_buffer.length, &realSize, model);
    return ret == ACL_SUCCESS? ge::GRAPH_SUCCESS : ge::GRAPH_FAILED;
}

graphStatus aclgrphDestroyModel(const aclmdlModel* model)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(model != nullptr, ge::GRAPH_FAILED, "model is nullptr");
    OH_NNCompilation *compilation = reinterpret_cast<OH_NNCompilation*>(*model);
    HIAI_NDK_NNCompilation_Destroy(&compilation);
    return ge::GRAPH_SUCCESS;
}
}

aclmdlConfigHandle* aclmdlCreateConfigHandle()
{
    return new (std::nothrow) aclmdlConfigHandle();
}

aclError aclmdlDestroyConfigHandle(aclmdlConfigHandle *handle)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(handle != nullptr, ACL_ERROR_INVALID_PARAM, "the config to be destroyed is nullptr");
    delete handle;
    handle = nullptr;
    return ACL_SUCCESS;
}

aclError aclmdlSetConfigOpt(aclmdlConfigHandle *handle, aclmdlConfigAttr attr, const void *attrValue, size_t valueSize)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(handle != nullptr && attrValue != nullptr && valueSize > 0,
        ACL_ERROR_INVALID_PARAM, "the handle or attrvalue or valuesize is nullptr");
    switch (attr)
    {
        case ACL_MDL_PRIORITY_INT32:
            ACL_EXPECT_TRUE_R(valueSize == sizeof(int32_t), ACL_ERROR_INVALID_PARAM);
            if (*reinterpret_cast<const int32_t*>(attrValue) < PRIORITY_MIN || *reinterpret_cast<const int32_t*>(attrValue) > PRIORITY_MAX) {
                ACL_LOGE("priority set can not support");
                return ACL_ERROR_INVALID_PARAM;
            }
            handle->priority = *reinterpret_cast<const int32_t*>(attrValue);
            break;
        case ACL_MDL_LOAD_TYPE_SIZET:
            ACL_EXPECT_TRUE_R(valueSize == sizeof(size_t), ACL_ERROR_INVALID_PARAM);
            if (*reinterpret_cast<const size_t*>(attrValue) != ACL_MDL_LOAD_FROM_FILE &&
                *reinterpret_cast<const size_t*>(attrValue) != ACL_MDL_LOAD_FROM_MEM) {
                ACL_LOGE("load type set can not support");
                return ACL_ERROR_INVALID_PARAM;
            }
            handle->mdlLoadType = *reinterpret_cast<const size_t*>(attrValue);
            break;
        case ACL_MDL_PATH_PTR:
            handle->loadPath = reinterpret_cast<const char*>(attrValue);
            break;
        case ACL_MDL_MEM_ADDR_PTR:
            handle->mdlAddr = reinterpret_cast<const void*>(attrValue);
            break;
        case ACL_MDL_MEM_SIZET:
            ACL_EXPECT_TRUE_R(valueSize == sizeof(size_t), ACL_ERROR_INVALID_PARAM);
            handle->mdlSize = *reinterpret_cast<const size_t*>(attrValue);
            break;
        case ACL_NPU_PERF_MODE:
            ACL_EXPECT_TRUE_R(valueSize == sizeof(int32_t), ACL_ERROR_INVALID_PARAM);
            handle->prefMode = *reinterpret_cast<const int32_t*>(attrValue);
            break;
    }
    return ACL_SUCCESS;
}

aclError aclmdlLoadFromModel(const aclmdlModel *model, const aclmdlConfigHandle *handle, uint32_t *modelId)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ACL_ERROR_INVALID_PARAM, "not init aclmanager");
    ACL_EXPECT_TRUE_R_WITH_LOG(handle != nullptr, ACL_ERROR_INVALID_PARAM, "the config to be loaded is nullptr");
    if (handle->priority != UNDEFINED_CONFIG)
    {
        ACL_LOGE("the model has been compiled, can not set priority");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (handle->prefMode != UNDEFINED_CONFIG)
    {
        ACL_LOGE("the model has been compiled, can not set performance mode");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (model == nullptr)
    {
        ACL_LOGE("the compiled model failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    OH_NNCompilation *compilation = reinterpret_cast<OH_NNCompilation*>(*model);
    return gACLManager->LoadModel(compilation, modelId);
}

aclError aclmdlLoadFromFile(const char *modelPath, uint32_t *modelId)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ACL_ERROR_INVALID_PARAM, "not init aclmanager");
    ACL_EXPECT_TRUE_R_WITH_LOG(modelPath != nullptr , ACL_ERROR_INVALID_PARAM, "the model path is nullptr");
    OH_NNCompilation *compilation = HIAI_NDK_NNCompilation_ConstructWithOfflineModelFile(modelPath);
    auto ret = HIAI_NDK_NNCompilation_Build(compilation);
    ACL_EXPECT_TRUE_R_WITH_LOG(ret != ACL_ERROR_FAILURE, ACL_ERROR_INVALID_PARAM, "build model failed");
    auto aclRet = gACLManager->LoadModel(compilation, modelId);
    HIAI_NDK_NNCompilation_Destroy(&compilation);
    return aclRet;
}

aclError aclmdlLoadFromMem(const void* model, size_t modelSize, uint32_t *modelId)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ACL_ERROR_INVALID_PARAM, "not init aclmanager");
    ACL_EXPECT_TRUE_R_WITH_LOG(model != nullptr && modelSize > 0, ACL_ERROR_INVALID_PARAM, "the model is nullptr");
    OH_NNCompilation *compilation = HIAI_NDK_NNCompilation_ConstructWithOfflineModelBuffer(model, modelSize);
    auto ret = HIAI_NDK_NNCompilation_Build(compilation);
    ACL_EXPECT_TRUE_R_WITH_LOG(ret != ACL_ERROR_FAILURE, ACL_ERROR_INVALID_PARAM, "build model failed");
    auto aclRet = gACLManager->LoadModel(compilation, modelId);
    HIAI_NDK_NNCompilation_Destroy(&compilation);
    return aclRet;
}

aclError aclmdlLoadWithConfig(const aclmdlConfigHandle *handle, uint32_t *modelId)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ACL_ERROR_INVALID_PARAM, "not init aclmanager");
    ACL_EXPECT_TRUE_R_WITH_LOG(handle != nullptr, ACL_ERROR_INVALID_PARAM, "the config to be loaded is nullptr");
    return gACLManager->LoadModel(handle, modelId);
}

aclError aclrtMalloc(void **devPtr, size_t size, aclrtMemMallocPolicy policy)
{
    if (devPtr == nullptr)
    {
        ACL_LOGE("the pointer to be malloced is nullptr");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (size == 0)
    {
        ACL_LOGE("the size to be malloced is below zero");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (policy != ACL_MEM_MALLOC_NORMAL_ONLY)
    {
        ACL_LOGE("the malloc policy is not supported");
        return ACL_ERROR_INVALID_PARAM;
    }
    if ((size + (DATA_MEMORY_ALIGN_SIZE * 2)) < size) {
        ACL_LOGE("the size to be malloced is too large: %zu", size);
        return ACL_ERROR_INVALID_PARAM;
    }
    
    const size_t appendSize = DATA_MEMORY_ALIGN_SIZE *2;
    size_t alignedSize = (size + appendSize - 1) / DATA_MEMORY_ALIGN_SIZE * DATA_MEMORY_ALIGN_SIZE;
    *devPtr = malloc(alignedSize);
    if (*devPtr == nullptr)
    {
        ACL_LOGE("Malloc failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

aclError aclrtFree(void *devPtr)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(devPtr != nullptr, ACL_ERROR_INVALID_PARAM, "the free data is nullptr");
    free(devPtr);
    return ACL_SUCCESS;
}

aclError aclrtMemset(void *devPtr, size_t maxCount, int32_t value, size_t count)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(devPtr != nullptr, ACL_ERROR_INVALID_PARAM, "the memset data is nullptr");
    ACL_EXPECT_TRUE_R_WITH_LOG(maxCount > count, ACL_ERROR_INVALID_PARAM, "the memset is larger than max count");
    if (memset(devPtr, static_cast<size_t>(value),count) != devPtr)
    {
        ACL_LOGE("buffer memset failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

aclmdlDataset *aclmdlCreateDataset()
{
    aclmdlDataset *dataSet = new (std::nothrow) aclmdlDataset();
    if (dataSet == nullptr)
    {
        return nullptr;
    }
    return dataSet;
}

aclError aclmdlAddDatasetBuffer(aclmdlDataset *dataset, aclDataBuffer *dataBuffer)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(dataBuffer != nullptr && dataset != nullptr, ACL_ERROR_INVALID_PARAM,
        "the dataset or databuffer is nullptr");
    (dataset->buffers).push_back(std::shared_ptr<aclDataBuffer>(dataBuffer));
    return ACL_SUCCESS;
}

aclError aclmdlDestroyDataset(const aclmdlDataset *dataset)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(dataset != nullptr, ACL_ERROR_INVALID_PARAM, "the destroy data is nullptr");
    delete dataset;
    dataset = nullptr;
    return ACL_SUCCESS;
}

size_t aclmdlGetDatasetNumBuffers(const aclmdlDataset *dataset)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(dataset != nullptr, 0, "the data to get num is nullptr");
    return static_cast<size_t>(dataset->buffers.size());
}

aclDataBuffer* aclmdlGetDatasetBuffer(const aclmdlDataset *dataset, size_t index)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(dataset != nullptr, nullptr, "the data to get buffer is nullptr");
    return (index < dataset->buffers.size()) ? dataset->buffers[index].get() : nullptr;
}

aclmdlDesc *aclmdlCreateDesc()
{
    return new(std::nothrow) aclmdlDesc();
}

aclError aclmdlDestroyDesc(aclmdlDesc *modelDesc)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, ACL_ERROR_INVALID_PARAM, "the modeldesc to destroy is nullptr");
    if (modelDesc->inputDescs.size() != 0)
    {
        for (size_t i = 0U; i < modelDesc->inputDescs.size(); ++i)
        {
            if (modelDesc->inputDescs[i] != nullptr)
            {
                HIAI_NDK_NNTensorDesc_Destroy(&modelDesc->inputDescs[i]);
                modelDesc->inputDescs[i] = nullptr;
            }  
        }
    }
    if (modelDesc->outputDescs.size() !=0)
    {
        for (size_t i = 0U; i < modelDesc->outputDescs.size(); i++)
        {
            if (modelDesc->outputDescs[i] != nullptr)
            {
                HIAI_NDK_NNTensorDesc_Destroy(&modelDesc->outputDescs[i]);
                modelDesc->outputDescs[i] = nullptr;
            } 
        }
    }
    delete modelDesc;
    modelDesc = nullptr;
    return ACL_SUCCESS;
}

aclError aclmdlGetDesc(aclmdlDesc *modelDesc, uint32_t modelId)
{
    return gACLManager->GetTensorDesc(modelDesc, modelId);
}

size_t aclmdlGetNumInputs(aclmdlDesc *modelDesc)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, 0, "the desc to get input number is nullptr");
    return modelDesc->inputDescs.size();
}

size_t aclmdlGetNumOutputs(aclmdlDesc *modelDesc)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, 0, "the desc to destroy is nullptr");
    return modelDesc->outputDescs.size();
}

size_t aclmdlGetInputSizeByIndex(aclmdlDesc *modelDesc, size_t index)
{
    size_t inputSize = 0;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, 0, "the desc to get input number is nullptr");
    if (index >= modelDesc->inputDescs.size())
    {
        ACL_LOGE("index is out of range of inputDescs");
        return 0;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetByteSize(modelDesc->inputDescs[index], &inputSize);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("Getinputsize failed");
    }
    return inputSize;
}

size_t aclmdlGetOutputSizeByIndex(aclmdlDesc *modelDesc, size_t index)
{
    size_t outputSize = 0;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, 0, "the desc to get output number is nullptr");
    if (index >= modelDesc->outputDescs.size())
    {
        ACL_LOGE("index is out of range of outputDescs");
        return 0;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetByteSize(modelDesc->outputDescs[index], &outputSize);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("Getoutputsize failed");
    }
    return outputSize;
}

const char *aclmdlGetInputNameByIndex(const aclmdlDesc *modelDesc, size_t index)
{
    const char* name = nullptr;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, nullptr, "the desc to get input name is nullptr");
    if (index >= modelDesc->inputDescs.size())
    {
        ACL_LOGE("index is out of the range of inputDescs");
        return name;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetName(modelDesc->inputDescs[index], &name);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("GetInputName failed");
    }
    return name;
}

const char *aclmdlGetOutputNameByIndex(const aclmdlDesc *modelDesc, size_t index)
{
    const char* name = nullptr;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, nullptr, "the desc to get output name is nullptr");
    if (index >= modelDesc->outputDescs.size())
    {
        ACL_LOGE("index is out of the range of outputDescs");
        return name;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetName(modelDesc->outputDescs[index], &name);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("GetOutputName failed");
    }
    return name;
}

aclFormat aclmdlGetInputFormat(const aclmdlDesc *modelDesc, size_t index)
{
    OH_NN_Format inputFormat = OH_NN_FORMAT_NONE;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, ACL_FORMAT_UNDEFINED, "the desc to get input format is nullptr");
    if (index >= modelDesc->inputDescs.size())
    {
        ACL_LOGE("index is out of the range of inputDescs");
        return ACL_FORMAT_UNDEFINED;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetFormat(modelDesc->inputDescs[index], &inputFormat);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("GetInputName failed");
        return ACL_FORMAT_UNDEFINED;
    }
    if (inputFormat == OH_NN_FORMAT_NCHW)
    {
        return ACL_FORMAT_NCHW;
    } else if (inputFormat == OH_NN_FORMAT_NHWC)
    {
        return ACL_FORMAT_NHWC;
    } else if (inputFormat == OH_NN_FORMAT_ND)
    {
        return ACL_FORMAT_ND;
    }
    return ACL_FORMAT_UNDEFINED;
}

aclFormat aclmdlGetOutputFormat(const aclmdlDesc *modelDesc, size_t index)
{
    OH_NN_Format outputFormat = OH_NN_FORMAT_NONE;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, ACL_FORMAT_UNDEFINED, "the desc to get output format is nullptr");
    if (index >= modelDesc->outputDescs.size())
    {
        ACL_LOGE("index is out of the range of outputDescs");
        return ACL_FORMAT_UNDEFINED;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetFormat(modelDesc->outputDescs[index], &outputFormat);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("GetOutputName failed");
        return ACL_FORMAT_UNDEFINED;
    }
    if (outputFormat == OH_NN_FORMAT_NCHW)
    {
        return ACL_FORMAT_NCHW;
    } else if (outputFormat == OH_NN_FORMAT_NHWC)
    {
        return ACL_FORMAT_NHWC;
    } else if (outputFormat == OH_NN_FORMAT_ND)
    {
        return ACL_FORMAT_ND;
    }
    return ACL_FORMAT_UNDEFINED;
}

aclDataType aclmdlGetInputDataType(const aclmdlDesc * modelDesc, size_t index)
{
    OH_NN_DataType inputType = OH_NN_UNKNOWN;
    aclDataType dataType = ACL_DT_UNDEFINED;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, ACL_DT_UNDEFINED, "the desc to get input datatype is nullptr");
    if (index >= modelDesc->inputDescs.size())
    {
        ACL_LOGE("index is out of the range of inputDescs");
        return dataType;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetDataType(modelDesc->inputDescs[index], &inputType);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("getinputName failed");
        return dataType;
    }
    auto it = formatMap.find(inputType);
    if (it != formatMap.end())
    {
        dataType = it->second;
    } else {
        ACL_LOGE("don't support this inputformat");
    }
    return dataType;
}

aclDataType aclmdlGetOutputDataType(const aclmdlDesc *modelDesc, size_t index)
{
    OH_NN_DataType outputType = OH_NN_UNKNOWN;
    aclDataType dataType = ACL_DT_UNDEFINED;
    ACL_EXPECT_TRUE_R_WITH_LOG(modelDesc != nullptr, ACL_DT_UNDEFINED, "the desc to get output datatype is nullptr");
    if (index >= modelDesc->outputDescs.size())
    {
        ACL_LOGE("indexis out of the range of outputDescs");
        return dataType;
    }
    auto ret = HIAI_NDK_NNTensorDesc_GetDataType(modelDesc->outputDescs[index], &outputType);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("getoutputName failed");
        return dataType;
    }
    auto it = formatMap.find(outputType);
    if (it != formatMap.end())
    {
        dataType = it->second;
    } else {
        ACL_LOGE("don't support this outputformat");
    }
    return dataType;
}

aclError aclmdlExecute(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ACL_ERROR_INVALID_PARAM, "not init aclmanager");
    return gACLManager->RunModel(modelId, input, output);
}

aclError aclmdlUnload(uint32_t modelId)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ACL_ERROR_INVALID_PARAM, "not init aclmanager");
    return gACLManager->UnloadModel(modelId);
}

aclError aclmdlSetInputDynamicDims(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlIODims *dims)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(gACLManager != nullptr, ACL_ERROR_INVALID_PARAM, "not init aclmanager");
    return gACLManager->SetInputDynamic(modelId, dataset, index, dims);
}