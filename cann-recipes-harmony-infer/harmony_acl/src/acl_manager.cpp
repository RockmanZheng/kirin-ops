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

#include "acl_manager.h"

#include <type_traits>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>

#include <nlohmann/json.hpp>

#include "utils/log.h"
#include "utils/assert.h"
#include "utils/string_utils.h"
#include "utils/ir_option.h"

#include "acl_data.h"
#include "ndk/hiai_ndk_tensor.h"

using namespace ge;
using namespace hiai;

void parseOpLevelJsonFile(const nlohmann::json &j, std::unordered_map<std::string, HiAI_ExecuteDevice> &deviceMap)
{
    //op_level模式
    ACL_EXPECT_TRUE_VOID(j.contains("OpExecuteDevice") && j["OpExecuteDevice"].is_array());
    for (const auto& op : j["OpExecuteDevice"])
    {
        if (op.contains("OpName") && op.contains("ExecuteDevice"))
        {
            if (op["ExecuteDevice"].get<std::string>() == "NPU")
            {
                deviceMap.emplace(op["OpName"].get<std::string>(), HIAI_EXECUTE_DEVICE_NPU);
            } else if (op["ExecuteDevice"].get<std::string>() == "GPU")
            {
                deviceMap.emplace(op["OpName"].get<std::string>(), HIAI_EXECUTE_DEVICE_GPU);
            } else if (op["ExecuteDevice"].get<std::string>() == "CPU")
            {
                deviceMap.emplace(op["OpName"].get<std::string>(), HIAI_EXECUTE_DEVICE_CPU);
            }
        }
    }
}

void parseModelLevelJsonFile(const nlohmann::json &j, std::unordered_map<std::string, HiAI_ExecuteDevice> &deviceMap)
{
    //model_level模式
    if (j.contains("ModelExecuteDevice") && j["ModelExecuteDevice"].is_string())
    {
        if (j["ModelExecuteDevice"].get<std::string>() == "NPU")
        {
            deviceMap.emplace("default_model_op", HIAI_EXECUTE_DEVICE_NPU);
        } else if (j["ModelExecuteDevice"].get<std::string>() == "GPU")
        {
            deviceMap.emplace("default_model_op", HIAI_EXECUTE_DEVICE_GPU);
        } else if (j["ModelExecuteDevice"].get<std::string>() == "CPU") {
            deviceMap.emplace("default_model_op", HIAI_EXECUTE_DEVICE_CPU);
        }
    }
}

aclError parseJsonFile(const std::string& filename, std::unordered_map<std::string, HiAI_ExecuteDevice>& deviceMap)
{
    try
    {
        //读取文件内容
        std::ifstream file(filename);
        ACL_EXPECT_TRUE_WITH_LOG(file.is_open(), "filename: [%s] open failed", filename.c_str());

        //解析json
        nlohmann::json j;
        file >> j;

        //检查deviceconfigmode
        ACL_EXPECT_TRUE_R(j.contains("DeviceConfigMode"), ACL_SUCCESS);

        if (j["DeviceConfigMode"] == "OP_LEVEL")
        {
            parseOpLevelJsonFile(j, deviceMap);
        } else if (j["DeviceConfigMode"] == "MODEL_LEVEL")
        {
            parseModelLevelJsonFile(j, deviceMap);
        }
    } catch(const nlohmann::json::exception& e) {
        std::string exception_message = e.what();
        ACL_LOGE("Json parsed failed. msg: [%s]", exception_message.c_str());
    }
    return ACL_SUCCESS;
}

aclError GetDirAndFile(const std::string& resolvedPathStr, std::string& dirName, std::string& fileName)
{
    size_t lastSplitIndex = resolvedPathStr.find_last_of("/\\");
    if (lastSplitIndex == std::string::npos)
    {
        dirName = "./";
        fileName = resolvedPathStr;
    } else {
        dirName = resolvedPathStr.substr(0, lastSplitIndex +1);
        fileName = resolvedPathStr.substr(lastSplitIndex + 1);
    }

    if (fileName.empty())
    {
        dirName = "";
        ACL_LOGE("fileName is empty");
        return ACL_ERROR_FAILURE;
    }
    
    if (dirName.size() >= PATH_MAX)
    {
        fileName = "";
        ACL_LOGE("dirName exceeds PATH_MAX");
        return ACL_ERROR_FAILURE;
    }

    return ACL_SUCCESS; 
}

