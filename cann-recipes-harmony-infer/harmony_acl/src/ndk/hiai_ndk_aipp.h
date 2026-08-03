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
 
#ifndef HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_AIPP_H
#define HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_AIPP_H

#include "hiai_foundation/hiai_aipp_param.h"
#include "acl/acl_base.h"

namespace hiai {

HiAI_AippParam *HIAI_NDK_HiAIAippParam_Create(uint32_t batchNum);
void HIAI_NDK_HiAIAippParam_Destroy(HiAI_AippParam** aippParam);

void* HIAI_NDK_HiAIAippParam_GetData(HiAI_AippParam* aippParam);
uint32_t HIAI_NDK_HiAIAippParam_GetDataSize(HiAI_AippParam* aippParam);
uint32_t HIAI_NDK_HiAIAippParam_GetBatchCount(HiAI_AippParam* aippParam);

aclError HIAI_NDK_HiAIAippParam_SetInputIndex(HiAI_AippParam* aippParam, uint32_t inputIndex);
int HIAI_NDK_HiAIAippParam_GetInputIndex(HiAI_AippParam* aippParam);

aclError HIAI_NDK_HiAIAippParam_SetInputAippIndex(HiAI_AippParam* aippParam, uint32_t inputAippIndex);
int HIAI_NDK_HiAIAippParam_GetInputAippIndex(HiAI_AippParam* aippParam);

aclError HIAI_NDK_HiAIAippParam_SetCscConfig(HiAI_AippParam* aippParam,
    HiAI_ImageFormat inputFormat, HiAI_ImageFormat outputFormat, HiAI_ImageColorSpace space);
aclError HIAI_NDK_HiAIAippParam_GetCscConfig(HiAI_AippParam* aippParam,
    HiAI_ImageFormat* inputFormat, HiAI_ImageFormat* outputFormat, HiAI_ImageColorSpace* space);

aclError HIAI_NDK_HiAIAippParam_SetChannelSwapConfig(HiAI_AippParam* aippParam, bool rbuvSwapSwitch, bool axSwapSwitch);
aclError HIAI_NDK_HiAIAippParam_GetChannelSwapConfig(HiAI_AippParam* aippParam, bool* rbuvSwapSwitch, bool* axSwapSwitch);

aclError HIAI_NDK_HiAIAippParam_SetSingleBatchMultiCrop(HiAI_AippParam* aippParam, bool singleBatchMultiCrop);
bool HIAI_NDK_HiAIAippParam_GetSingleBatchMultiCrop(HiAI_AippParam* aippParam);

aclError HIAI_NDK_HiAIAippParam_SetCropConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, 
    uint32_t startPosW, uint32_t startPosH, uint32_t croppedW, uint32_t croppedH);
aclError HIAI_NDK_HiAIAippParam_GetCropConfig(HiAI_AippParam* aippParam, uint32_t batchIndex, 
    uint32_t* startPosW, uint32_t* startPosH, uint32_t* croppedW, uint32_t* croppedH);

aclError HIAI_NDK_HiAIAippParam_SetResizeConfig(
    HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t resizedW, uint32_t resizedH);
aclError HIAI_NDK_HiAIAippParam_GetResizeConfig(
    HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t* resizedW, uint32_t* resizedH);

aclError HIAI_NDK_HiAIAippParam_SetPadConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t leftPadSize, uint32_t rightPadSize, uint32_t topPadSize, uint32_t bottomPadSize);
aclError HIAI_NDK_HiAIAippParam_GetPadConfig(HiAI_AippParam* aippParam, uint32_t batchIndex,
    uint32_t* leftPadSize, uint32_t* rightPadSize, uint32_t* topPadSize, uint32_t* bottomPadSize);

aclError HIAI_NDK_HiAIAippParam_SetChannelPadding(
    HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t paddingValues[], uint32_t channelCount);
aclError HIAI_NDK_HiAIAippParam_GetChannelPadding(
    HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t paddingValues[], uint32_t channelCount);

aclError HIAI_NDK_HiAIAippParam_SetDtcMeanPixel(
    HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t meanPixel[], uint32_t channelCount);
aclError HIAI_NDK_HiAIAippParam_GetDtcMeanPixel(
    HiAI_AippParam* aippParam, uint32_t batchIndex, uint32_t meanPixel[], uint32_t channelCount);

aclError HIAI_NDK_HiAIAippParam_SetDtcMinPixel(
    HiAI_AippParam* aippParam, uint32_t batchIndex, float minPixel[], uint32_t channelCount);
aclError HIAI_NDK_HiAIAippParam_GetDtcMinPixel(
    HiAI_AippParam* aippParam, uint32_t batchIndex, float minPixel[], uint32_t channelCount);

aclError HIAI_NDK_HiAIAippParam_SetDtcVarReciPixel(
    HiAI_AippParam* aippParam, uint32_t batchIndex, float varReciPixel[], uint32_t channelCount);
aclError HIAI_NDK_HiAIAippParam_GetDtcVarReciPixel(
    HiAI_AippParam* aippParam, uint32_t batchIndex, float varReciPixel[], uint32_t channelCount);

aclError HIAI_NDK_HiAIAippParam_SetRotationAngle(
    HiAI_AippParam* aippParam, uint32_t batchIndex, float rotationAngle);
aclError HIAI_NDK_HiAIAippParam_GetRotationAngle(
    HiAI_AippParam* aippParam, uint32_t batchIndex, float* rotationAngle);

aclError HIAI_NDK_HiAIAippParam_SetInputFormat(HiAI_AippParam* aippParam, HiAI_ImageFormat inputFormat);
HiAI_ImageFormat HIAI_NDK_HiAIAippParam_GetInputFormat(HiAI_AippParam* aippParam);

aclError HIAI_NDK_HiAIAippParam_SetInputShape(
    HiAI_AippParam* aippParam, uint32_t srcImageW, uint32_t srcImageH);
aclError HIAI_NDK_HiAIAippParam_GetInputShape(
    HiAI_AippParam* aippParam, uint32_t* srcImageW, uint32_t* srcImageH);

} // namespace hiai
#endif // HIAI_FOUNDATION_HIAI_NDK_HIAI_NDK_AIPP_H