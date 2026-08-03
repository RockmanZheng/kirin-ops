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
#include <algorithm>

#include "acl_manager.h"
#include "utils/log.h"

using namespace std;
using namespace hiai;
using namespace ge;

enum class ImageFormat {
    YUV420SP = 0,
    XRGB8888,
    YUV400,
    ARGB8888,
    YUYV,
    YUV422SP,
    AYUV444,
    RGB888,
    BGR888,
    YUV444SP,
    YVU444SP,
    INVALID = 255  
};

enum class ImageColorSpace {
    JPEG = 0,
    BT_601_NARROW,
    BT_601_FULL,
    BT_709_NARROW,
    INVALID = 255
};

const static vector<vector<vector<int32_t>>> YUV_TO_RGB = {
    {{256, 0, 359}, {256, -88, -183}, {256, 454, 0}},
    {{298, 0, 409}, {298, -100, -208}, {298, 516, 0}},
    {{256, 0, 359}, {256, -88, -183}, {256, 454, 0}},
    {{298, 0, 460}, {298, -55, -137}, {298, 541, 0}}};

const static vector<vector<vector<int32_t>>> RGB_TO_YUV = {
    {{77, 150, 29}, {-43, -85, 128}, {128, -107, -21}},
    {{66, 129, 25}, {-38, -74, 112}, {112, -94, -18}},
    {{77, 150, 29}, {-43, -85, 128}, {128, -107, -21}},
    {{47, 157, 16}, {-26, -87, 112}, {112, -102, -10}}};

const static vector<int32_t> YUV_TO_GRAY = {256, 0, 0, 0, 0, 0, 0, 0, 0};
const static vector<int32_t> RGB_TO_GRAY = {76, 150, 30, 0, 0, 0, 0, 0, 0};

bool IsCscMatrixToGray(const vector<int32_t>& cscPara, ImageFormat& inputFormat)
{
    if (cscPara == YUV_TO_GRAY) {
        inputFormat = ImageFormat::YUV420SP;
        return true;
    }
    if (cscPara == RGB_TO_GRAY) {
        inputFormat = ImageFormat::XRGB8888;
        return true;
    }
    return false;
}

bool CheckYUV2RGBOrBGREqual(size_t k, size_t rgb, const vector<int32_t>& cscPara)
{
    size_t t = rgb;
    for (size_t i = 0; i < cscPara.size(); i++) {
        size_t j = i % 3;
        if (cscPara[i] != YUV_TO_RGB[k][t][j])
        {
            return false;
        }
        if (j == 2 && rgb == 0) {
             t++;
        }
        if (j == 2 && rgb == 2) {
             t--;
        }  
    }
    return true;
}

bool IsCscMatrixToRGBOrBGR(const vector<int>& cscPara, int keyIdx, ImageFormat& inputFormat,
    ImageFormat& targetFormat, ImageColorSpace& colorSpace)
{
    for (int k = 0;k < 4; k++) {
        for (int j = 0; j < 3; j++) {
            if (cscPara[j] != YUV_TO_RGB[k][keyIdx][j])
            {
                break;
            }
            if (j == 2 && CheckYUV2RGBOrBGREqual(k, keyIdx, cscPara))
            {
                inputFormat = ImageFormat::YUV420SP;
                targetFormat = keyIdx == 0 ? ImageFormat::RGB888 : ImageFormat::BGR888;
                colorSpace = static_cast<ImageColorSpace>(k);
                return true;
            }
        }
    }
    return false;
}

bool CheckRGB2YUVOrYVUEqual(size_t k, size_t yuv, const vector<int32_t>& cscPara)
{
    for (size_t i = 0; i < 3; i++)
    {
        if (cscPara[i] != RGB_TO_YUV[k][0][i])
        {
            return false;
        }
    }
    size_t t = yuv;
    for (size_t i = 3; i < cscPara.size(); i++) {
        size_t j = i % 3;
        if (cscPara[i] != RGB_TO_YUV[k][t][j])
        {
            return false;
        }
        if (j == 2 && yuv == 1) {
             t++;
        }
        if (j == 2 && yuv == 2) {
             t--;
        }  
    }
    return true;
}

