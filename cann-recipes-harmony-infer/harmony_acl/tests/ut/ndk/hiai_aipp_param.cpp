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

#include "hiai_foundation/hiai_aipp_param.h"
#include "hiai_ndk_struct_def.h"

#include "utils/assert.h"

HiAI_AippParam* HMS_HiAIAippParam_Create(uint32_t batchNum)
{
    ACL_EXPECT_TRUE_R(batchNum > 0 && batchNum <= 127, nullptr);

    AippParaStub* aippParam = new (std::nothrow) AippParaStub();
    ACL_EXPECT_NOT_NULL_R(aippParam, nullptr);
    aippParam->batchNum = batchNum;
    aippParam->cropConfig.resize(batchNum);
    aippParam->resizeConfig.resize(batchNum);
    aippParam->padConfig.resize(batchNum);
    aippParam->paddingValues.resize(batchNum);
    aippParam->rotationAngle.resize(batchNum);
    aippParam->dtcMeanPixel.resize(batchNum);
    aippParam->dtcMinPixel.resize(batchNum);
    aippParam->dtcVarReciPixel.resize(batchNum);
    return reinterpret_cast<HiAI_AippParam*>(aippParam);
}

void* HMS_HiAIAippParam_GetData(HiAI_AippParam* aippParam)
{
    return static_cast<void*>(aippParam);
}

uint32_t HMS_HiAIAippParam_GetDataSize(HiAI_AippParam* aippParam)
{
    return aippParam == nullptr ? 0 : sizeof(AippParaStub);
}

int HMS_HiAIAippParam_GetInputIndex(HiAI_AippParam* aippParam)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, -1);
    return GetType<AippParaStub>(aippParam)->inputIndex;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetInputIndex(HiAI_AippParam* aippParam, uint32_t inputIndex)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->inputIndex = inputIndex;
    return OH_NN_SUCCESS;
}

int HMS_HiAIAippParam_GetInputAippIndex(HiAI_AippParam* aippParam)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, -1);
    return GetType<AippParaStub>(aippParam)->inputIndex;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetInputAippIndex(HiAI_AippParam* aippParam, uint32_t inputAippIndex)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->inputAippIndex = inputAippIndex;
    return OH_NN_SUCCESS;
}

void HMS_HiAIAippParam_Destroy(HiAI_AippParam** aippParam)
{
    if (aippParam != nullptr && *aippParam != nullptr) {
        AippParaStub* aipp = reinterpret_cast<AippParaStub*>(*aippParam);
        delete aipp;
        *aippParam = nullptr;
    }
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetInputFormat(HiAI_AippParam* aippParam, HiAI_ImageFormat inputFormat)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->inputFormat = inputFormat;
    return OH_NN_SUCCESS;
}

HiAI_ImageFormat HMS_HiAIAippParam_GetInputFormat(HiAI_AippParam* aippParam)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, HIAI_IMAGE_FORMAT_INVALID);
    return GetType<AippParaStub>(aippParam)->inputFormat;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetInputShape(HiAI_AippParam* aippParam, uint32_t srcImageW, uint32_t srcImageH)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->srcImageW = srcImageW;
    GetType<AippParaStub>(aippParam)->srcImageH = srcImageH;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetInputShape(HiAI_AippParam* aippParam, uint32_t* srcImageW, uint32_t* srcImageH)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    *srcImageW = GetType<AippParaStub>(aippParam)->srcImageW;
    *srcImageH = GetType<AippParaStub>(aippParam)->srcImageH;
    return OH_NN_SUCCESS;
}

