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
#include "neural_network_runtime/neural_network_core.h"
#include "neural_network_runtime/neural_network_runtime_type.h"
#include "acl_manager.h"

using namespace testing;
using namespace hiai;

namespace Unittest::AscendclTest {
namespace {
constexpr uint32_t BATCH_NUM = 3;
}

class AscendclAippUT : public testing::Test {
public:
    void SetUp()
    {
        std::string configPath = "";
        auto ret = aclInit(configPath.c_str());
        EXPECT_EQ(ACL_SUCCESS, ret);
        aippParam_ = aclmdlCreateAIPP(BATCH_NUM);
    }

    void TearDown()
    {
        aclmdlDestroyAIPP(aippParam_);
        aippParam_ = nullptr;
        auto ret = aclFinalize();
        EXPECT_EQ(ACL_SUCCESS, ret);
    }

public:
    std::string modelPath_ = "./test/framework/ut/ascendcl/test.om";
    aclmdlAIPP* aippParam_ { nullptr };
};

/* set和get有效的input format */
TEST_F(AscendclAippUT, aippParam_image_format_0)
{
    aclAippInputFormat imageFormat = ACL_YUV420SP_U8;
    HiAI_ImageFormat hiImageFormat = HIAI_YUV420SP_U8;
    auto ret = aclmdlSetAIPPInputFormat(aippParam_, imageFormat);
    EXPECT_EQ(ACL_SUCCESS, ret);
    HiAI_AippParam* hiAippParam = reinterpret_cast<HiAI_AippParam*>(aippParam_);
    auto retImageFormat = HMS_HiAIAippParam_GetInputFormat(hiAippParam);
    EXPECT_EQ(hiImageFormat, retImageFormat);

    imageFormat = ACL_YUYV_U8;
    hiImageFormat = HIAI_YUYV_U8;
    ret = aclmdlSetAIPPInputFormat(aippParam_, imageFormat);
    EXPECT_EQ(ACL_SUCCESS, ret);
    
    HiAI_AippParam* hiAippParam1 = reinterpret_cast<HiAI_AippParam*>(aippParam_);
    retImageFormat = HMS_HiAIAippParam_GetInputFormat(hiAippParam);
    EXPECT_EQ(hiImageFormat, retImageFormat);
}

/* set和get非法的input format */
TEST_F(AscendclAippUT, aippParam_image_format_1)
{
    aclAippInputFormat imageFormat = ACL_RAW10;
    auto ret = aclmdlSetAIPPInputFormat(aippParam_, imageFormat);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, ret);
}

/* set和get有效的input shape */
TEST_F(AscendclAippUT, aippParam_image_shape_0)
{
    uint32_t srcImageW = 16;
    uint32_t srcImageH = 16;
    auto retCode = aclmdlSetAIPPSrcImageSize(aippParam_, srcImageW, srcImageH);
    EXPECT_EQ(ACL_SUCCESS, retCode);

    srcImageW = 4096;
    srcImageH = 4096;

    retCode = aclmdlSetAIPPSrcImageSize(aippParam_, srcImageW, srcImageH);
    EXPECT_EQ(ACL_SUCCESS, retCode);
}

