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

#include <gtest/gtest.h>
#include <dirent.h>
#include <fstream>
#include <nlohmann/json.hpp>

#include "neural_network_runtime/neural_network_runtime_type.h"
#include "acl_manager.h"
#include "utils/ir_option.h"

using namespace testing;
using namespace hiai;

namespace Unittest::AscendclTest {
class CompilerUT : public testing::Test {
public:
    void SetUp()
    {
        std::string configPath = "";
        auto ret = aclInit(configPath.c_str());
        EXPECT_EQ(ACL_SUCCESS, ret);
    }

    void TearDown()
    {
        auto ret = aclFinalize();
    }

public:
    std::string modelPath_ = "test/framework/ut/ascendcl/test.om";
    uint32_t modelId_ = 1;
};

TEST_F(CompilerUT, acl_repeat_init_fail)
{
    std::string configPath = "";
    auto ret = aclInit(configPath.c_str());
    EXPECT_EQ(ACL_ERROR_REPEAT_INITIALIZE, ret);
}

TEST_F(CompilerUT, acl_init_and_finalize_fail)
{
    auto ret = aclFinalize();
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclFinalize();
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
    std::string configPath = "./test.config";
    ret = aclInit(configPath.c_str());
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
}

TEST_F(CompilerUT, build_model_from_file_fail)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "1";
    std::string value3 = "fp16";
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
    };
    
    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    const char* nullPath = nullptr;
    auto ret = aclgrphBuildModelFromFile(nullPath, buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_FAILED, ret);
}

/* 编译来自文件的模型，返回成功 */
TEST_F(CompilerUT, build_model_from_file_and_save)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "1";
    std::string value3 = "fp16";
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
    };

    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromFile(modelPath_.c_str(), buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);

    std::string nullPath = "";
    ret = aclgrphSaveModel(nullPath.c_str(), p);
    EXPECT_EQ(ge::GRAPH_FAILED, ret);

    std::string path = "./new_compiler_file.om";
    ret = aclgrphSaveModel(path.c_str(), p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
    
    ret = aclgrphDestroyModel(p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
}

TEST_F(CompilerUT, load_model_from_aclmdlmodel_success)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "1";
    std::string value3 = "fp16";
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
    };

    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromFile(modelPath_.c_str(), buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);

    uint32_t modelId = 0;
    auto handle = aclmdlCreateConfigHandle();
    auto aclRet = aclmdlLoadFromModel(p, handle, &modelId);
    EXPECT_EQ(ACL_SUCCESS, aclRet);
    ret = aclgrphDestroyModel(p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
    aclRet = aclmdlDestroyConfigHandle(handle);
    EXPECT_EQ(ACL_SUCCESS, aclRet);
}

TEST_F(CompilerUT, build_model_from_file_with_reuse_config)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "0";
    std::string value3 = "fp16";
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
    };
    
    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromFile(modelPath_.c_str(), buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);

    std::string path = "./new_compiler_file.om";
    ret = aclgrphSaveModel(path.c_str(), p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
    ret = aclgrphDestroyModel(p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
}

TEST_F(CompilerUT, build_model_from_file_with_config_fail)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "0";
    std::string value3 = "fp32";
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
    };
    
    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromFile(modelPath_.c_str(), buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_FAILED, ret);
}

TEST_F(CompilerUT, build_model_from_mem_fail)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "2";
    std::string value3 = "fp16";
    void* fileData = nullptr;
    uint32_t fileSize = 1024;
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
    };
    
    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromData(fileData, fileSize, buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_FAILED, ret);
}

TEST_F(CompilerUT, build_model_from_mem_success)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "2";
    std::string value3 = "fp16";
    void* fileData = malloc(1024);
    uint32_t fileSize = 1024;
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
    };
    
    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromData(fileData, fileSize, buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);

    void* data = malloc(1024);
    size_t realSize = 0;

    ret = aclgrphDestroyModel(p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
    free(fileData);
    free(data);
}