uint32_t HMS_HiAIAippParam_GetBatchCount(HiAI_AippParam* aippParam)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, 0);
    return GetType<AippParaStub>(aippParam)->batchNum; 
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetCscConfig(HiAI_AippParam* aippParam,
    HiAI_ImageFormat inputFormat, HiAI_ImageFormat outputFormat, HiAI_ImageColorSpace space)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && inputFormat >= HIAI_YUV420SP_U8 && inputFormat <= HIAI_BGR888_U8 &&
        space >= HIAI_JPEG && space < HIAI_IMAGE_COLOR_SPACE_INVALID, OH_NN_INVALID_PARAMETER);

    GetType<AippParaStub>(aippParam)->cscInputFormat = inputFormat;
    GetType<AippParaStub>(aippParam)->cscOutputFormat = outputFormat;
    GetType<AippParaStub>(aippParam)->space = space;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetCscConfig(HiAI_AippParam* aippParam,
    HiAI_ImageFormat* inputFormat, HiAI_ImageFormat* outputFormat, HiAI_ImageColorSpace* space)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && inputFormat != nullptr && outputFormat != nullptr && space != nullptr, OH_NN_INVALID_PARAMETER);
    *inputFormat = GetType<AippParaStub>(aippParam)->cscInputFormat;
    *outputFormat = GetType<AippParaStub>(aippParam)->cscOutputFormat;
    *space = GetType<AippParaStub>(aippParam)->space;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetChannelSwapConfig(HiAI_AippParam* aippParam,
    bool rbuvSwapSwitch, bool axSwapSwitch)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);

    GetType<AippParaStub>(aippParam)->rbuvSwapSwitch = rbuvSwapSwitch;
    GetType<AippParaStub>(aippParam)->axSwapSwitch = axSwapSwitch;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetChannelSwapConfig(HiAI_AippParam* aippParam,
    bool* rbuvSwapSwitch, bool* axSwapSwitch)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && rbuvSwapSwitch != nullptr && axSwapSwitch != nullptr, OH_NN_INVALID_PARAMETER);

    *rbuvSwapSwitch = GetType<AippParaStub>(aippParam)->rbuvSwapSwitch;
    *axSwapSwitch = GetType<AippParaStub>(aippParam)->axSwapSwitch;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetSingleBatchMultiCrop(HiAI_AippParam* aippParam, bool singleBatchMultiCrop)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->singleBatchMultiCrop = singleBatchMultiCrop;
    return OH_NN_SUCCESS;
}

bool HMS_HiAIAippParam_GetSingleBatchMultiCrop(HiAI_AippParam* aippParam)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, false);
    return GetType<AippParaStub>(aippParam)->singleBatchMultiCrop;
}