aclError CheckFilePath(std::string& resolvedPathStr)
{
    if (resolvedPathStr.empty())
    {
        ACL_LOGE("resolvedPath is empty");
        return ACL_ERROR_FAILURE;
    }
    
    std::string dirName = "";
    std::string outputFileName = "";
    if (GetDirAndFile(resolvedPathStr, dirName, outputFileName) != ACL_SUCCESS)
    {
        return ACL_ERROR_FAILURE;
    }

    char resolvedOutputPath[PATH_MAX + 1] = {0x00};
    if (realpath(dirName.c_str(), resolvedOutputPath) == nullptr)
    {
        ACL_LOGE("invalid output file path: %s", dirName.c_str());
        return ACL_ERROR_FAILURE;
    }
    resolvedPathStr = std::string(resolvedOutputPath) + "/" + outputFileName;
    return ACL_SUCCESS;
}

aclError WriteBufferToFile(const void* data, uint32_t size, const char* file)
{
    ACL_EXPECT_NOT_NULL_R(file, ACL_ERROR_FAILURE);
    std::string resolvedPathStr = file;
    if (CheckFilePath(resolvedPathStr) != ACL_SUCCESS)
    {
        ACL_LOGE("the file path is not real");
        return ACL_ERROR_FAILURE;
    }
    FILE* fp = fopen(resolvedPathStr.c_str(), "w+");
    if (fp == nullptr)
    {
        ACL_LOGE("open model fail, because not found file path");
        return ACL_ERROR_FAILURE;
    }
    
    uint32_t writeSize = static_cast<uint32_t>(fwrite(data, 1, size, fp));
    if (writeSize != size)
    {
        fclose(fp);
        return ACL_ERROR_FAILURE;
    }
    
    fclose(fp);
    return ACL_SUCCESS;
}

aclError ParseInputShape(const string& inputShape, std::unordered_map<string, std::vector<int32_t>> &shapeMap)
{
    std::vector<string> shapeVec = StringUtils::Split(inputShape, ';');
    for (const auto& shape : shapeVec)
    {
        std::vector<string> shapePairVec = StringUtils::Split(shape, ':');
        if (shapePairVec.size() != 2)
        {
            ACL_LOGE("Invalid input shape: %s", shape.c_str());
            return ACL_ERROR_FAILURE;
        }
        std::vector<string> shapeValueStrs = StringUtils::Split(shapePairVec[1], ',');
        if (static_cast<int32_t>(shapeValueStrs.size()) > 5)
        {
            ACL_LOGE("Invalid input shape: %s, shape dim size:%zu should <= 5", shape.c_str(), shapeValueStrs.size());
            return ACL_ERROR_FAILURE;
        }
        std::vector<int32_t> shapeValues;
        for (auto& shapeValueStr : shapeValueStrs)
        {
            if (shapeValueStr.find('.') != std::string::npos)
            {
                ACL_LOGE("Invalid parameter for input shape: %s ,expect int, but value = %s", shape.c_str(),
                    shapeValueStr.c_str());
                return ACL_ERROR_FAILURE;
            }
            long lresult = strtol((StringUtils::Trim(shapeValueStr)).c_str(), nullptr, 0);
            int32_t result = lresult;
            if (result <= 0)
            {
                ACL_LOGE("Invalid parameter for input shape: %s ,expect positive integer , but value = %d",
                    shape.c_str(), result);
                return ACL_ERROR_FAILURE;
            }
            shapeValues.push_back(result);
        }
        std::string shapeName = StringUtils::Trim(shapePairVec[0]);
        if (shapeMap.find(shapeName) != shapeMap.end())
        {
            ACL_LOGE("input node name is repeated, node name: %s", shapeName.c_str());
            return ACL_ERROR_FAILURE;
        }
        shapeMap.emplace(shapeName, shapeValues);
    }
    return ACL_SUCCESS;
}