/* 编译来自文件的模型，有设备选项配置 */
TEST_F(CompilerUT, build_model_from_file_with_deviceconfig)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "0";
    std::string value3 = "fp16";
    std::string value4 = "./deviceconfig.json";
    nlohmann::json j;
    j["DeviceConfigMode"] = "MODEL_LEVEL";
    j["ModelExecuteDevice"] = "NPU";
    std::ofstream outFile("deviceconfig.json");
    if (outFile.is_open()) {
        outFile << j.dump(4); // 将JSON对象转换为字符串并写入文件，缩进4个空格
        outFile.close();
        std::cout << "JSON data written to example.json" << std::endl;
    } else {
        std::cout << "Failed to open file for writing" << std::endl;
    }
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
        {ge::ir_option::EXEC_DEVICE_CONFIG_PATH, value4.c_str()}
    };
    
    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromFile(modelPath_.c_str(), buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
    ret = aclgrphDestroyModel(p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
}

TEST_F(CompilerUT, build_model_from_file_with_opconfig)
{
    std::string value1 = "input_0_0:16,32,208,208;input_1_0:16,64,208,208";
    std::string value2 = "0";
    std::string value3 = "fp16";
    std::string value4 = "./deviceconfig.json";
    nlohmann::json j;
    j["DeviceConfigMode"] = "OP_LEVEL";
    nlohmann::json op1;
    op1["OpName"] = "Reshape";
    op1["ExecuteDevice"] = "NPU:CPU";
    j["OpExecuteDevice"].push_back(op1);
    std::ofstream outFile("deviceconfig.json");
    if (outFile.is_open()) {
        outFile << j.dump(4); // 将JSON对象转换为字符串并写入文件，缩进4个空格
        outFile.close();
        std::cout << "JSON data written to example.json" << std::endl;
    } else {
        std::cout << "Failed to open file for writing" << std::endl;
    }
    std::map<AscendString, AscendString> buildOptions {
        {ge::ir_option::INPUT_SHAPE, value1.c_str()},
        {ge::ir_option::EXEC_DISABLE_REUSED_MEMORY, value2.c_str()},
        {ge::ir_option::PRECISION_MODE_V2, value3.c_str()},
        {ge::ir_option::EXEC_DEVICE_CONFIG_PATH, value4.c_str()}
    };
    
    aclmdlModel model = nullptr;
    aclmdlModel* p = &model;
    aclmdlModel** ptr = &p;
    auto ret = aclgrphBuildModelFromFile(modelPath_.c_str(), buildOptions, ptr);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
    ret = aclgrphDestroyModel(p);
    EXPECT_EQ(ge::GRAPH_SUCCESS, ret);
}

TEST_F(CompilerUT, load_model_from_file_fail)
{
    uint32_t modelId = 0;
    const char* nullpath = nullptr;
    auto ret = aclmdlLoadFromFile(nullpath, &modelId);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);

    void* fileData = nullptr;
    uint32_t fileSize = 1024;
    uint32_t modelIdMem = 0;
    
    ret = aclmdlLoadFromMem(fileData, fileSize, &modelIdMem);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
}

TEST_F(CompilerUT, load_model_from_file)
{
    uint32_t modelId = 0;
    auto ret = aclmdlLoadFromFile(modelPath_.c_str(), &modelId);
    EXPECT_EQ(ACL_SUCCESS, ret);
}

TEST_F(CompilerUT, load_model_from_mem)
{
    void* fileData = malloc(1024);
    uint32_t fileSize = 1024;
    uint32_t modelIdMem = 0;
    auto ret = aclmdlLoadFromMem(fileData, fileSize, &modelIdMem);
    EXPECT_EQ(ACL_SUCCESS, ret);
    free(fileData);
}

