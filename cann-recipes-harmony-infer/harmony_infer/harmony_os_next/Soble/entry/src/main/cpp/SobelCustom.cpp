/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "napi/native_api.h"
#include <hilog/log.h>
#include <rawfile/raw_file_manager.h>
#include "HIAIModelManager.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include <vector>

#undef LOG_DOMAIN
#define LOG_DOMAIN 0x0000

#undef LOG_TAG
#define LOG_TAG "CANN_KIT_DEMO"

double cpuRunTime = 0.0;
double npuRunTime = 0.0;

static napi_value NAPI_Global_processImageWithSobel(napi_env env, napi_callback_info info)
{
    // 参数解析 [buffer, width, height]
    size_t argc = 3;
    napi_value args[3];
    napi_value result;
    
    struct timeval start, end;
    gettimeofday(&start, NULL);

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    // 获取输入数据指针（BGRA_8888格式）

    napi_typedarray_type type;
    size_t length;
    void *data;
    napi_value arraybuffer;
    napi_get_typedarray_info(env, args[0], &type, &length, &data, &arraybuffer, nullptr);
    
    if (type != napi_uint8_array) {
        napi_throw_type_error(env, nullptr, "Expected a Uint8Array");
        return nullptr;
    }

    uint8_t *uint8_data = static_cast<uint8_t *>(data);

    // 解析宽高参数
    int32_t width, height;
    napi_get_value_int32(env, args[1], &width);
    napi_get_value_int32(env, args[2], &height);
    
    // 创建输出缓冲区（与原图同尺寸）

    uint8_t* outputData;
    napi_value outputBuffer;
    napi_create_arraybuffer(env, length, (void**)&outputData, &outputBuffer);
    
    // 核心处理流程 
    // 1. 转换到float32并灰度化（处理BGRA通道顺序）
    float* grayData = new float[width * height];
    for (int i = 0; i < width * height; ++i) {
        const int pixelOffset = i * 4;
        
        // BGRA通道解析
        const float b = uint8_data[pixelOffset] / 255.0f;// 通道0: B
        const float g = uint8_data[pixelOffset + 1] / 255.0f; // 通道1:
        const float r = uint8_data[pixelOffset + 2] / 255.0f; // 通道2: R
        grayData[i] = 0.299f * r + 0.587f * g + 0.114f * b; // ITU-R BT.601标准
    }

    // 2. Sobel滤波计算
    float* sobelX = new float[width * height]{0};
    
    float* sobelY = new float[width * height]{0};
    const int edgeMargin = 1; // 边缘留白
    for (int y = edgeMargin; y < height - edgeMargin; ++y) {
        for (int x = edgeMargin; x < width - edgeMargin; ++x) {
            const int center = y * width + x;

            // X方向梯度核 [-1 0 1; -2 0 2; -1 0 1]
            sobelX[center] = 
            -grayData[(y-1)*width + x-1] + grayData[(y-1)*width + x+1] +
            -2*grayData[y*width + x-1]   + 2*grayData[y*width + x+1] +
            -grayData[(y+1)*width + x-1] + grayData[(y+1)*width + x+1];

            // Y方向梯度核 [-1 -2 -1; 0 0 0; 1 2 1]
            sobelY[center] = 
            -grayData[(y-1)*width + x-1] - 2*grayData[(y-1)*width + x] - grayData[(y-1)*width + x+1] +
            grayData[(y+1)*width + x-1]  + 2*grayData[(y+1)*width + x] + grayData[(y+1)*width + x+1];
        }
    }
    
    // 3. 曼哈顿距离融合与结果归一化
    const float intensityScale = 0.3f; // 边缘强度调节系数
    for (int i = 0; i < width * height; ++i) {
        // 计算边缘强度
        float edgeValue = fabs(sobelX[i]) + fabs(sobelY[i]);
        edgeValue = fminf(edgeValue * intensityScale, 1.0f); // 限制最大值

        // 转换回BGRA_8888格式
        const uint8_t value = static_cast<uint8_t>(edgeValue * 255);
        const int pixelOffset = i * 4;
        outputData[pixelOffset] = value;      // 通道0: B
        outputData[pixelOffset + 1] = value;  // 通道1: G
        outputData[pixelOffset + 2] = value;  // 通道2: R
        outputData[pixelOffset + 3] = 255;    // 通道3: Alpha（不透明）
    }
  
    // 内存清理
    delete[] grayData;
    delete[] sobelX;
    delete[] sobelY;

    napi_create_typedarray(env, napi_uint8_array, length, outputBuffer, 0, &result);
    gettimeofday(&end, NULL);
    
    cpuRunTime = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_usec - start.tv_usec);
    cpuRunTime = cpuRunTime/1000;
    return result;
}

static napi_value NAPI_Global_InitIOTensors(napi_env env, napi_callback_info info)
{
    napi_value errorCode;
    napi_create_int32(env, 1, &errorCode);
    napi_value successCode;
    napi_create_int32(env, 0, &successCode);
    napi_status status;
    size_t argc = 1;
    napi_value argv[1];
    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Failed to parse arguments");
        return errorCode;
    }

    napi_typedarray_type type;
    size_t length = 0;
    void *data = nullptr;
    napi_value arraybuffer;
    size_t byte_offset;
    status = napi_get_typedarray_info(env, argv[0], &type, &length, &data, &arraybuffer, &byte_offset);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Failed to get typedarray info");
        return errorCode;
    }
    if (type != napi_uint8_array) {
        napi_throw_error(env, nullptr, "Invalid typedarray type");
        return errorCode;
    }

    uint8_t *bgrData = static_cast<uint8_t*>(data);
   
    std::vector<std::pair<uint8_t *, size_t>> inputData{ {bgrData, length} };
    OH_NN_ReturnCode ret = HIAIModelManager::GetInstance().InitIOTensors(inputData);
    
    return successCode;
}