OH_NN_ReturnCode HMS_HiAIAippParam_SetCropConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t startPosW, uint32_t startPosH,
    uint32_t croppedW, uint32_t croppedH)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropStartPosW = startPosW;
    GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropStartPosH = startPosH;
    GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropSizeW = croppedW;
    GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropSizeH = croppedH;
    GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropSwitch = true;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetCropConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t* startPosW, uint32_t* startPosH,
    uint32_t* croppedW, uint32_t* croppedH)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && startPosW != nullptr && startPosH != nullptr &&
        croppedW != nullptr && croppedH != nullptr, OH_NN_INVALID_PARAMETER);
    *startPosW = GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropStartPosW;
    *startPosH = GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropStartPosH;
    *croppedW = GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropSizeW;
    *croppedH = GetType<AippParaStub>(aippParam)->cropConfig[batchIndex].cropSizeH;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetResizeConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t resizeW, uint32_t resizeH)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && resizeW >= 16 && resizeW <= 448 &&
        resizeH >= 16 && resizeH <= 4096, OH_NN_INVALID_PARAMETER);

    GetType<AippParaStub>(aippParam)->resizeConfig[batchIndex].resizeW = resizeW;
    GetType<AippParaStub>(aippParam)->resizeConfig[batchIndex].resizeH = resizeH;
    GetType<AippParaStub>(aippParam)->resizeConfig[batchIndex].resizeSwitch = true;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetResizeConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t* resizedW, uint32_t* resizedH)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && resizedW != nullptr && resizedH != nullptr, OH_NN_INVALID_PARAMETER);

    *resizedW = GetType<AippParaStub>(aippParam)->resizeConfig[batchIndex].resizeW;
    *resizedH = GetType<AippParaStub>(aippParam)->resizeConfig[batchIndex].resizeH;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetPadConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t leftPadSize, uint32_t rightPadSize,
    uint32_t topPadSize, uint32_t bottomPadSize)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->padConfig[batchIndex].left = leftPadSize;
    GetType<AippParaStub>(aippParam)->padConfig[batchIndex].right = rightPadSize;
    GetType<AippParaStub>(aippParam)->padConfig[batchIndex].top = topPadSize;
    GetType<AippParaStub>(aippParam)->padConfig[batchIndex].bottom = bottomPadSize;
    GetType<AippParaStub>(aippParam)->padConfig[batchIndex].padSwitch = true;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetPadConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t* leftPadSize, uint32_t* rightPadSize,
    uint32_t* topPadSize, uint32_t* bottomPadSize)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && leftPadSize != nullptr && rightPadSize != nullptr &&
        topPadSize != nullptr && bottomPadSize != nullptr, OH_NN_INVALID_PARAMETER);
    *leftPadSize = GetType<AippParaStub>(aippParam)->padConfig[batchIndex].left;
    *rightPadSize = GetType<AippParaStub>(aippParam)->padConfig[batchIndex].right;
    *topPadSize = GetType<AippParaStub>(aippParam)->padConfig[batchIndex].top;
    *bottomPadSize = GetType<AippParaStub>(aippParam)->padConfig[batchIndex].bottom;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetChannelPadding(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t paddingValues[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        paddingValues != nullptr && channelCount >= 1 && channelCount <=4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->paddingValues[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        GetType<AippParaStub>(aippParam)->paddingValues[batchIndex][i] = paddingValues[i];
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetChannelPadding(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t paddingValues[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        paddingValues != nullptr && channelCount >= 1 && channelCount <=4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->paddingValues[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        paddingValues[i] = GetType<AippParaStub>(aippParam)->paddingValues[batchIndex][i];
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetRotationAngle(HiAI_AippParam* aippParam, uint32_t batchIndex, float rotationAngle)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        rotationAngle >= 0 && rotationAngle <= 360, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->rotationAngle[batchIndex] = rotationAngle;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetRotationAngle(HiAI_AippParam* aippParam, uint32_t batchIndex, float* rotationAngle)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr && rotationAngle != nullptr, OH_NN_INVALID_PARAMETER);
    *rotationAngle = GetType<AippParaStub>(aippParam)->rotationAngle[batchIndex] ;
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetDtcMeanPixel(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t meanPixel[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        meanPixel != nullptr && channelCount >= 1 && channelCount <= 4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->dtcMeanPixel[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        GetType<AippParaStub>(aippParam)->dtcMeanPixel[batchIndex][i] = meanPixel[i];
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetDtcMeanPixel(HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t meanPixel[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        meanPixel != nullptr && channelCount >= 1 && channelCount <= 4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->dtcMeanPixel[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        meanPixel[i] = GetType<AippParaStub>(aippParam)->dtcMeanPixel[batchIndex][i];
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetDtcMinPixel(HiAI_AippParam* aippParam, uint32_t batchIndex, float minPixel[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        minPixel != nullptr && channelCount >= 1 && channelCount <= 4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->dtcMinPixel[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        GetType<AippParaStub>(aippParam)->dtcMinPixel[batchIndex][i] = minPixel[i];
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetDtcMinPixel(HiAI_AippParam* aippParam, uint32_t batchIndex, float minPixel[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        minPixel != nullptr && channelCount >= 1 && channelCount <= 4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->dtcMinPixel[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        minPixel[i] = GetType<AippParaStub>(aippParam)->dtcMinPixel[batchIndex][i];
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_SetDtcVarReciPixel(HiAI_AippParam* aippParam, uint32_t batchIndex, float varReciPixel[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        varReciPixel != nullptr && channelCount >= 1 && channelCount <= 4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->dtcVarReciPixel[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        GetType<AippParaStub>(aippParam)->dtcVarReciPixel[batchIndex][i] = varReciPixel[i];
    }
    return OH_NN_SUCCESS;
}

OH_NN_ReturnCode HMS_HiAIAippParam_GetDtcVarReciPixel(HiAI_AippParam* aippParam, uint32_t batchIndex, float varReciPixel[], uint32_t channelCount)
{
    ACL_EXPECT_TRUE_R(aippParam != nullptr &&
        varReciPixel != nullptr && channelCount >= 1 && channelCount <= 4, OH_NN_INVALID_PARAMETER);
    GetType<AippParaStub>(aippParam)->dtcVarReciPixel[batchIndex].resize(channelCount);
    for (uint32_t i = 0; i < channelCount; ++i) {
        varReciPixel[i] = GetType<AippParaStub>(aippParam)->dtcVarReciPixel[batchIndex][i];
    }
    return OH_NN_SUCCESS;
}