TEST_F(CompilerUT, load_model_from_file_with_config)
{
    uint32_t modelId = 0;
    void* fileData = malloc(1024);
    uint32_t fileSize = 1024;
    auto handle = aclmdlCreateConfigHandle();
    int32_t priority = 1;
    int32_t perfMode = 1;
    auto ret = aclmdlSetConfigOpt(handle, ACL_MDL_PRIORITY_INT32, &priority, sizeof(int32_t));
    EXPECT_EQ(ACL_SUCCESS, ret);
    
    /* note: ACL_NPU_PERF_MODE没这个枚举
    ret = aclmdlSetConfigOpt(handle, ACL_NPU_PERF_MODE, &perfMode, sizeof(int32_t));
    EXPECT_EQ(ACL_SUCCESS, ret);*/

    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);

    size_t loadFile = ACL_MDL_LOAD_FROM_FILE;
    ret = aclmdlSetConfigOpt(handle, ACL_MDL_LOAD_TYPE_SIZET, &loadFile, sizeof(loadFile));
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);

    ret = aclmdlSetConfigOpt(handle, ACL_MDL_PATH_PTR, modelPath_.c_str(), sizeof(modelPath_));
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclmdlLoadWithConfig(handle, &modelId);
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclmdlDestroyConfigHandle(handle);
    EXPECT_EQ(ACL_SUCCESS, ret);

    uint32_t modelIdMem = 0;
    auto handle2 = aclmdlCreateConfigHandle();
    size_t memSize = static_cast<size_t>(fileSize);
    size_t loadMem = ACL_MDL_LOAD_FROM_MEM;
    ret = aclmdlSetConfigOpt(handle2, ACL_MDL_LOAD_TYPE_SIZET, &loadMem, sizeof(loadMem));
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclmdlLoadWithConfig(handle2, &modelIdMem);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
    
    ret = aclmdlSetConfigOpt(handle2, ACL_MDL_MEM_ADDR_PTR, fileData, sizeof(fileData));
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclmdlSetConfigOpt(handle2, ACL_MDL_MEM_SIZET, &memSize, sizeof(memSize));
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclmdlLoadWithConfig(handle2, &modelIdMem);
    EXPECT_EQ(ACL_SUCCESS, ret);
    ret = aclmdlDestroyConfigHandle(handle2);
    EXPECT_EQ(ACL_SUCCESS, ret);
    free(fileData);
}

TEST_F(CompilerUT, load_model_from_file_with_unsupport_config)
{
    void* fileData = malloc(1024);
    uint32_t fileSize = 1024;
    auto handle = aclmdlCreateConfigHandle();
    int32_t priority = 7;
    int32_t perfMode = 1;
    auto ret = aclmdlSetConfigOpt(handle, ACL_MDL_PRIORITY_INT32, &priority, sizeof(int32_t));
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
    ret = aclmdlDestroyConfigHandle(handle);
    EXPECT_EQ(ACL_SUCCESS, ret);

    auto handle1 = aclmdlCreateConfigHandle();
    size_t loadMemQ = ACL_MDL_LOAD_FROM_MEM_WITH_Q;
    ret = aclmdlSetConfigOpt(handle1, ACL_MDL_LOAD_TYPE_SIZET, &loadMemQ, sizeof(loadMemQ));
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
    ret = aclmdlDestroyConfigHandle(handle1);
    EXPECT_EQ(ACL_SUCCESS, ret);
    free(fileData);
}

// 测试用例2：分配0字节内存（应失败）
TEST_F(CompilerUT, ZeroSizeAllocation)
{
    void* ptr = nullptr;
    aclError ret = aclrtMalloc(&ptr, 0, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
    EXPECT_EQ(nullptr, ptr);
}

TEST_F(CompilerUT, UnsupportedPolicy)
{
    void* ptr = nullptr;
    const size_t size = 1024;
    aclError ret = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_HUGE_ONLY);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
    EXPECT_EQ(ptr, nullptr);
    ret = aclrtFree(ptr);
    EXPECT_EQ(ret, ACL_ERROR_INVALID_PARAM);
}

