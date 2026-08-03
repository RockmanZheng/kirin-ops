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

#ifndef STUB_NDK_HIAI_FOUNDATION_HIAI_NDK_STRUCT_DEF_H
#define STUB_NDK_HIAI_FOUNDATION_HIAI_NDK_STRUCT_DEF_H

#include <vector>
#include "hiai_foundation/hiai_aipp_param.h"
#include "hiai_foundation/hiai_options.h"

template<typename T>
T* GetType(void* p)
{
    return reinterpret_cast<T*>(p);
}

template<typename T>
const T* GetType(const void* p)
{
    return reinterpret_cast<const T*>(p);
}

typedef struct HiAIOptions HiAIOptions;

const std::vector<size_t> allDevices {123};
struct NNCompilation {
    const char* modelPath {nullptr};
    const void* modelBuffer {nullptr};
    size_t modelSize {209715200};
    HiAIOptions* hiaiOptions {nullptr};
    size_t deviceID {0};
    const char* cachePath {nullptr};
    uint32_t version {0};
    OH_NN_PerformanceMode performanceMode {OH_NN_PERFORMANCE_NONE};
    OH_NN_Priority priority {OH_NN_PRIORITY_NONE};
    bool enableFloat16 {false};
};

struct NNTensorDesc {
    OH_NN_DataType dataType {OH_NN_UNKNOWN};
    OH_NN_Format format {OH_NN_FORMAT_NONE};
    std::vector<int32_t> shape {1, 3, 64, 64};
    const char* name {nullptr};
    size_t elementCount {200};
    size_t byteSize {200};
};

struct NNTensor {
    size_t deviceID {0};
    NN_TensorDesc* tensorDesc {nullptr};
    std::vector<uint8_t> tensorData;
    size_t tensorSize {1 * 3 * 64 * 64 * sizeof(float)};
    int fd {0};
    size_t offset {0};
    std::vector<HiAI_AippParam*> aippParams;  
};

struct NNExecutor {
    OH_NNCompilation* compilation {nullptr};
    size_t inputCount {2};
    size_t outputCount {1};
    NN_OnRunDone onRunDone {nullptr};
    NN_OnServiceDied onServiceDied {nullptr};
};

struct CropConfig {
    bool cropSwitch {false};
    uint32_t cropStartPosW {0};
    uint32_t cropStartPosH {0};
    uint32_t cropSizeW {0};
    uint32_t cropSizeH {0};
};

struct ResizeConfig {
    bool resizeSwitch {false};
    uint32_t resizeW {0};
    uint32_t resizeH {0};  
};

struct PadConfig {
    bool padSwitch {false};
    uint32_t left {0};
    uint32_t right {0};
    uint32_t top {0};
    uint32_t bottom {0};
};

struct AippParaStub {
    uint32_t batchNum {0};
    uint32_t inputIndex {0};
    uint32_t inputAippIndex {0};
    HiAI_ImageFormat inputFormat {HIAI_IMAGE_FORMAT_INVALID};
    uint32_t srcImageW {0};
    uint32_t srcImageH {0};
    HiAI_ImageFormat cscInputFormat {HIAI_IMAGE_FORMAT_INVALID};
    HiAI_ImageFormat cscOutputFormat {HIAI_IMAGE_FORMAT_INVALID};
    HiAI_ImageColorSpace space {HIAI_IMAGE_COLOR_SPACE_INVALID};
    bool rbuvSwapSwitch {false};
    bool axSwapSwitch {false};
    bool singleBatchMultiCrop {false};

    std::vector<CropConfig> cropConfig;
    std::vector<ResizeConfig> resizeConfig;
    std::vector<PadConfig> padConfig;
    std::vector<std::vector<uint32_t>> paddingValues;
    std::vector<float> rotationAngle;
    std::vector<std::vector<uint32_t>> dtcMeanPixel;
    std::vector<std::vector<float>> dtcMinPixel;
    std::vector<std::vector<float>> dtcVarReciPixel;
};

struct OpDeviceOrder {
    const char* operatorName {nullptr};
    HiAI_ExecuteDevice* executeDevices {nullptr};
    size_t deviceCount {0};  
};

typedef struct HiAI_NativeHandle HiAI_NativeHandle;

typedef void (*onAllocate) (void* userData, uint32_t requiredSize, HiAI_NativeHandle* handles[], size_t* handlesSize);

typedef void (*onFree) (void* userData, HiAI_NativeHandle* handles[], size_t handlesSize);

struct HiAIOptions {
    size_t shapeCount {0};
    NN_TensorDesc** inputTensorDescs {nullptr};
    HiAI_FormatMode formatMode {HIAI_FORMAT_MODE_NCHW};
    HiAI_DynamicShapeStatus status {HIAI_DYNAMIC_SHAPE_DISABLED};
    size_t maxCacheCount {0};
    HiAI_DynamicShapeCacheMode mode {HIAI_DYNAMIC_SHAPE_CACHE_BUILT_MODEL};
    std::vector<OpDeviceOrder> opDeviceOrder;

    HiAI_ExecuteDevice* executeDevices {nullptr};
    size_t deviceCount {0};
    HiAI_FallbackMode fallbackMode {HIAI_FALLBACK_ENABLED};
    HiAI_DeviceMemoryReusePlan deviceMemoryReusePlan {HIAI_DEVICE_MEMORY_REUSE_PLAN_UNSET};
    HiAI_TuningStrategy tuningStrategy {HIAI_TUNING_STRATEGY_OFF};

    void* quantizeConfigData {nullptr};
    size_t quantizeConfigSize {0};

    HiAI_TuningMode tuningMode {HIAI_TUNING_MODE_UNSET};
    const char* cacheDir {nullptr};
    
    HiAI_BandMode bandMode {HIAI_BANDMODE_UNSET};
    bool isEnableAsync {false};
    const char* customPath {nullptr};
    
    onAllocate allocateFunc {nullptr};
    onFree freeFunc {nullptr};
    void* userData {nullptr};
};

struct HiAI_NativeHandle {
    int fd {0};
    int size {0};
    int offset {0};
};
#endif