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
 
#include "hiai_ndk_aipp.h"

#include "ndk_proxy.h"
#include "utils/assert.h"

namespace hiai {

HiAI_AippParam* HIAI_NDK_HiAIAippParam_Create(uint32_t batchNum)
{
    auto createAippParamFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_Create)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_Create"));
    ACL_EXPECT_NOT_NULL_R(createAippParamFunc, nullptr);

    return createAippParamFunc(batchNum);
}

void HIAI_NDK_HiAIAippParam_Destroy(HiAI_AippParam** aippParam)
{
    auto destroyAippParamFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_Destroy)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_Destroy"));
    ACL_EXPECT_NOT_NULL_VOID(destroyAippParamFunc);

    destroyAippParamFunc(aippParam);
    aippParam = nullptr;
}

void* HIAI_NDK_HiAIAippParam_GetData(HiAI_AippParam* aippParam)
{
    auto getRawBufferFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetData)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetData"));
    ACL_EXPECT_NOT_NULL_R(getRawBufferFunc, nullptr);

    return getRawBufferFunc(aippParam);
}

uint32_t HIAI_NDK_HiAIAippParam_GetDataSize(HiAI_AippParam* aippParam)
{
    auto getRawBufferSizeFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetDataSize)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetDataSize"));
    ACL_EXPECT_NOT_NULL_R(getRawBufferSizeFunc, 0);

    return getRawBufferSizeFunc(aippParam);
}

uint32_t HIAI_NDK_HiAIAippParam_GetBatchCount(HiAI_AippParam* aippParam)
{
    auto getBatchCountFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetBatchCount)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetBatchCount"));
    ACL_EXPECT_NOT_NULL_R(getBatchCountFunc, 0);

    return getBatchCountFunc(aippParam);
}

aclError HIAI_NDK_HiAIAippParam_SetInputIndex(HiAI_AippParam* aippParam, uint32_t inputIndex)
{
    auto setInputIndexFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetInputIndex)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetInputIndex"));
    ACL_EXPECT_NOT_NULL(setInputIndexFunc);

    OH_NN_ReturnCode retCode = setInputIndexFunc(aippParam, inputIndex);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

int HIAI_NDK_HiAIAippParam_GetInputIndex(HiAI_AippParam* aippParam)
{
    auto getInputIndexFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetInputIndex)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetInputIndex"));
    ACL_EXPECT_NOT_NULL_R(getInputIndexFunc, -1);

    return getInputIndexFunc(aippParam);
}

aclError HIAI_NDK_HiAIAippParam_SetInputAippIndex(HiAI_AippParam* aippParam, uint32_t inputAippIndex)
{
    auto setInputAippIndexFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetInputAippIndex)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetInputAippIndex"));
    ACL_EXPECT_NOT_NULL(setInputAippIndexFunc);

    OH_NN_ReturnCode retCode = setInputAippIndexFunc(aippParam, inputAippIndex);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

int HIAI_NDK_HiAIAippParam_GetInputAippIndex(HiAI_AippParam* aippParam)
{
    auto getInputAippIndexFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetInputAippIndex)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetInputAippIndex"));
    ACL_EXPECT_NOT_NULL_R(getInputAippIndexFunc, -1);

    return getInputAippIndexFunc(aippParam);
}