TEST_F(CompilerUT, MallocAndMemset)
{
    void* ptr = nullptr;
    const size_t size = 10;
    aclError ret = aclrtMalloc(&ptr, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    EXPECT_EQ(ret, ACL_SUCCESS);
    EXPECT_NE(ptr, nullptr);

    // 初始化缓冲区
    ret = aclrtMemset(ptr, size, 0x55, 5); // 设置前5字节
    
    // 验证结果
    uint8_t* ptr_char = static_cast<uint8_t*>(ptr);
    EXPECT_EQ(ACL_SUCCESS, ret);
    for (size_t i = 0; i < 4; i++) {
        EXPECT_EQ(ptr_char[i], 0x55);
    }

    aclDataBuffer* dataBuffer = aclCreateDataBuffer(ptr, size);
    size_t getSize = aclGetDataBufferSizeV2(dataBuffer);
    EXPECT_EQ(getSize, size);

    ret = aclDestroyDataBuffer(dataBuffer);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclrtFree(ptr);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(CompilerUT, run_execute_dynamic)
{
    aclmdlDataset* dataset = aclmdlCreateDataset();
    EXPECT_NE(dataset, nullptr);
    void* ptr = nullptr;
    const size_t size = 1 * 3 * 64 * 64 * sizeof(float);

    aclError ret = aclrtMalloc(&ptr, size + 10000, ACL_MEM_MALLOC_NORMAL_ONLY);
    aclDataBuffer* dataBuffer = aclCreateDataBuffer(ptr, size);
    size_t getSize = aclGetDataBufferSizeV2(dataBuffer);
    EXPECT_EQ(getSize, size);

    ret = aclmdlAddDatasetBuffer(dataset, dataBuffer);
    size_t dataSize = aclmdlGetDatasetNumBuffers(dataset);
    EXPECT_EQ(dataSize, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    
    aclmdlDataset* dataset1 = aclmdlCreateDataset();
    EXPECT_NE(dataset1, nullptr);

    void* ptr1 = reinterpret_cast<char*>(ptr) + size;
    ret = aclrtMalloc(&ptr1, size, ACL_MEM_MALLOC_NORMAL_ONLY);

    void* ptr2 = reinterpret_cast<char*>(ptr) + size * 2;
    ret = aclrtMalloc(&ptr2, size, ACL_MEM_MALLOC_NORMAL_ONLY);

    aclDataBuffer* dataBuffer1 = aclCreateDataBuffer(ptr1, size);
    aclDataBuffer* dataBuffer2 = aclCreateDataBuffer(ptr2, size);
    ret = aclmdlAddDatasetBuffer(dataset1, dataBuffer1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlAddDatasetBuffer(dataset, dataBuffer2);
    EXPECT_EQ(ret, ACL_SUCCESS);
    auto dataBuffer3 = aclmdlGetDatasetBuffer(dataset1, 0);
    EXPECT_EQ(dataBuffer3, dataBuffer1);

    uint32_t modelId = 0;
    ret = aclmdlLoadFromFile(modelPath_.c_str(), &modelId);
    EXPECT_EQ(ACL_SUCCESS, ret);
    const struct aclmdlIODims dims = {"", 8, {1, 2, 3, 4, 1, 2, 3, 4}};
    ret = aclmdlSetInputDynamicDims(modelId, dataset, 0, &dims);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlExecute(modelId, dataset, dataset1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    auto desc = aclmdlCreateDesc();
    ret = aclmdlGetDesc(desc, modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDataset(dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlDestroyDataset(dataset1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlUnload(modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtFree(ptr);
    EXPECT_EQ(ret, ACL_SUCCESS);
    
    ret = aclrtFree(ptr1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtFree(ptr2);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(CompilerUT, run_execute)
{
    aclmdlDataset* dataset = aclmdlCreateDataset();
    EXPECT_NE(dataset, nullptr);
    void* ptr = nullptr;
    const size_t size = 1 * 3 * 64 * 64 * sizeof(float);

    aclError ret = aclrtMalloc(&ptr, size + 10000, ACL_MEM_MALLOC_NORMAL_ONLY);
    aclDataBuffer* dataBuffer = aclCreateDataBuffer(ptr, size);
    size_t getSize = aclGetDataBufferSizeV2(dataBuffer);
    EXPECT_EQ(getSize, size);

    void* p = aclGetDataBufferAddr(dataBuffer);
    EXPECT_EQ(p, ptr);
    ret = aclmdlAddDatasetBuffer(dataset, dataBuffer);
    size_t dataSize = aclmdlGetDatasetNumBuffers(dataset);
    EXPECT_EQ(dataSize, 1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    
    aclmdlDataset* dataset1 = aclmdlCreateDataset();
    EXPECT_NE(dataset1, nullptr);

    void* ptr1 = reinterpret_cast<char*>(ptr) + size;
    ret = aclrtMalloc(&ptr1, size, ACL_MEM_MALLOC_NORMAL_ONLY);

    void* ptr2 = reinterpret_cast<char*>(ptr) + size * 2;
    ret = aclrtMalloc(&ptr2, size, ACL_MEM_MALLOC_NORMAL_ONLY);

    aclDataBuffer* dataBuffer1 = aclCreateDataBuffer(ptr1, size);
    aclDataBuffer* dataBuffer2 = aclCreateDataBuffer(ptr2, size);
    ret = aclmdlAddDatasetBuffer(dataset1, dataBuffer1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlAddDatasetBuffer(dataset, dataBuffer2);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t modelId = 0;
    auto nullDesc = aclmdlCreateDesc();
    ret = aclmdlGetDesc(nullDesc, modelId);
    EXPECT_EQ(ret, ACL_ERROR_FAILURE);
    ret = aclmdlDestroyDesc(nullDesc);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlLoadFromFile(modelPath_.c_str(), &modelId);
    EXPECT_EQ(ACL_SUCCESS, ret);

    ret = aclmdlExecute(modelId, dataset, dataset1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    auto desc = aclmdlCreateDesc();
    ret = aclmdlGetDesc(desc, modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDataset(dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlDestroyDataset(dataset1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlUnload(modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtFree(ptr);
    EXPECT_EQ(ret, ACL_SUCCESS);
    
    ret = aclrtFree(ptr1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclrtFree(ptr2);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(CompilerUT, create_tensor_and_get)
{
    OH_NN_DataType dataType = OH_NN_BOOL;
    OH_NN_Format format = OH_NN_FORMAT_ND;
    std::vector<int32_t> dims = {2, 4, 5, 5};
    NN_TensorDesc* tensorDesc = OH_NNTensorDesc_Create();
    OH_NNTensorDesc_SetDataType(tensorDesc, dataType);
    OH_NNTensorDesc_SetFormat(tensorDesc, format);
    OH_NNTensorDesc_SetShape(tensorDesc, dims.data(), dims.size());
    std::string descNameInput = "input";
    OH_NNTensorDesc_SetName(tensorDesc, descNameInput.c_str());

    NN_TensorDesc* tensorDesc1 = OH_NNTensorDesc_Create();
    OH_NNTensorDesc_SetDataType(tensorDesc1, dataType);
    OH_NNTensorDesc_SetFormat(tensorDesc1, format);
    OH_NNTensorDesc_SetShape(tensorDesc1, dims.data(), dims.size());
    std::string descNameOutput = "output";
    OH_NNTensorDesc_SetName(tensorDesc1, descNameOutput.c_str());
    auto desc = aclmdlCreateDesc();
    desc->inputDescs.push_back(tensorDesc);
    desc->outputDescs.push_back(tensorDesc1);

    size_t size = aclmdlGetNumInputs(desc);
    EXPECT_EQ(size, 1);
    size = aclmdlGetNumOutputs(desc);
    EXPECT_EQ(size, 1);
    
    size_t index = 0;
    size_t inputSize = aclmdlGetInputSizeByIndex(desc, index);
    EXPECT_EQ(inputSize, 200);
    size_t outputSize = aclmdlGetOutputSizeByIndex(desc, index);
    EXPECT_EQ(outputSize, 200);
    string inputName = aclmdlGetInputNameByIndex(desc, index);
    EXPECT_EQ(inputName, "input");
    string outputName = aclmdlGetOutputNameByIndex(desc, index);
    EXPECT_EQ(outputName, "output");
    aclFormat inputFormat = aclmdlGetInputFormat(desc, index);
    EXPECT_EQ(inputFormat, ACL_FORMAT_ND);
    aclFormat outputFormat = aclmdlGetOutputFormat(desc, index);
    EXPECT_EQ(outputFormat, ACL_FORMAT_ND);

    aclDataType inputType = aclmdlGetInputDataType(desc, index);
    EXPECT_EQ(inputType, ACL_BOOL);
    aclDataType outputType = aclmdlGetOutputDataType(desc, index);
    EXPECT_EQ(outputType, ACL_BOOL);

    auto ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}

TEST_F(CompilerUT, create_tensor_and_get_fail)
{
    auto desc = aclmdlCreateDesc();
    size_t size = aclmdlGetNumInputs(desc);
    EXPECT_EQ(size, 0);

    size = aclmdlGetNumOutputs(desc);
    EXPECT_EQ(size, 0);

    size_t index = 0;
    size_t inputSize = aclmdlGetInputSizeByIndex(desc, index);
    EXPECT_EQ(inputSize, 0);
    size_t outputSize = aclmdlGetOutputSizeByIndex(desc, index);
    EXPECT_EQ(outputSize, 0);
    
    aclFormat inputFormat = aclmdlGetInputFormat(desc, index);
    EXPECT_EQ(inputFormat, ACL_FORMAT_UNDEFINED);
    aclFormat outputFormat = aclmdlGetOutputFormat(desc, index);
    EXPECT_EQ(outputFormat, ACL_FORMAT_UNDEFINED);

    aclDataType inputType = aclmdlGetInputDataType(desc, index);
    EXPECT_EQ(inputType, ACL_DT_UNDEFINED);
    aclDataType outputType = aclmdlGetOutputDataType(desc, index);
    EXPECT_EQ(outputType, ACL_DT_UNDEFINED);

    auto ret = aclmdlDestroyDesc(desc);
    EXPECT_EQ(ret, ACL_SUCCESS);
}
}