/* set和get非法的input shape */
TEST_F(AscendclAippUT, aippParam_image_shape_1)
{
    uint32_t srcImageW = 80;
    uint32_t srcImageH = 80;
    auto retCode = aclmdlSetAIPPSrcImageSize(aippParam_, srcImageW, srcImageH);
    EXPECT_EQ(ACL_SUCCESS, retCode);

    srcImageW = 4096;
    srcImageH = 4096 + 1;

    retCode = aclmdlSetAIPPSrcImageSize(aippParam_, srcImageW, srcImageH);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

/* set合法的scf config */
TEST_F(AscendclAippUT, aippParam_scf_config_1)
{
    uint32_t srcImageW = 80;
    uint32_t srcImageH = 80;
    auto retCode = aclmdlSetAIPPSrcImageSize(aippParam_, srcImageW, srcImageH);
    EXPECT_EQ(ACL_SUCCESS, retCode);

    int32_t scfInputSizeW = 80;
    int32_t scfInputSizeH = 80;
    int32_t scfOutputSizeW = 16;
    int32_t scfOutputSizeH = 16;
    int8_t scfSwitch = 1;
    uint64_t batchIndex = 1;
    retCode = aclmdlSetAIPPScfParams(aippParam_, scfSwitch, scfInputSizeW, scfInputSizeH,
        scfOutputSizeW, scfOutputSizeH, batchIndex);
    EXPECT_EQ(ACL_SUCCESS, retCode);   
}

/* set有效和非法的rbuvSwapSwitch axSwapSwitch */
TEST_F(AscendclAippUT, aippParam_rbuvSwap_axSwap_0)
{
    int8_t rbuvSwapSwitch = 1;
    int8_t axSwapSwitch = 1;
    auto retCode = aclmdlSetAIPPRbuvSwapSwitch(aippParam_, rbuvSwapSwitch);
    EXPECT_EQ(ACL_SUCCESS, retCode);
    retCode = aclmdlSetAIPPAxSwapSwitch(aippParam_, axSwapSwitch);
    EXPECT_EQ(ACL_SUCCESS, retCode);

    rbuvSwapSwitch = 10;
    axSwapSwitch = 10;
    retCode = aclmdlSetAIPPRbuvSwapSwitch(aippParam_, rbuvSwapSwitch);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
    retCode = aclmdlSetAIPPAxSwapSwitch(aippParam_, axSwapSwitch);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

TEST_F(AscendclAippUT, aippParam_getsize)
{
    uint64_t batchSize = 1;
    size_t size = 0;
    auto retCode = aclmdlGetAippDataSize(batchSize, &size);
    EXPECT_EQ(ACL_SUCCESS, retCode);
    EXPECT_EQ(232, size);
}

/* set非法的scf config */
TEST_F(AscendclAippUT, aippParam_scf_config_2)
{
    uint32_t srcImageW = 80;
    uint32_t srcImageH = 80;
    auto retCode = aclmdlSetAIPPSrcImageSize(aippParam_, srcImageW, srcImageH);
    EXPECT_EQ(ACL_SUCCESS, retCode);

    int32_t scfInputSizeW = 81;
    int32_t scfInputSizeH = 81;
    int32_t scfOutputSizeW = 5;
    int32_t scfOutputSizeH = 5;
    int8_t scfSwitch = 1;
    uint64_t batchIndex = 1;
    retCode = aclmdlSetAIPPScfParams(aippParam_, scfSwitch, scfInputSizeW, scfInputSizeH,
        scfOutputSizeW, scfOutputSizeH, batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

/* set有效和非法的pad config */
TEST_F(AscendclAippUT, aippParam_pad_config_0)
{
    uint64_t batchIndex = 1;
    int32_t  leftPadSize = 10;
    int32_t rightPadSize = 10;
    int32_t topPadSize = 10;
    int32_t bottomPadSize= 10;
    int8_t paddingSwitch = 1;
    auto retCode = aclmdlSetAIPPPaddingParams(aippParam_, paddingSwitch, topPadSize, bottomPadSize, leftPadSize, rightPadSize,
        batchIndex);
    EXPECT_EQ(ACL_SUCCESS, retCode);

    paddingSwitch = 10;
    retCode = aclmdlSetAIPPPaddingParams(aippParam_, paddingSwitch, topPadSize, bottomPadSize, leftPadSize, rightPadSize,
        batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);

    leftPadSize = 32 + 1;
    rightPadSize = 32 + 1;
    topPadSize = 32 + 1;
    bottomPadSize= 32 + 1;
    retCode = aclmdlSetAIPPPaddingParams(aippParam_, paddingSwitch, topPadSize, bottomPadSize, leftPadSize, rightPadSize,
        batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

/* set有效和非法的crop config */
TEST_F(AscendclAippUT, aippParam_crop_config_0)
{
    uint64_t batchlndex = 1;
    int32_t cropStartPosW = 10;
    int32_t cropStartPosH = 10;
    int32_t cropSizeW = 10;
    int32_t cropSizeH = 10;
    int8_t cropSwitch = 1;
    auto retCode = aclmdlSetAIPPCropParams(aippParam_, cropSwitch, cropStartPosW, cropStartPosH,
        cropSizeW, cropSizeH, batchlndex);
    EXPECT_EQ(ACL_SUCCESS, retCode);

    cropSwitch = 10;
    retCode = aclmdlSetAIPPCropParams(aippParam_, cropSwitch, cropStartPosW, cropStartPosH,
        cropSizeW, cropSizeH, batchlndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);

    cropStartPosW = 4095 + 1;
    cropStartPosH = 4095 + 1;
    retCode = aclmdlSetAIPPCropParams(aippParam_, cropSwitch, cropStartPosW, cropStartPosH,
        cropSizeW, cropSizeH, batchlndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);

    cropSizeW = 0;
    cropSizeH = 0;
    retCode = aclmdlSetAIPPCropParams(aippParam_, cropSwitch, cropStartPosW, cropStartPosH,
        cropSizeW, cropSizeH, batchlndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

/* set有效和非法的DtcPixelMean config */
TEST_F(AscendclAippUT, aippParam_DtcPixelMean_config_0)
{
    uint64_t batchIndex = 1;
    int16_t dtcPixelMeanChn0 = 10;
    int16_t dtcPixelMeanChn1 = 10;
    int16_t dtcPixelMeanChn2 = 10;
    int16_t dtcPixelMeanChn3 = 10;

    auto retCode = aclmdlSetAIPPDtcPixelMean(aippParam_,
        dtcPixelMeanChn0, dtcPixelMeanChn1, dtcPixelMeanChn2, dtcPixelMeanChn3, batchIndex);
    EXPECT_EQ(ACL_SUCCESS, retCode);
    
    dtcPixelMeanChn0 = 255 + 1;
    retCode = aclmdlSetAIPPDtcPixelMean(aippParam_,
        dtcPixelMeanChn0, dtcPixelMeanChn1, dtcPixelMeanChn2, dtcPixelMeanChn3, batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);

    dtcPixelMeanChn1 = -1;
    retCode = aclmdlSetAIPPDtcPixelMean(aippParam_,
        dtcPixelMeanChn0, dtcPixelMeanChn1, dtcPixelMeanChn2, dtcPixelMeanChn3, batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

/* set有效和非法的DtcPixelMean config */
TEST_F(AscendclAippUT, aippParam_DtcPixelMin_config_0)
{
    uint64_t batchIndex = 1;
    float dtcPixelMinChn0 = 10.0f;
    float dtcPixelMinChn1 = 10.0f;
    float dtcPixelMinChn2 = 10.0f;
    float dtcPixelMinChn3 = 10.0f;

    auto retCode = aclmdlSetAIPPDtcPixelMin(aippParam_,
        dtcPixelMinChn0, dtcPixelMinChn1, dtcPixelMinChn2, dtcPixelMinChn3, batchIndex);
    EXPECT_EQ(ACL_SUCCESS, retCode);
    
    dtcPixelMinChn0 = 255.0f + 1.0f;
    retCode = aclmdlSetAIPPDtcPixelMin(aippParam_,
        dtcPixelMinChn0, dtcPixelMinChn1, dtcPixelMinChn2, dtcPixelMinChn3, batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);

    dtcPixelMinChn1 = -1.0f;
    retCode = aclmdlSetAIPPDtcPixelMin(aippParam_,
        dtcPixelMinChn0, dtcPixelMinChn1, dtcPixelMinChn2, dtcPixelMinChn3, batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

/* set有效和非法的DtcPixelVarReci config */
TEST_F(AscendclAippUT, aippParam_DtcPixelVarReci_config_0)
{
    uint64_t batchIndex = 1;
    float dtcPixelVarReciChn0 = 10.0f;
    float dtcPixelVarReciChn1 = 10.0f;
    float dtcPixelVarReciChn2 = 10.0f;
    float dtcPixelVarReciChn3 = 10.0f;

    auto retCode = aclmdlSetAIPPPixelVarReci(aippParam_,
        dtcPixelVarReciChn0, dtcPixelVarReciChn1, dtcPixelVarReciChn2, dtcPixelVarReciChn3, batchIndex);
    EXPECT_EQ(ACL_SUCCESS, retCode);
    
    dtcPixelVarReciChn0 = 65504.0f + 1.0f;
    retCode = aclmdlSetAIPPPixelVarReci(aippParam_,
        dtcPixelVarReciChn0, dtcPixelVarReciChn1, dtcPixelVarReciChn2, dtcPixelVarReciChn3, batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);

    dtcPixelVarReciChn1 = -65505.0f;
    retCode = aclmdlSetAIPPPixelVarReci(aippParam_,
        dtcPixelVarReciChn0, dtcPixelVarReciChn1, dtcPixelVarReciChn2, dtcPixelVarReciChn3, batchIndex);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

TEST_F(AscendclAippUT, aippParam_CscParams_config_fail)
{
    int16_t matrixR0C0 = 256;
    int16_t matrixR0C1 = 0;
    int16_t matrixR0C2 = 359;
    int16_t matrixR1C0 = 256;
    int16_t matrixR1C1 = -88;
    int16_t matrixR1C2 = -183;
    int16_t matrixR2C0 = 256;
    int16_t matrixR2C1 = 454;
    int16_t matrixR2C2 = 0;
    uint8_t outputBias0 = 40;
    uint8_t outputBias1 = 41;
    uint8_t outputBias2 = 42;
    uint8_t inputBias0 = 50;
    uint8_t inputBias1 = 51;
    uint8_t inputBias2 = 52;

    auto retCode = aclmdlSetAIPPCscParams(aippParam_, 1,
        matrixR0C0, matrixR0C1, matrixR0C2,
        matrixR1C0, matrixR1C1, matrixR1C2,
        matrixR2C0, matrixR2C1, matrixR2C2,
        outputBias0, outputBias1, outputBias2,
        inputBias0, inputBias1, inputBias2);
    EXPECT_EQ(ACL_ERROR_INVALID_PARAM, retCode);
}

TEST_F(AscendclAippUT, aippParam_CscParams_config_success)
{
    int16_t matrixR0C0 = 256;
    int16_t matrixR0C1 = 0;
    int16_t matrixR0C2 = 0;
    int16_t matrixR1C0 = 0;
    int16_t matrixR1C1 = 0;
    int16_t matrixR1C2 = 0;
    int16_t matrixR2C0 = 0;
    int16_t matrixR2C1 = 0;
    int16_t matrixR2C2 = 0;
    uint8_t outputBias0 = 0;
    uint8_t outputBias1 = 0;
    uint8_t outputBias2 = 0;
    uint8_t inputBias0 = 0;
    uint8_t inputBias1 = 0;
    uint8_t inputBias2 = 0;

    auto retCode = aclmdlSetAIPPCscParams(aippParam_, 1,
        matrixR0C0, matrixR0C1, matrixR0C2,
        matrixR1C0, matrixR1C1, matrixR1C2,
        matrixR2C0, matrixR2C1, matrixR2C2,
        outputBias0, outputBias1, outputBias2,
        inputBias0, inputBias1, inputBias2);
    EXPECT_EQ(ACL_SUCCESS, retCode);
}

TEST_F(AscendclAippUT, set_aipp_config)
{
    auto aippconfig = aclmdlCreateAIPP(1);
    EXPECT_NE(nullptr, aippconfig);

    auto ret = aclmdlSetAIPPInputFormat(aippconfig, ACL_YUV420SP_U8);
    EXPECT_EQ(ACL_SUCCESS, ret);

    ret = aclmdlSetAIPPSrcImageSize(aippconfig, 320, 240);
    EXPECT_EQ(ACL_SUCCESS, ret);

    ret = aclmdlSetAIPPCropParams(aippconfig, 1, 50, 50, 150, 150, 0);
    EXPECT_EQ(ACL_SUCCESS, ret);

    ret = aclmdlSetAIPPPaddingParams(aippconfig, 1, 32, 0, 32, 0, 0);
    EXPECT_EQ(ACL_SUCCESS, ret);

    aclmdlDataset* dataset = aclmdlCreateDataset();
    EXPECT_NE(nullptr, dataset);
    void* ptr = nullptr;
    const size_t size = 1 * 3 * 64 * 64 * sizeof(float);
    ret = aclrtMalloc(&ptr, size + 10000, ACL_MEM_MALLOC_NORMAL_ONLY);
    aclDataBuffer *dataBuffer = aclCreateDataBuffer(ptr, size);
    ret = aclmdlAddDatasetBuffer(dataset, dataBuffer);
    EXPECT_EQ(ret, ACL_SUCCESS);

    aclmdlDataset* dataset1 = aclmdlCreateDataset();
    EXPECT_NE(dataset1, nullptr);
    void* ptr1 = reinterpret_cast<uint8_t*>(ptr) + size;
    ret = aclrtMalloc(&ptr1, size, ACL_MEM_MALLOC_NORMAL_ONLY);
    void* ptr2 = reinterpret_cast<uint8_t*>(ptr) + size * 2;
    ret = aclrtMalloc(&ptr2, size, ACL_MEM_MALLOC_NORMAL_ONLY);

    aclDataBuffer* dataBuffer1 = aclCreateDataBuffer(ptr1, size);
    aclDataBuffer* dataBuffer2 = aclCreateDataBuffer(ptr2, size);
    ret = aclmdlAddDatasetBuffer(dataset1, dataBuffer1);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlAddDatasetBuffer(dataset, dataBuffer2);
    EXPECT_EQ(ret, ACL_SUCCESS);

    uint32_t modelId = 0;
    ret = aclmdlLoadFromFile(modelPath_.c_str(), &modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);
    
    ret = aclmdlSetAIPPByInputIndex(0, dataset, 0, aippconfig);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlExecute(modelId, dataset, dataset1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlDestroyDataset(dataset);
    EXPECT_EQ(ret, ACL_SUCCESS);
    ret = aclmdlDestroyDataset(dataset1);
    EXPECT_EQ(ret, ACL_SUCCESS);

    ret = aclmdlUnload(modelId);
    EXPECT_EQ(ret, ACL_SUCCESS);

    (void) aclrtFree(ptr);
    (void) aclrtFree(ptr1);
    (void) aclrtFree(ptr2);
}
}