aclError HIAI_NDK_HiAIAippParam_SetCscConfig(HiAI_AippParam* aippParam, 
    HiAI_ImageFormat inputFormat, HiAI_ImageFormat outputFormat, HiAI_ImageColorSpace space)
{
    auto setCscConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetCscConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetCscConfig"));
    ACL_EXPECT_NOT_NULL(setCscConfigFunc);

    OH_NN_ReturnCode retCode = setCscConfigFunc(aippParam, inputFormat, outputFormat, space);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetCscConfig(HiAI_AippParam* aippParam, 
    HiAI_ImageFormat* inputFormat, HiAI_ImageFormat* outputFormat, HiAI_ImageColorSpace* space)
{
    auto getCscConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetCscConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetCscConfig"));
    ACL_EXPECT_NOT_NULL(getCscConfigFunc);

    OH_NN_ReturnCode retCode = getCscConfigFunc(aippParam, inputFormat, outputFormat, space);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetChannelSwapConfig(HiAI_AippParam* aippParam, 
    bool rbuvSwapSwitch, bool axSwapSwitch)
{
    auto setChannelSwapConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetChannelSwapConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetChannelSwapConfig"));
    ACL_EXPECT_NOT_NULL(setChannelSwapConfigFunc);

    OH_NN_ReturnCode retCode = setChannelSwapConfigFunc(aippParam, rbuvSwapSwitch, axSwapSwitch);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetChannelSwapConfig(HiAI_AippParam* aippParam, 
    bool* rbuvSwapSwitch, bool* axSwapSwitch)
{
    auto getChannelSwapConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetChannelSwapConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetChannelSwapConfig"));
    ACL_EXPECT_NOT_NULL(getChannelSwapConfigFunc);

    OH_NN_ReturnCode retCode = getChannelSwapConfigFunc(aippParam, rbuvSwapSwitch, axSwapSwitch);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetSingleBatchMultiCrop(HiAI_AippParam* aippParam, 
    bool singleBatchMultiCrop)
{
    auto setSingleBatchMultiCropFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetSingleBatchMultiCrop)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetSingleBatchMultiCrop"));
    ACL_EXPECT_NOT_NULL(setSingleBatchMultiCropFunc);

    OH_NN_ReturnCode retCode = setSingleBatchMultiCropFunc(aippParam, singleBatchMultiCrop);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

bool HIAI_NDK_HiAIAippParam_GetSingleBatchMultiCrop(HiAI_AippParam* aippParam)
{
    auto getSingleBatchMultiCropFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetSingleBatchMultiCrop)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetSingleBatchMultiCrop"));
    ACL_EXPECT_NOT_NULL_R(getSingleBatchMultiCropFunc, false);

    return getSingleBatchMultiCropFunc(aippParam);
}

aclError HIAI_NDK_HiAIAippParam_SetCropConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t startPosW, uint32_t startPosH, uint32_t croppedW, uint32_t croppedH)
{
    auto setCropConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetCropConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetCropConfig"));
    ACL_EXPECT_NOT_NULL(setCropConfigFunc);

    OH_NN_ReturnCode retCode = setCropConfigFunc(aippParam, batchIndex, startPosW, startPosH, croppedW, croppedH);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetCropConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t* startPosW, uint32_t* startPosH, uint32_t* croppedW, uint32_t* croppedH)
{
    auto getCropConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetCropConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetCropConfig"));
    ACL_EXPECT_NOT_NULL(getCropConfigFunc);

    OH_NN_ReturnCode retCode = getCropConfigFunc(aippParam, batchIndex, startPosW, startPosH, croppedW, croppedH);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetResizeConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t resizedW, uint32_t resizedH)
{
    auto setResizeConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetResizeConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetResizeConfig"));
    ACL_EXPECT_NOT_NULL(setResizeConfigFunc);

    OH_NN_ReturnCode retCode = setResizeConfigFunc(aippParam, batchIndex, resizedW, resizedH);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetResizeConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t* resizedW, uint32_t* resizedH)
{
    auto getResizeConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetResizeConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetResizeConfig"));
    ACL_EXPECT_NOT_NULL(getResizeConfigFunc);

    OH_NN_ReturnCode retCode = getResizeConfigFunc(aippParam, batchIndex, resizedW, resizedH);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetPadConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t leftPadSize, uint32_t rightPadSize, uint32_t topPadSize, uint32_t bottomPadSize)
{
    auto setPadConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetPadConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetPadConfig"));
    ACL_EXPECT_NOT_NULL(setPadConfigFunc);

    OH_NN_ReturnCode retCode = setPadConfigFunc(aippParam, batchIndex, leftPadSize, rightPadSize, topPadSize, bottomPadSize);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetPadConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t* leftPadSize, uint32_t* rightPadSize, uint32_t* topPadSize, uint32_t* bottomPadSize)
{
    auto getPadConfigFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetPadConfig)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetPadConfig"));
    ACL_EXPECT_NOT_NULL(getPadConfigFunc);

    OH_NN_ReturnCode retCode = getPadConfigFunc(aippParam, batchIndex, leftPadSize, rightPadSize, topPadSize, bottomPadSize);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetChannelPadding(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t paddingValues[], uint32_t channelCount)
{
    auto setChannelPaddingFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetChannelPadding)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetChannelPadding"));
    ACL_EXPECT_NOT_NULL(setChannelPaddingFunc);

    OH_NN_ReturnCode retCode = setChannelPaddingFunc(aippParam, batchIndex, paddingValues, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetChannelPadding(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t paddingValues[], uint32_t channelCount)
{
    auto getChannelPaddingFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetChannelPadding)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetChannelPadding"));
    ACL_EXPECT_NOT_NULL(getChannelPaddingFunc);

    OH_NN_ReturnCode retCode = getChannelPaddingFunc(aippParam, batchIndex, paddingValues, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetDtcMeanPixel(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t meanPixel[], uint32_t channelCount)
{
    auto setDtcMeanPixelFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetDtcMeanPixel)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetDtcMeanPixel"));
    ACL_EXPECT_NOT_NULL(setDtcMeanPixelFunc);

    OH_NN_ReturnCode retCode = setDtcMeanPixelFunc(aippParam, batchIndex, meanPixel, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetDtcMeanPixel(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t meanPixel[], uint32_t channelCount)
{
    auto getDtcMeanPixelFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetDtcMeanPixel)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetDtcMeanPixel"));
    ACL_EXPECT_NOT_NULL(getDtcMeanPixelFunc);

    OH_NN_ReturnCode retCode = getDtcMeanPixelFunc(aippParam, batchIndex, meanPixel, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetDtcMinPixel(HiAI_AippParam* aippParam, uint32_t batchIndex,
    float minPixel[], uint32_t channelCount)
{
    auto setDtcMinPixelFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetDtcMinPixel)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetDtcMinPixel"));
    ACL_EXPECT_NOT_NULL(setDtcMinPixelFunc);

    OH_NN_ReturnCode retCode = setDtcMinPixelFunc(aippParam, batchIndex, minPixel, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetDtcMinPixel(HiAI_AippParam* aippParam, uint32_t batchIndex,
    float minPixel[], uint32_t channelCount)
{
    auto getDtcMinPixelFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetDtcMinPixel)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetDtcMinPixel"));
    ACL_EXPECT_NOT_NULL(getDtcMinPixelFunc);

    OH_NN_ReturnCode retCode = getDtcMinPixelFunc(aippParam, batchIndex, minPixel, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetDtcVarReciPixel(HiAI_AippParam* aippParam, uint32_t batchIndex,
    float varReciPixel[], uint32_t channelCount)
{
    auto setDtcVarReciPixelFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetDtcVarReciPixel)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetDtcVarReciPixel"));
    ACL_EXPECT_NOT_NULL(setDtcVarReciPixelFunc);

    OH_NN_ReturnCode retCode = setDtcVarReciPixelFunc(aippParam, batchIndex, varReciPixel, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetDtcVarReciPixel(HiAI_AippParam* aippParam, uint32_t batchIndex,
    float varReciPixel[], uint32_t channelCount)
{
    auto getDtcVarReciPixelFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetDtcVarReciPixel)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetDtcVarReciPixel"));
    ACL_EXPECT_NOT_NULL(getDtcVarReciPixelFunc);

    OH_NN_ReturnCode retCode = getDtcVarReciPixelFunc(aippParam, batchIndex, varReciPixel, channelCount);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetRotationAngle(HiAI_AippParam* aippParam, uint32_t batchIndex,
    float rotationAngle)
{
    auto setRotationAngleFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetRotationAngle)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetRotationAngle"));
    ACL_EXPECT_NOT_NULL(setRotationAngleFunc);

    OH_NN_ReturnCode retCode = setRotationAngleFunc(aippParam, batchIndex, rotationAngle);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetRotationAngle(HiAI_AippParam* aippParam, uint32_t batchIndex,
    float* rotationAngle)
{
    auto getRotationAngleFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetRotationAngle)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetRotationAngle"));
    ACL_EXPECT_NOT_NULL(getRotationAngleFunc);

    OH_NN_ReturnCode retCode = getRotationAngleFunc(aippParam, batchIndex, rotationAngle);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_SetInputFormat(HiAI_AippParam* aippParam, HiAI_ImageFormat inputFormat)
{
    auto setInputFormatFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetInputFormat)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetInputFormat"));
    ACL_EXPECT_NOT_NULL(setInputFormatFunc);

    OH_NN_ReturnCode retCode = setInputFormatFunc(aippParam, inputFormat);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

HiAI_ImageFormat HIAI_NDK_HiAIAippParam_GetInputFormat(HiAI_AippParam* aippParam)
{
    auto getInputFormatFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetInputFormat)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetInputFormat"));
    ACL_EXPECT_NOT_NULL_R(getInputFormatFunc, HIAI_IMAGE_FORMAT_INVALID);

    return getInputFormatFunc(aippParam);
}

aclError HIAI_NDK_HiAIAippParam_SetInputShape(HiAI_AippParam* aippParam, uint32_t srcImageW, uint32_t srcImageH)
{
    auto setInputShapeFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_SetInputShape)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_SetInputShape"));
    ACL_EXPECT_NOT_NULL(setInputShapeFunc);

    OH_NN_ReturnCode retCode = setInputShapeFunc(aippParam, srcImageW, srcImageH);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

aclError HIAI_NDK_HiAIAippParam_GetInputShape(HiAI_AippParam* aippParam, uint32_t* srcImageW, uint32_t* srcImageH)
{
    auto getInputShapeFunc = reinterpret_cast<decltype(HMS_HiAIAippParam_GetInputShape)*>(
        NDKProxy::GetSymbol("HMS_HiAIAippParam_GetInputShape"));
    ACL_EXPECT_NOT_NULL(getInputShapeFunc);

    OH_NN_ReturnCode retCode = getInputShapeFunc(aippParam, srcImageW, srcImageH);
    return retCode == OH_NN_SUCCESS ? ACL_SUCCESS : ACL_ERROR_FAILURE;
}

}