aclError SetCompilationShapeConfig(OH_NNCompilation *compilation, std::vector<NN_TensorDesc*>& inputDescs)
{
    auto ret = HIAI_NDK_HiAIOptions_SetInputTensorShapes(compilation, inputDescs.data(), inputDescs.size());
    for (size_t i = 0; i < inputDescs.size(); i++)
    {
        if (inputDescs[i] != nullptr)
        {
            HIAI_NDK_NNTensorDesc_Destroy(&inputDescs[i]);
        }
    }
    if (ret == ACL_ERROR_FAILURE)
    {
        ACL_LOGE("set input shape failed");
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

aclError SetCompilationReuseConfig(OH_NNCompilation *compilation, std::string &reuseString)
{
    HiAI_DeviceMemoryReusePlan reusePlan = static_cast<HiAI_DeviceMemoryReusePlan>(-1);
    if (std::stoi(reuseString) == HIAI_DEVICE_MEMORY_REUSE_PLAN_LOW)
    {
        reusePlan = HIAI_DEVICE_MEMORY_REUSE_PLAN_LOW;
    } else if (std::stoi(reuseString) == HIAI_DEVICE_MEMORY_REUSE_PLAN_HIGH)
    {
        reusePlan = HIAI_DEVICE_MEMORY_REUSE_PLAN_HIGH;
    } else if (std::stoi(reuseString) == HIAI_DEVICE_MEMORY_REUSE_PLAN_UNSET)
    {
        reusePlan = HIAI_DEVICE_MEMORY_REUSE_PLAN_UNSET;
    } else {
        ACL_LOGE("The value of parameter EXEC_DISABLE_REUSED_MEMORY is not support");
        return ACL_ERROR_FAILURE;
    }
    return HIAI_NDK_HiAIOptions_SetDeviceMemoryReusePlan(compilation, reusePlan);
}

aclError SetCompilationPrecisionConfig(OH_NNCompilation *compilation, std::string &precisionString)
{
    if (precisionString == "fp16")
    {
        bool enableFloat16 = true;
        return HIAI_NDK_NNCompilation_EnableFloat16(compilation, enableFloat16);
    } else if (precisionString != "origin") {
        ACL_LOGE("The value of parameter PRECISION_MODE_V2 is not support");
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

aclError CreateTensorWithShape (std::unordered_map<string, std::vector<int32_t>> &shapeMap, std::vector<NN_TensorDesc*>& inputDescs)
{
    for (const auto& pairShape : shapeMap)
    {
        NN_TensorDesc* tensorDesc = HIAI_NDK_NNTensorDesc_Create();
        OH_NN_DataType dataType = OH_NN_FLOAT32;
        if (HIAI_NDK_NNTensorDesc_SetDataType(tensorDesc, dataType) != ACL_SUCCESS)
        {
            ACL_LOGE("SetDataType failed.");
            HIAI_NDK_NNTensorDesc_Destroy(&tensorDesc);
            return ACL_ERROR_FAILURE;
        }
        if (HIAI_NDK_NNTensorDesc_SetShape(tensorDesc, pairShape.second.data(), pairShape.second.size()) != ACL_SUCCESS)
        {
            ACL_LOGE("SetShape failed.");
            HIAI_NDK_NNTensorDesc_Destroy(&tensorDesc);
            return ACL_ERROR_FAILURE;
        }
        if (HIAI_NDK_NNTensorDesc_SetFormat(tensorDesc, OH_NN_FORMAT_NCHW) != ACL_SUCCESS)
        {
            ACL_LOGE("SetFormat failed.");
            HIAI_NDK_NNTensorDesc_Destroy(&tensorDesc);
            return ACL_ERROR_FAILURE;
        }
        inputDescs.push_back(tensorDesc);
    }
    return ACL_SUCCESS;
}

void DestroyTensors(std::vector<NN_Tensor *> &tensors)
{
    for (auto tensor : tensors)
    {
        HIAI_NDK_NNTensor_Destroy(&tensor);
    }
    tensors.clear();
}

aclError SetInputTensorData(std::vector<NN_Tensor*> inputTensors, const aclmdlDataset *input)
{
    for (size_t i = 0; i < inputTensors.size(); ++i)
    {
        void *data = HIAI_NDK_NNTensor_GetDataBuffer(inputTensors[i]);
        size_t dataSize = 0;
        HIAI_NDK_NNTensor_GetSize(inputTensors[i], &dataSize);
        if (data == nullptr)
        {
            ACL_LOGE("invalid data or dataSize");
            return ACL_ERROR_FAILURE;
        }
        if (dataSize != input->buffers[i]->size)
        {
            ACL_LOGE("invalid data or dataSize");
            return ACL_ERROR_FAILURE;
        }
        ACL_EXPECT_TRUE(memcpy(data, input->buffers[i]->data, input->buffers[i]->size) == data);
    }
    return ACL_SUCCESS;
}

aclError SetOutputTensorData(std::vector<NN_Tensor*> outputTensors, const aclmdlDataset *output)
{
    for (size_t i = 0; i < outputTensors.size(); ++i)
    {
        void *data = HIAI_NDK_NNTensor_GetDataBuffer(outputTensors[i]);
        size_t dataSize = 0;
        HIAI_NDK_NNTensor_GetSize(outputTensors[i], &dataSize);
        if (data == nullptr || dataSize != output->buffers[i]->size)
        {
            ACL_LOGE("invalid data or dataSize");
            return ACL_ERROR_FAILURE;
        }
        ACL_EXPECT_TRUE(output->buffers[i]->size >= dataSize);
        ACL_EXPECT_TRUE(memcpy(output->buffers[i]->data, data, dataSize) == output->buffers[i]->data);
    }
    return ACL_SUCCESS;
}

aclError ACLManager::BuildModel(OH_NNCompilation *compilation, const std::map<AscendString, AscendString> &build_options)
{
    ACL_EXPECT_TRUE_WITH_LOG(compilation != nullptr, "OH_NNCompilation_ConstructWithOfflineModelBuffer failed");
    aclError ret = ACL_ERROR_FAILURE;
    for (const auto &pair : build_options)
    {
        if (pair.first == ge::ir_option::INPUT_SHAPE)
        {
            std::string optionShape = pair.second.GetString();
            std::vector<NN_TensorDesc*> inputDescs;
            std::unordered_map<string, std::vector<int32_t>> shapeMap;
            ret = ParseInputShape(optionShape, shapeMap);
            ret = CreateTensorWithShape(shapeMap, inputDescs);
            ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "parse inputshape is fail");
            ret = SetCompilationShapeConfig(compilation, inputDescs);
            ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "set compilation shape config is fail");
        }
        if (pair.first == ge::ir_option::EXEC_DISABLE_REUSED_MEMORY)
        {
            std::string reuseString = pair.second.GetString();
            ret = SetCompilationReuseConfig(compilation, reuseString);
            ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "set reuse config is fail");
        }
        if (pair.first == ge::ir_option::PRECISION_MODE_V2)
        {
            std::string precisionString = pair.second.GetString();
            ret = SetCompilationPrecisionConfig(compilation, precisionString);
            ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "set precision config is fail");
        }
        if (pair.first == ge::ir_option::EXEC_DEVICE_CONFIG_PATH)
        {
            std::string filename = pair.second.GetString();
            std::unordered_map<string, HiAI_ExecuteDevice> deviceMap;
            auto result = parseJsonFile(filename, deviceMap);
            for (auto& pair : deviceMap)
            {
                HiAI_ExecuteDevice* device = &(pair.second);
                if (pair.first == "default_model_op")
                {
                    ret = HIAI_NDK_HiAIOptions_SetModelDeviceOrder(compilation, device, deviceMap.size());
                    break;
                } else {
                    ret = HIAI_NDK_HiAIOptions_SetOperatorDeviceOrder(compilation, pair.first.c_str(), device, deviceMap.size());

                }
                ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "set device config is fail");
            }
        }
    }
    return HIAI_NDK_NNCompilation_Build(compilation);
}