bool IsCscMatrixToYUVOrYVU(const vector<int>& cscPara, int keyIdx,
    ImageFormat& inputFormat,ImageFormat& targetFormat, ImageColorSpace& colorSpace)
{
    for (int k = 0;k < 4; k++) {
        for (int j = 0; j < 3; j++) {
            if (cscPara[j + 3] != RGB_TO_YUV[k][keyIdx][j])
            {
                break;
            }
            if (j == 2 && CheckRGB2YUVOrYVUEqual(k, keyIdx, cscPara))
            {
                inputFormat = ImageFormat::RGB888;
                targetFormat = keyIdx == 1 ? ImageFormat::YUV444SP : ImageFormat::YVU444SP;
                colorSpace = static_cast<ImageColorSpace>(k);
                return true;
            }
        }
    }
    return false;
}

bool CheckBiasEqual(const vector<int32_t>& bias, ImageFormat& targetFormat, ImageColorSpace& colorSpace)
{
    vector<int32_t> biasValues(3, 0);
    biasValues[0] = colorSpace == ImageColorSpace::JPEG ? 0 : 16;
    biasValues[1] = 128;
    biasValues[2] = 128;
    for (int32_t i = 0; i < 3; i++)
    {
        if (targetFormat == ImageFormat::RGB888 || targetFormat == ImageFormat::BGR888) {
            if (bias[i + 3] != biasValues[i])
            {
                return false;
            }
        } else {
            if (bias[i] != biasValues[i])
            {
                return false;
            }
        }  
    }
    return true;
}

aclError ConvertCscMatric2Csc(const vector<int32_t>& cscPara, ImageFormat& inputFormat,
    ImageFormat& targetFormat, ImageColorSpace& colorSpace)
{
    if (IsCscMatrixToGray(cscPara, inputFormat)) {
        targetFormat = ImageFormat::YUV400;
        colorSpace = ImageColorSpace::JPEG;
        return ACL_SUCCESS;
    }
    if (IsCscMatrixToRGBOrBGR(cscPara, 0, inputFormat, targetFormat, colorSpace)) {
        return ACL_SUCCESS;
    }
    if (IsCscMatrixToRGBOrBGR(cscPara, 2, inputFormat, targetFormat, colorSpace)) {
        return ACL_SUCCESS;
    }
    if (IsCscMatrixToYUVOrYVU(cscPara, 1, inputFormat, targetFormat, colorSpace)) {
        return ACL_SUCCESS;
    }
    if (IsCscMatrixToYUVOrYVU(cscPara, 2, inputFormat, targetFormat, colorSpace)) {
        return ACL_SUCCESS;
    }
    return ACL_ERROR_FAILURE;
}

std::map<aclAippInputFormat, HiAI_ImageFormat> aippMap = {
    {ACL_RGB888_U8, HIAI_RGB888_U8},
    {ACL_XRGB8888_U8, HIAI_XRGB8888_U8},
    {ACL_ARGB8888_U8, HIAI_ARGB8888_U8},
    {ACL_YUYV_U8, HIAI_YUYV_U8},
    {ACL_YUV400_U8, HIAI_YUV400_U8},
    {ACL_AYUV444_U8, HIAI_AYUV444_U8},
    {ACL_YUV420SP_U8, HIAI_YUV420SP_U8},
    {ACL_YUV422SP_U8, HIAI_YUV422SP_U8},
};

struct ChannelSwapParaSave 
{
    HiAI_ImageFormat imageFormat = HIAI_IMAGE_FORMAT_INVALID;
    bool rbuvSwapSwitch = false;
    bool axSwapSwitch = false;
};

ChannelSwapParaSave channelSwapPara;