static napi_value NAPI_Global_RunModel(napi_env env, napi_callback_info info) {
    napi_value errorCode;
    napi_create_int32(env, 1, &errorCode);
    napi_value successCode;
    napi_create_int32(env, 0, &successCode);
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    OH_NN_ReturnCode ret = HIAIModelManager::GetInstance().RunModel();
  
    gettimeofday(&end, NULL);
    npuRunTime = (end.tv_sec - start.tv_sec) * 1000000.0 + (end.tv_usec - start.tv_usec);
    npuRunTime = npuRunTime/1000;
    
    return successCode;
}

static napi_value NAPI_Global_GetResult(napi_env env, napi_callback_info info) {
    napi_value resList;
    napi_create_array_with_length(env, 2, &resList);
    
    const char* cpuRunResult = std::to_string(cpuRunTime).c_str();
    const char* npuRunResult = std::to_string(npuRunTime).c_str();
      
    
    napi_value jsCpuString;
    napi_value jsNpuString;
    
    napi_status status;
    
    status = napi_create_string_utf8(env, cpuRunResult, strlen(cpuRunResult) + 1, &jsCpuString);
    if (status != napi_ok) {
        OH_LOG_ERROR(LOG_APP, "Obtain cpuRunResult failed");
        return resList;
    }
    
    status = napi_create_string_utf8(env, npuRunResult, strlen(npuRunResult) + 1, &jsNpuString);
    if (status != napi_ok) {
        OH_LOG_ERROR(LOG_APP, "Obtain npuRunResult failed");
        return resList;
    }
    
    napi_set_element(env, resList, 0, jsCpuString);
    napi_set_element(env, resList, 1, jsNpuString);
    return resList;
}

static napi_value NAPI_Global_UnloadModel(napi_env env, napi_callback_info info) {
    napi_value errorCode;
    napi_create_int32(env, 1, &errorCode);
    napi_value successCode;
    napi_create_int32(env, 0, &successCode);

    OH_NN_ReturnCode ret = HIAIModelManager::GetInstance().UnloadModel();
    if (ret != OH_NN_SUCCESS) {
        OH_LOG_ERROR(LOG_APP, "UnloadModel failed");
        return errorCode;
    }
    return successCode;
}

static napi_value NAPI_Global_LoadModel(napi_env env, napi_callback_info info) {
    napi_value errorCode;
    napi_create_int32(env, 1, &errorCode);
    napi_value successCode;
    napi_create_int32(env, 0, &successCode);
    size_t argc = 2;
    napi_value argv[2] = {nullptr, nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    
    // Obtain model file resources and create a model buffer.
    NativeResourceManager *resourceMgr = OH_ResourceManager_InitNativeResourceManager(env, argv[0]);
    if (resourceMgr == nullptr) {
        return errorCode;
    }

    const char* modelPath = "SobelCustom.omc";
    RawFile *rawFile = OH_ResourceManager_OpenRawFile(resourceMgr, modelPath);
    if (rawFile == nullptr) {
        return errorCode;
    }
    long modelSize = OH_ResourceManager_GetRawFileSize(rawFile);
    std::unique_ptr<uint8_t[]> modelData = std::make_unique<uint8_t[]>(modelSize);
    int res = OH_ResourceManager_ReadRawFile(rawFile, modelData.get(), modelSize);
    if (res == 0) {
        return errorCode;
    }

    OH_NN_ReturnCode ret = HIAIModelManager::GetInstance().LoadModelFromBuffer(modelData.get(), modelSize);
    if (ret != OH_NN_SUCCESS) {
        return errorCode;
    }
    return successCode;
}
static napi_value NAPI_Global_GetSobelResult(napi_env env, napi_callback_info info)
{
    napi_value result;
    // 获取输入数据指针（BGRA_8888格式）
    uint8_t* outputData;
    napi_value outputBuffer;
    size_t length = 1*1*1022*761;
    napi_create_arraybuffer(env, length, (void**)&outputData, &outputBuffer);
    
    HIAIModelManager::GetInstance().GetResult(outputData);
    napi_create_typedarray(env, napi_uint8_array, length, outputBuffer, 0, &result);
    return result;
}

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"processImageWithSobel", nullptr, NAPI_Global_processImageWithSobel, nullptr, nullptr, nullptr, napi_default,
         nullptr},
        {"InitIOTensors", nullptr, NAPI_Global_InitIOTensors, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"RunModel", nullptr, NAPI_Global_RunModel, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"GetResult", nullptr, NAPI_Global_GetResult, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"UnloadModel", nullptr, NAPI_Global_UnloadModel, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"LoadModel", nullptr, NAPI_Global_LoadModel, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"GetSobelResult", nullptr, NAPI_Global_GetSobelResult, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&demoModule);
}