aclError ACLManager::SaveModel(const char_t *graph_file, const aclmdlModel* model)
{
    ACL_EXPECT_TRUE_WITH_LOG(model != nullptr, "the model to save is nullptr");
    OH_NNCompilation *compilation = reinterpret_cast<OH_NNCompilation*>(*model);
    size_t size = 200 * 1024 * 1024;
    void *data = malloc(size);
    size_t realSize = 0;
    auto ret = HIAI_NDK_NNCompilation_ExportCacheToBuffer(compilation, data, size, &realSize);
    ret = WriteBufferToFile(data, realSize, graph_file);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("save model to file failed");
        free(data);
        return ACL_ERROR_FAILURE;
    }
    free(data);
    return ACL_SUCCESS;
}

aclError ACLManager::SaveModel(const void *data, size_t size, size_t *realSize, const aclmdlModel* model)
{
    ACL_EXPECT_TRUE_WITH_LOG(model != nullptr, "the model to save is nullptr");
    OH_NNCompilation *compilation = reinterpret_cast<OH_NNCompilation*>(*model);
    auto ret = HIAI_NDK_NNCompilation_ExportCacheToBuffer(compilation, data, size, realSize);
    ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "save model to buffer failed");
    return ACL_SUCCESS;
}

aclError ACLManager::LoadModel(OH_NNCompilation *compilation, uint32_t *modelId)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(compilation != nullptr, ACL_ERROR_INVALID_PARAM, "the compilation to load is nullptr");
    auto executor = std::shared_ptr<OH_NNExecutor>(HIAI_NDK_NNExecutor_Construct(compilation),
        [](OH_NNExecutor* p) { HIAI_NDK_NNExecutor_Destroy(&p); });
    ACL_EXPECT_TRUE_R(executor != nullptr, ACL_ERROR_INVALID_PARAM);
    auto ptr = executor.get();
    auto ret = HIAI_NDK_HiAIExecutor_GetModelID(ptr, modelId);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("get modelid failed");
        HIAI_NDK_NNExecutor_Destroy(&ptr);
        return ACL_ERROR_INVALID_PARAM;
    }
    executorMap_.emplace(std::make_pair(*modelId, executor));
    return ACL_SUCCESS;
}