constexpr int8_t AIPP_SWITCH_ON = 1;
constexpr int8_t AIPP_SWITCH_OFF = 0;
constexpr int16_t CSC_MATRIX_MIN = -32677;
constexpr int16_t CSC_MATRIX_MAX = 32676;
constexpr int32_t IMAGE_SIZE_MIN = 1;
constexpr int32_t IMAGE_SIZE_MAX = 4096;
constexpr int32_t SCF_SIZE_MAX = 16;
constexpr int32_t SCF_SIZEW_MAX = 1920;
constexpr int32_t SCF_OUTPUTW_MAX = 1080;
constexpr int32_t PADDING_MIN = 0;
constexpr int32_t PADDING_MAX = 32;
constexpr int16_t MEAN_CHN_MIN = 0;
constexpr int16_t MEAN_CHN_MAX = 255;
constexpr float32_t MIN_CHN_MIN = 0.0F;
constexpr float32_t MIN_CHN_MAX = 255.0F;
constexpr float32_t VR_CHN_MIN = -65504.0F;
constexpr float32_t VR_CHN_MAX = 65504.0F;

aclmdlAIPP* aclmdlCreateAIPP(uint64_t batchSize)
{
    auto hiaiAipp = HIAI_NDK_HiAIAippParam_Create(batchSize);
    aclmdlAIPP* aippParam = reinterpret_cast<aclmdlAIPP*>(hiaiAipp);
    return aippParam;
}

aclError aclmdlSetAIPPCscParams(aclmdlAIPP *aippParmsSet, int8_t cscSwitch,
    int16_t cscMatrixROCO, int16_t cscMatrixROC1, int16_t cscMatrixR0C2,
    int16_t cscMatrixR1CO, int16_t cscMatrixR1C1, int16_t cscMatrixR1C2,
    int16_t cscMatrixR2C0, int16_t cscMatrixR2C1, int16_t cscMatrixR2C2,
    uint8_t cscOutputBiasRO, uint8_t cscOutputBiasR1, uint8_t cscOutputBiasR2,
    uint8_t cscinputBiasRO, uint8_t cscInputBiasR1, uint8_t cscInputBiasR2)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, "the aippParmsSet is nullptr");
    HiAI_AippParam* aippParam = const_cast<HiAI_AippParam*>(reinterpret_cast<const HiAI_AippParam*>(aippParmsSet));
    ImageFormat inputFormat = ImageFormat::INVALID;
    ImageFormat targetFormat = ImageFormat::INVALID;
    ImageColorSpace colorSpace = ImageColorSpace::INVALID;

    std::vector<int32_t> cscParaList = {
        cscMatrixROCO, cscMatrixROC1, cscMatrixR0C2,
        cscMatrixR1CO, cscMatrixR1C1, cscMatrixR1C2,
        cscMatrixR2C0, cscMatrixR2C1, cscMatrixR2C2};

    std::vector<int32_t> bias = {
        cscOutputBiasRO, cscOutputBiasR1, cscOutputBiasR2,
        cscinputBiasRO, cscInputBiasR1, cscInputBiasR2};

    if (ConvertCscMatric2Csc(cscParaList, inputFormat, targetFormat, colorSpace) != ACL_SUCCESS)
    {
        ACL_LOGE("ConvertCscMatric2Csc fail, please check the csc parameters");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (targetFormat == ImageFormat::RGB888 || targetFormat == ImageFormat::BGR888 ||
        targetFormat == ImageFormat::YUV444SP || targetFormat == ImageFormat::YVU444SP) {
        if (!CheckBiasEqual(bias, targetFormat, colorSpace))
        {
            ACL_LOGE("CheckBiasEqual failed");
            inputFormat = ImageFormat::INVALID;
            targetFormat = ImageFormat::INVALID;
            colorSpace = ImageColorSpace::INVALID;
            return ACL_ERROR_INVALID_PARAM;
        }   
    }

    aclError ret = HIAI_NDK_HiAIAippParam_SetCscConfig(aippParam, static_cast<HiAI_ImageFormat>(inputFormat),
        static_cast<HiAI_ImageFormat>(targetFormat), static_cast<HiAI_ImageColorSpace>(colorSpace));
    if (ret != ACL_SUCCESS) {
        return ACL_ERROR_INVALID_PARAM;
    }   
    return ACL_SUCCESS;
}
aclError aclmdlDestroyAIPP(const aclmdlAIPP *aippParmsSet)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, "the aippParmsSet to destroy is nullptr");
    HiAI_AippParam* aippParam = const_cast<HiAI_AippParam*>(reinterpret_cast<const HiAI_AippParam*>(aippParmsSet));
    HIAI_NDK_HiAIAippParam_Destroy(&aippParam);
    aippParmsSet = nullptr;
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPInputFormat(aclmdlAIPP *aippParmsSet, aclAippInputFormat inputFormat)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, "the aippParmsSet to setformat is nullptr");
    HiAI_ImageFormat hiaiInputFormat = HIAI_IMAGE_FORMAT_INVALID;
    auto it = aippMap.find(inputFormat);
    if (it != aippMap.end()) {
        hiaiInputFormat = it->second;
    } else {
        ACL_LOGE("unsupported input format");
        return ACL_ERROR_INVALID_PARAM;
    }
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    auto ret = HIAI_NDK_HiAIAippParam_SetInputFormat(aippParam, hiaiInputFormat);
    if (ret != ACL_SUCCESS) {
        return ACL_ERROR_INVALID_PARAM;
    }   
    return ACL_SUCCESS;
}