namespace {
aclError SetBuildConfig(OH_NNCompilation* compilation, const aclmdlConfigHandle* handle)
{
    aclError ret = ACL_ERROR_FAILURE;
    ACL_EXPECT_TRUE_WITH_LOG(compilation != nullptr, "the compilation is nullptr");
    if (handle->priority != UNDEFINED_CONFIG)
    {
        ACL_EXPECT_TRUE(handle->priority <= static_cast<int32_t>(OH_NN_PRIORITY_HIGH) &&
            handle->priority >= 0);
        OH_NN_Priority priority = static_cast<OH_NN_Priority>(handle->priority);
        ret = HIAI_NDK_NNCompilation_SetPriority(compilation, priority);
        if (ret != ACL_SUCCESS)
        {
            ACL_LOGE("set priority failed");
            HIAI_NDK_NNCompilation_Destroy(&compilation);
            return ret;
        }
    }
    if (handle->prefMode != UNDEFINED_CONFIG)
    {
        OH_NN_PerformanceMode performanceMode = static_cast<OH_NN_PerformanceMode>(handle->prefMode);
        ret = HIAI_NDK_NNCompilation_SetPerformanceMode(compilation, performanceMode);
        if (ret != ACL_SUCCESS)
        {
            ACL_LOGE("set perfmode failed");
            HIAI_NDK_NNCompilation_Destroy(&compilation);
            return ret;
        }
    }
    return ACL_SUCCESS;
}

aclError SetLoadConfig(OH_NNCompilation** compilation , const aclmdlConfigHandle* handle)
{
    if (handle->mdlLoadType ==0)
    {
        ACL_LOGE("input handle is invalid");
        return ACL_ERROR_FAILURE;
    }
    if (handle->mdlLoadType == static_cast<size_t>(ACL_MDL_LOAD_FROM_FILE))
    {
        if (handle->loadPath == nullptr)
        {
            ACL_LOGE("load with file , the file path is null");
            return ACL_ERROR_FAILURE;
        }
        *compilation = HIAI_NDK_NNCompilation_ConstructWithOfflineModelFile(handle->loadPath);
    } else if (handle->mdlLoadType == static_cast<size_t>(ACL_MDL_LOAD_FROM_MEM))
    {
        if (handle->mdlAddr == nullptr || handle->mdlSize == 0)
        {
            ACL_LOGE("load with file , the file path is null");
            return ACL_ERROR_FAILURE;
        }
        *compilation = HIAI_NDK_NNCompilation_ConstructWithOfflineModelBuffer(handle->mdlAddr, handle->mdlSize);
    }
    if (*compilation == nullptr)
    {
        ACL_LOGE("OH_NNCompilation_Construct failed");
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}
}

aclError ACLManager::LoadModel(const aclmdlConfigHandle* handle, uint32_t* modelId)
{
    ACL_EXPECT_TRUE(handle != nullptr && modelId != nullptr);
    OH_NNCompilation* compilation = nullptr;
    auto ret = SetLoadConfig(&compilation, handle);
    ACL_EXPECT_TRUE_R_WITH_LOG(ret == ACL_SUCCESS, ACL_ERROR_INVALID_PARAM, "the config to load is fail");
    ret = SetBuildConfig(compilation, handle);
    ACL_EXPECT_TRUE_R_WITH_LOG(ret == ACL_SUCCESS, ACL_ERROR_INVALID_PARAM, "the config to build is fail");
    if (HIAI_NDK_NNCompilation_Build(compilation) != ACL_SUCCESS)
    {
        ACL_LOGE("HIAI_NDK_NNCompilation_Build failed");
        HIAI_NDK_NNCompilation_Destroy(&compilation);
        return ACL_ERROR_INVALID_PARAM;
    };
    auto executor = std::shared_ptr<OH_NNExecutor>(HIAI_NDK_NNExecutor_Construct(compilation),
        [](OH_NNExecutor* p) { HIAI_NDK_NNExecutor_Destroy(&p); });
    HIAI_NDK_NNCompilation_Destroy(&compilation);
    ACL_EXPECT_TRUE_R(executor != nullptr, ACL_ERROR_INVALID_PARAM);
    auto ptr = executor.get();
    ret = HIAI_NDK_HiAIExecutor_GetModelID(ptr, modelId);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("get modelid failed");
        HIAI_NDK_NNExecutor_Destroy(&ptr);
        return ACL_ERROR_INVALID_PARAM;
    }
    executorMap_.emplace(std::make_pair(*modelId, executor));
    return ACL_SUCCESS;
}

aclError PrepareAippModelInputs(OH_NNExecutor* executor, const aclmdlDataset *input, std::vector<NN_Tensor*> &inputTensors)
{
    std::vector<HiAI_AippParam*> aippConfig;
    aclError ret = ACL_ERROR_FAILURE;
    for (size_t i = 0; i < input->aippParas.size(); ++i)
    {
        NN_TensorDesc* inputDesc = HIAI_NDK_NNExecutor_CreateInputTensorDesc(executor, i);
        ACL_EXPECT_TRUE_WITH_LOG(inputDesc != nullptr, "CreateInputTensorDesc failed");
        int32_t* shape = nullptr;
        size_t shapeLength = 0;
        ret = HIAI_NDK_NNTensorDesc_GetShape(inputDesc, &shape, &shapeLength);
        ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "NNTensorDesc_GetShape failed");
        uint32_t srcImageW = 0;
        uint32_t srcImageH = 0;
        ret = HIAI_NDK_HiAIAippParam_GetInputShape(input->aippParas[i].get(), &srcImageW, &srcImageH);
        ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "HiAIAippParam_GetInputShape failed");
        std::vector<int32_t> aippShape;
        for (size_t i = 0; i < shapeLength - 2; i++)
        {
            aippShape.push_back(shape[i]);
        }
        aippShape.push_back(static_cast<int32_t>(srcImageW));
        aippShape.push_back(static_cast<int32_t>(srcImageH));
        ret = HIAI_NDK_NNTensorDesc_SetShape(inputDesc, aippShape.data(), aippShape.size());
        ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS, "NNTensorDesc_SetShape failed");

        NN_Tensor* tensor = HIAI_NDK_NNTensor_Create(inputDesc);
        ACL_EXPECT_TRUE_WITH_LOG(tensor != nullptr, "Create input Tensor failed");
        inputTensors.push_back(tensor);

        HIAI_NDK_NNTensorDesc_Destroy(&inputDesc);
        aippConfig.push_back(input->aippParas[i].get());
    }
    ret = SetInputTensorData(inputTensors, input);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("SetInputTensorData failed");
        return ACL_ERROR_FAILURE;
    }
    ret = HIAI_NDK_HiAITensor_SetAippParams(inputTensors, aippConfig);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("SetAippParams failed");
        for (size_t i = 0; i < aippConfig.size(); ++i)
        {
            HIAI_NDK_HiAIAippParam_Destroy(&(aippConfig[i]));
        }
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