namespace {
aclError SetImageFormat(aclmdlAIPP *aippParmsSet)
{
    HiAI_ImageFormat hiaiInputFormat = HIAI_IMAGE_FORMAT_INVALID;
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    hiaiInputFormat = HIAI_NDK_HiAIAippParam_GetInputFormat(aippParam);
    if (channelSwapPara.imageFormat == HIAI_IMAGE_FORMAT_INVALID) {
        channelSwapPara.imageFormat = hiaiInputFormat;
    } else if (channelSwapPara.imageFormat != hiaiInputFormat) {
        ACL_LOGE("imageFormat in aipp is not equal to previous one");
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}
}

aclError aclmdlSetAIPPRbuvSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t rbuvSwapSwitch)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setRbuvSwapSwitch is nullptr");
    if (rbuvSwapSwitch != AIPP_SWITCH_OFF && rbuvSwapSwitch != AIPP_SWITCH_ON) {
        ACL_LOGE("invalid rbuvSwapSwitch value, only 0 and 1 are supported");
        return ACL_ERROR_INVALID_PARAM;
    }
    
    auto ret = SetImageFormat(aippParmsSet);
    ACL_EXPECT_TRUE(ret == ACL_SUCCESS);
    channelSwapPara.rbuvSwapSwitch = rbuvSwapSwitch;
    return ACL_SUCCESS;
}
aclError aclmdlSetAIPPAxSwapSwitch(aclmdlAIPP *aippParmsSet, int8_t axSwapSwitch)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setAxSwapSwitch is nullptr");
    if (axSwapSwitch != AIPP_SWITCH_OFF && axSwapSwitch != AIPP_SWITCH_ON) {
        ACL_LOGE("invalid axSwapSwitch value, only 0 and 1 are supported");
        return ACL_ERROR_INVALID_PARAM;
    }

    auto ret = SetImageFormat(aippParmsSet);
    ACL_EXPECT_TRUE(ret == ACL_SUCCESS);
    channelSwapPara.axSwapSwitch = axSwapSwitch;
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPSrcImageSize(aclmdlAIPP *aippParmsSet, int32_t srcimageSizeW, int32_t srcimageSizeH)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM,
        "the aippParmsSet to setsrcimageSize is nullptr");
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    auto retImageFormat = HIAI_NDK_HiAIAippParam_GetInputFormat(aippParam);
    if (srcimageSizeW < IMAGE_SIZE_MIN || srcimageSizeH < IMAGE_SIZE_MIN || srcimageSizeW > IMAGE_SIZE_MAX || srcimageSizeH > IMAGE_SIZE_MAX)
    {
        ACL_LOGE("invalid src image size, width and height should be in range [1, 4096]");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (retImageFormat == HIAI_YUV420SP_U8 || retImageFormat == HIAI_YUV422SP_U8 || retImageFormat == HIAI_YUYV_U8) {
        if (srcimageSizeW % 2 != 0 || srcimageSizeH % 2 != 0) {
            ACL_LOGE("invalid src image size for YUV420SP/YUV422SP/YUYV format, width and height should be even");
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    auto ret = HIAI_NDK_HiAIAippParam_SetInputShape(aippParam, srcimageSizeW, srcimageSizeH);
    if (ret != ACL_SUCCESS) {
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPScfParams(aclmdlAIPP *aippParmsSet, int8_t scfSwitch, int32_t scfInputSizeW, 
    int32_t scfInputSizeH, int32_t scfOutputSizeW, int32_t scfOutputSizeH, uint64_t batchIndex)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setScfParam is nullptr");
    ACL_EXPECT_TRUE_R(scfInputSizeW > 0, ACL_ERROR_INVALID_PARAM);
    ACL_EXPECT_TRUE_R(scfInputSizeH > 0, ACL_ERROR_INVALID_PARAM);
    ACL_EXPECT_TRUE_R(scfOutputSizeW > 0, ACL_ERROR_INVALID_PARAM);
    ACL_EXPECT_TRUE_R(scfOutputSizeH > 0, ACL_ERROR_INVALID_PARAM);

    if (scfSwitch != AIPP_SWITCH_OFF && scfSwitch != AIPP_SWITCH_ON) {
        ACL_LOGE("invalid scfSwitch value, only 0 and 1 are supported");
        return ACL_ERROR_INVALID_PARAM;
    }
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    uint32_t retSrcImageW = 0;
    uint32_t retSrcImageH = 0;
    auto ret = HIAI_NDK_HiAIAippParam_GetInputShape(aippParam, &retSrcImageW, &retSrcImageH);
    if (ret != ACL_SUCCESS) {
        return ACL_ERROR_INVALID_PARAM;
    }
    if (retSrcImageW != scfInputSizeW || retSrcImageH != scfInputSizeH) {
        ACL_LOGE("scf input size should be equal to source image size");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (scfInputSizeH / scfOutputSizeW > SCF_SIZE_MAX || scfInputSizeW / scfOutputSizeW > SCF_SIZE_MAX ||
        scfOutputSizeW / scfInputSizeH > SCF_SIZE_MAX || scfOutputSizeW / scfInputSizeW > SCF_SIZE_MAX) {
        ACL_LOGE("resize factor is too larger");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (scfInputSizeW > SCF_SIZEW_MAX || scfOutputSizeW > SCF_OUTPUTW_MAX)
    {
        ACL_LOGE("scf input size width or output size width is too larger");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (scfSwitch == AIPP_SWITCH_ON) {
        ret = HIAI_NDK_HiAIAippParam_SetResizeConfig(aippParam, batchIndex, scfOutputSizeW, scfOutputSizeH);
        if (ret != ACL_SUCCESS) {
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPCropParams(aclmdlAIPP *aippParmsSet, int8_t cropSwitch, int32_t cropStartPosW, 
    int32_t cropStartPosH, int32_t cropSizeW, int32_t cropSizeH, uint64_t batchIndex)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setCropParams is nullptr");
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    if (cropSwitch != AIPP_SWITCH_OFF && cropSwitch != AIPP_SWITCH_ON) {
        ACL_LOGE("invalid cropSwitch value, only 0 and 1 are supported");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (cropSwitch == AIPP_SWITCH_ON) {
        auto retImageFormat = HIAI_NDK_HiAIAippParam_GetInputFormat(aippParam);
        if (retImageFormat == HIAI_YUV420SP_U8) {
            if (cropStartPosW % 2 != 0 || cropStartPosH % 2 != 0) {
                ACL_LOGE("invalid crop position or size for YUV420SP format, start position and size should be even");
                return ACL_ERROR_INVALID_PARAM;
            }
        }
        if (cropStartPosW < IMAGE_SIZE_MIN || cropStartPosH < IMAGE_SIZE_MIN || cropStartPosW > IMAGE_SIZE_MAX || cropStartPosH > IMAGE_SIZE_MAX) {
             ACL_LOGE("invalid cropStartPosW or cropStartPosH, too larger or too smaller");
             return ACL_ERROR_INVALID_PARAM;
        }
        if (cropSizeW < IMAGE_SIZE_MIN || cropSizeH < IMAGE_SIZE_MIN || cropSizeW > IMAGE_SIZE_MAX || cropSizeH > IMAGE_SIZE_MAX) {
             ACL_LOGE("invalid cropSizeW or cropSizeH, too larger or too smaller");
             return ACL_ERROR_INVALID_PARAM;
        }
        auto ret = HIAI_NDK_HiAIAippParam_SetCropConfig(aippParam, batchIndex, cropStartPosW, cropStartPosH, cropSizeW, cropSizeH);
        if (ret != ACL_SUCCESS) {
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    return ACL_SUCCESS;
}


aclError aclmdlSetAIPPPaddingParams(aclmdlAIPP *aippParmsSet, int8_t paddingSwitch, int32_t paddingSizeTop, 
    int32_t paddingSizeBottom, int32_t paddingSizeLeft, int32_t paddingSizeRight, uint64_t batchIndex)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setPaddingParams is nullptr");
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    if (paddingSwitch != AIPP_SWITCH_OFF && paddingSwitch != AIPP_SWITCH_ON) {
        ACL_LOGE("invalid paddingSwitch value, only 0 and 1 are supported");
        return ACL_ERROR_INVALID_PARAM;
    }
    if (paddingSwitch == AIPP_SWITCH_ON) {
        if (paddingSizeTop < PADDING_MIN || paddingSizeBottom < PADDING_MIN || paddingSizeLeft < PADDING_MIN || paddingSizeRight < PADDING_MIN ||
            paddingSizeTop > PADDING_MAX || paddingSizeBottom > PADDING_MAX || paddingSizeLeft > PADDING_MAX || paddingSizeRight > PADDING_MAX) {
            ACL_LOGE("invalid paddingSizeTop, paddingSizeBottom, paddingSizeLeft or paddingSizeRight, too larger or too smaller");
            return ACL_ERROR_INVALID_PARAM;
        }
        auto ret = HIAI_NDK_HiAIAippParam_SetPadConfig(aippParam, batchIndex, paddingSizeLeft, paddingSizeRight, paddingSizeTop, paddingSizeBottom);
        if (ret != ACL_SUCCESS) {
            return ACL_ERROR_INVALID_PARAM;
        }
    }
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPDtcPixelMean(aclmdlAIPP *aippParmsSet, int16_t dtcPixelMeanChn0, int16_t dtcPixelMeanChn1, 
    int16_t dtcPixelMeanChn2, int16_t dtcPixelMeanChn3, uint64_t batchIndex)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setDtcPixelMean is nullptr");
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    if (dtcPixelMeanChn0 < MEAN_CHN_MIN || dtcPixelMeanChn1 < MEAN_CHN_MIN || dtcPixelMeanChn2 < MEAN_CHN_MIN || dtcPixelMeanChn3 < MEAN_CHN_MIN ||
        dtcPixelMeanChn0 > MEAN_CHN_MAX || dtcPixelMeanChn1 > MEAN_CHN_MAX || dtcPixelMeanChn2 > MEAN_CHN_MAX || dtcPixelMeanChn3 > MEAN_CHN_MAX) {
        ACL_LOGE("invalid dtcPixelMeanChn0, dtcPixelMeanChn1, dtcPixelMeanChn2 or dtcPixelMeanChn3 value, should be in range [0, 255]");
        return ACL_ERROR_INVALID_PARAM;
    }
    std::vector<uint32_t> dtcPixelMeanChn;
    dtcPixelMeanChn.push_back(static_cast<uint32_t>(dtcPixelMeanChn0));
    dtcPixelMeanChn.push_back(static_cast<uint32_t>(dtcPixelMeanChn1));
    dtcPixelMeanChn.push_back(static_cast<uint32_t>(dtcPixelMeanChn2));
    dtcPixelMeanChn.push_back(static_cast<uint32_t>(dtcPixelMeanChn3));

    auto ret = HIAI_NDK_HiAIAippParam_SetDtcMeanPixel(aippParam, batchIndex, dtcPixelMeanChn.data(), dtcPixelMeanChn.size());
    if (ret != ACL_SUCCESS) {
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}
aclError aclmdlSetAIPPDtcPixelMin(aclmdlAIPP *aippParmsSet, float dtcPixelMinChn0, float dtcPixelMinChn1, 
    float dtcPixelMinChn2, float dtcPixelMinChn3, uint64_t batchIndex)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setDtcPixelMin is nullptr");
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    if (dtcPixelMinChn0 < MIN_CHN_MIN || dtcPixelMinChn1 < MIN_CHN_MIN || dtcPixelMinChn2 < MIN_CHN_MIN || dtcPixelMinChn3 < MIN_CHN_MIN ||
        dtcPixelMinChn0 > MIN_CHN_MAX || dtcPixelMinChn1 > MIN_CHN_MAX || dtcPixelMinChn2 > MIN_CHN_MAX || dtcPixelMinChn3 > MIN_CHN_MAX) {
        ACL_LOGE("invalid dtcPixelMinChn0, dtcPixelMinChn1, dtcPixelMinChn2 or dtcPixelMinChn3 value, should be in range [0.0, 255.0]");
        return ACL_ERROR_INVALID_PARAM;
    }
    std::vector<float> dtcPixelMinChn;
    dtcPixelMinChn.push_back(dtcPixelMinChn0);
    dtcPixelMinChn.push_back(dtcPixelMinChn1);
    dtcPixelMinChn.push_back(dtcPixelMinChn2);
    dtcPixelMinChn.push_back(dtcPixelMinChn3);

    auto ret = HIAI_NDK_HiAIAippParam_SetDtcMinPixel(aippParam, batchIndex, dtcPixelMinChn.data(), dtcPixelMinChn.size());
    if (ret != ACL_SUCCESS) {
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}
aclError aclmdlSetAIPPPixelVarReci(aclmdlAIPP *aippParmsSet, float dtcPixelVarReciChn0, float dtcPixelVarReciChn1, 
    float dtcPixelVarReciChn2, float dtcPixelVarReciChn3, uint64_t batchIndex)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(aippParmsSet != nullptr, ACL_ERROR_INVALID_PARAM, 
        "the aippParmsSet to setDtcPixelVarRec is nullptr");
    HiAI_AippParam* aippParam = reinterpret_cast<HiAI_AippParam*>(aippParmsSet);
    if (dtcPixelVarReciChn0 < VR_CHN_MIN || dtcPixelVarReciChn1 < VR_CHN_MIN || dtcPixelVarReciChn2 < VR_CHN_MIN || dtcPixelVarReciChn3 < VR_CHN_MIN ||
        dtcPixelVarReciChn0 > VR_CHN_MAX || dtcPixelVarReciChn1 > VR_CHN_MAX || dtcPixelVarReciChn2 > VR_CHN_MAX || dtcPixelVarReciChn3 > VR_CHN_MAX) {
        ACL_LOGE("invalid dtcPixelVarReciChn0, dtcPixelVarReciChn1, dtcPixelVarReciChn2 or dtcPixelVarReciChn3 value, should be in range [-65504.0, 65504.0]");
        return ACL_ERROR_INVALID_PARAM;
    }
    auto reciprocal = [](float x) -> float {
        if (x == 0.0) {
            ACL_LOGE("dtcPixelVarReciChn value should not be zero to avoid division by zero");
        }
        return 1.0 / x;
    };
    std::vector<float> varReciPixels;
    std::vector<float> dtcPixelVarReciChn;
    dtcPixelVarReciChn.push_back(dtcPixelVarReciChn0);
    dtcPixelVarReciChn.push_back(dtcPixelVarReciChn1);
    dtcPixelVarReciChn.push_back(dtcPixelVarReciChn2);
    dtcPixelVarReciChn.push_back(dtcPixelVarReciChn3);
    float varReciPixel = 0.0;
    for (size_t i = 0; i < dtcPixelVarReciChn.size(); i++) {
        float varReciPixel = reciprocal(dtcPixelVarReciChn[i]);
        varReciPixels.push_back(varReciPixel);
    }

    auto ret = HIAI_NDK_HiAIAippParam_SetDtcVarReciPixel(aippParam, batchIndex, varReciPixels.data(), varReciPixels.size());
    if (ret != ACL_SUCCESS) {
        ACL_LOGE("HIAI_NDK_HiAIAippParam_SetDtcVarReciPixel failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    return ACL_SUCCESS;
}
aclError aclmdlGetAippDataSize(uint64_t batchSize, size_t *size)
{
    HiAI_AippParam* aippParam = HIAI_NDK_HiAIAippParam_Create(static_cast<uint32_t>(batchSize));
    uint32_t aippSize = HIAI_NDK_HiAIAippParam_GetDataSize(aippParam);
    ACL_EXPECT_TRUE_R_WITH_LOG(aippSize != 0, ACL_ERROR_INVALID_PARAM, "the aipp size is 0");
    HIAI_NDK_HiAIAippParam_Destroy(&aippParam);
    aippParam = nullptr;
    *size = aippSize;
    return ACL_SUCCESS;
}

aclError aclmdlSetAIPPByInputIndex(uint32_t modelId, aclmdlDataset *dataset, size_t index, const aclmdlAIPP *aippParmSet)
{
    (void)modelId;
    ACL_EXPECT_TRUE_R_WITH_LOG(dataset != nullptr, ACL_ERROR_INVALID_PARAM, "the dataset is nullptr");
    HiAI_AippParam* aippParm = const_cast<HiAI_AippParam*>(reinterpret_cast<const HiAI_AippParam*>(aippParmSet));
    aclError ret = ACL_ERROR_FAILURE;
    if (channelSwapPara.axSwapSwitch || channelSwapPara.rbuvSwapSwitch) {
        ret = HIAI_NDK_HiAIAippParam_SetChannelSwapConfig(
            aippParm, channelSwapPara.rbuvSwapSwitch, channelSwapPara.axSwapSwitch);
            if (ret != ACL_SUCCESS) {
                ACL_LOGE("HIAI_NDK_HiAIAippParam_SetChannelSwapConfig failed");
                return ACL_ERROR_INVALID_PARAM;
            }
    }
    
    ret = HIAI_NDK_HiAIAippParam_SetInputIndex(aippParm, index);
    if (ret != ACL_SUCCESS) {
        ACL_LOGE("HIAI_NDK_HiAIAippParam_SetInputIndex failed");
        return ACL_ERROR_INVALID_PARAM;
    }
    auto aippParmPtr = std::shared_ptr<HiAI_AippParam>(aippParm,
        [](HiAI_AippParam* p) { HIAI_NDK_HiAIAippParam_Destroy(&p); });
    dataset->aippParas.push_back(std::shared_ptr<HiAI_AippParam>(aippParmPtr));
    std::sort(dataset->aippParas.begin(), dataset->aippParas.end(),
        [](std::shared_ptr<HiAI_AippParam> a, std::shared_ptr<HiAI_AippParam> b) {
            return HIAI_NDK_HiAIAippParam_GetInputIndex(a.get()) <
                HIAI_NDK_HiAIAippParam_GetInputIndex(b.get());
    });
    return ACL_SUCCESS;
}