aclError PrepareNormalModelInputs(OH_NNExecutor* executor, const aclmdlDataset *input, std::vector<NN_Tensor*> &inputTensors)
{
    size_t inputCount = 0;
    auto ret = HIAI_NDK_NNExecutor_GetInputCount(executor, &inputCount);
    ACL_EXPECT_TRUE_WITH_LOG(ret == ACL_SUCCESS && inputCount == input->buffers.size(), 
        "OH_NNExecutor_GetInputCount failed, size mismatch");

    for (size_t i = 0; i < inputCount; ++i)
    {
        NN_TensorDesc* tensorDesc = HIAI_NDK_NNExecutor_CreateInputTensorDesc(executor, i);
        NN_Tensor* tensor = HIAI_NDK_NNTensor_Create(tensorDesc);
        if (tensor != nullptr)
        {
            inputTensors.push_back(tensor);
        }
        HIAI_NDK_NNTensorDesc_Destroy(&tensorDesc);
    }
    ret = SetInputTensorData(inputTensors, input);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("SetInputTensorData failed");
        return ACL_ERROR_FAILURE;
    }
    return ACL_SUCCESS;
}

OH_NNExecutor* ACLManager::GetExecutorByModelId(uint32_t modelId)
{
    std::lock_guard<std::mutex> locker(mutex_);
    const auto iter = executorMap_.find(modelId);
    ACL_EXPECT_TRUE_R_WITH_LOG(iter != executorMap_.end(), nullptr, "modelId can not find");
    return iter->second.get(); 
}

aclError ACLManager::RunModel(uint32_t modelId, const aclmdlDataset *input, aclmdlDataset *output)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(input != nullptr && output != nullptr, ACL_ERROR_INVALID_PARAM,
        "input or output is nullptr");

    auto executor = GetExecutorByModelId(modelId);
    ACL_EXPECT_NOT_NULL(executor);

    std::vector<NN_Tensor*> outputTensors;
    size_t outputCount = 0;
    auto ret = HIAI_NDK_NNExecutor_GetOutputCount(executor, &outputCount);
    ACL_EXPECT_TRUE_R_WITH_LOG(ret == ACL_SUCCESS, ACL_ERROR_INVALID_PARAM, "GetOutputCount failed");

    for (size_t i = 0; i < outputCount; i++)
    {
        NN_TensorDesc *tensorDesc = HIAI_NDK_NNExecutor_CreateOutputTensorDesc(executor, i);
        NN_Tensor *tensor = HIAI_NDK_NNTensor_CreateWithSize(tensorDesc, output->buffers[i]->size);
        if (tensor != nullptr)
        {
            outputTensors.push_back(tensor);
        }
        HIAI_NDK_NNTensorDesc_Destroy(&tensorDesc);
    }
    const auto dyna = dynamicInput_.find(modelId);
    if (dyna != dynamicInput_.end())
    {
        ret = HIAI_NDK_NNExecutor_RunSync(executor, dyna->second, outputTensors);
        DestroyTensors(dyna->second);
    } else {
        std::vector<NN_Tensor*> inputTensors;
        if (input->aippParas.size() != 0)
        {
            ret = PrepareAippModelInputs(executor, input, inputTensors);
        } else {
            ret = PrepareNormalModelInputs(executor, input, inputTensors);
        }
        if (ret == ACL_ERROR_FAILURE)
        {
            DestroyTensors(inputTensors);
            return ACL_ERROR_INVALID_PARAM;
        }
        ret = HIAI_NDK_NNExecutor_RunSync(executor, inputTensors, outputTensors);
        DestroyTensors(inputTensors);
        ACL_EXPECT_TRUE_R_WITH_LOG(ret == ACL_SUCCESS, ACL_ERROR_INVALID_PARAM, "OH_NNExecutor_RunSync failed");
    }

    ret = SetOutputTensorData(outputTensors, output);
    DestroyTensors(outputTensors);
    return ret;
}

aclError ACLManager::GetTensorDesc(aclmdlDesc *modelDesc, uint32_t modelId)
{
    auto executor = GetExecutorByModelId(modelId);
    ACL_EXPECT_NOT_NULL(executor);

    size_t inputCount = 0;
    size_t outCount = 0;
    auto ret = HIAI_NDK_NNExecutor_GetInputCount(executor, &inputCount);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("GetinputCount failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    for (size_t i = 0; i < inputCount; i++)
    {
        NN_TensorDesc *tensorDesc = HIAI_NDK_NNExecutor_CreateInputTensorDesc(executor, i);
        if (tensorDesc != nullptr)
        {
            modelDesc->inputDescs.push_back(tensorDesc);
        }
    }
    ret = HIAI_NDK_NNExecutor_GetOutputCount(executor, &outCount);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("GetinputCount failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    for (size_t i = 0; i < outCount; i++)
    {
        NN_TensorDesc *tensorDesc = HIAI_NDK_NNExecutor_CreateOutputTensorDesc(executor, i);
        if (tensorDesc != nullptr)
        {
            modelDesc->outputDescs.push_back(tensorDesc);
        }
    }
    return ACL_SUCCESS;
}

aclError ACLManager::SetInputDynamic(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlIODims *dims)
{
    (void)index;
    ACL_EXPECT_TRUE_R_WITH_LOG(dataset != nullptr, ACL_ERROR_INVALID_PARAM,
        "inputData or outputData is empty");

    auto executor = GetExecutorByModelId(modelId);
    ACL_EXPECT_NOT_NULL(executor);

    std::vector<NN_Tensor*> inputTensors;
    size_t inputCount = 0;
    auto ret = HIAI_NDK_NNExecutor_GetInputCount(executor, &inputCount);
    ACL_EXPECT_TRUE_R_WITH_LOG(ret == ACL_SUCCESS && inputCount == dataset->buffers.size(),
        ACL_ERROR_INVALID_PARAM, "OH_NNExecutor_GetInputCount failed, size mismatch");

    size_t descCount = 0;
    for (size_t i = 0; i < inputCount; ++i)
    {
        NN_TensorDesc *tensorDesc = HIAI_NDK_NNExecutor_CreateInputTensorDesc(executor, i);
        int32_t* shape = nullptr;
        size_t shapeLength = 0;
        ret = HIAI_NDK_NNTensorDesc_GetShape(tensorDesc, &shape, &shapeLength);
        ACL_EXPECT_TRUE(ret == ACL_SUCCESS);
        if (descCount + shapeLength > dims->dimCount)
        {
            ACL_LOGE("input tensor desc dim size is larger than dynamic dims dimssize");
            HIAI_NDK_NNTensorDesc_Destroy(&tensorDesc);
            return ACL_ERROR_INVALID_PARAM;
        }
        std::vector<int32_t> tmpShapes;
        for (size_t i = 0; i < shapeLength; i++) {
            tmpShapes.push_back(static_cast<int32_t>(*(dims->dims + descCount + i)));
        }
        ret = HIAI_NDK_NNTensorDesc_SetShape(tensorDesc, tmpShapes.data(), shapeLength);
        descCount = descCount + shapeLength;
        NN_Tensor *tensor = HIAI_NDK_NNTensor_Create(tensorDesc);
        if (tensor != nullptr)
        {
            inputTensors.push_back(tensor);
        }
        HIAI_NDK_NNTensorDesc_Destroy(&tensorDesc);
    }
    ret = SetInputTensorData(inputTensors, dataset);
    if (ret != ACL_SUCCESS)
    {
        ACL_LOGE("SetInputTensorData failed");
        DestroyTensors(inputTensors);
        return ACL_ERROR_INVALID_PARAM;
    }

    dynamicInput_.emplace(std::make_pair(modelId, inputTensors));
    return ACL_SUCCESS;
}

aclError ACLManager::UnloadModel(uint32_t modelId)
{
    const auto iter = executorMap_.find(modelId);
    if (iter == executorMap_.end())
    {
        ACL_LOGE("modelId can not find");
        return ACL_ERROR_INVALID_PARAM;
    }
    executorMap_.erase(modelId);
    return ACL_SUCCESS;
}

ACLManager::~ACLManager()
{
    for (auto& pair : dynamicInput_)
    {
        DestroyTensors(pair.second);
    }
    dynamicInput_.clear();
    executorMap_.clear();
}