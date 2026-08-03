/**
 * ************************************************************************************
 * Copyright (c) 2025 Huibin Lin
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
 * ************************************************************************************
 *
 * @File    :   band_norm_music.cpp
 * @Time    :   2025/05/11 17:53:50
 * @Author  :   Huibin Lin
 * @Contact :   steelbin@126.com
 * @Version :   1.0
 * @Desc    :   Device-side implementation of BandNorm Music op.
 * ************************************************************************************
 */


#include "kernel_operator.h"
using namespace AscendC;

constexpr uint32_t BUFFER_NUM = 2;
constexpr uint32_t NPU_DUMP_NUM = 32;
constexpr float EPS =1.1920928955078125e-7;  // the eps of FP32 in torch

#include "kernel_operator.h"


class KernalBandNormMusic
{
public:
    __aicore__ inline KernalBandNormMusic() {}

    __aicore__ inline void Init(GM_ADDR x1, GM_ADDR x2, GM_ADDR magnitude,
                                const BandNormMusicTilingData &tilingInfo, TPipe *pipeIn)
    {
        const BandNormMusicTilingData &tilingData = tilingInfo;

        bLen = tilingData.bLen;
        tLen = tilingData.tLen;
        bandWide = tilingData.bandWide;
        blockElement = tilingData.blockElement;
        blockHalfAlignSize = tilingData.blockHalfAlignSize;
        blockHalfAlignElement = tilingData.blockHalfAlignElement;
        blockFloatAlignSize = tilingData.blockFloatAlignSize;
        blockFloatAlignElement = tilingData.blockFloatAlignElement;
        gatherBaseAddrOffset = tLen * sizeof(float);

        x1Gm.SetGlobalBuffer((__gm__ DTYPE_X1 *)x1, bLen * blockElement);
        x2Gm.SetGlobalBuffer((__gm__ DTYPE_X2 *)x2, bLen * blockElement);
        magnitudeGm.SetGlobalBuffer((__gm__ DTYPE_MAGNITUDE *)magnitude, bLen * tLen);

        pipe = pipeIn;
        pipe->InitBuffer(inQueue, BUFFER_NUM, 2 * blockHalfAlignSize);
        pipe->InitBuffer(outQueue, BUFFER_NUM, tLen * sizeof(DTYPE_MAGNITUDE));
        pipe->InitBuffer(floatBuf, blockFloatAlignSize * 2);
        pipe->InitBuffer(gatherIndexBuf, tLen * sizeof(uint32_t));

        blockDataCopyInParams.blockCount = 1;
        blockDataCopyInParams.blockLen = blockElement * sizeof(DTYPE_X1);
        blockDataCopyInParams.srcStride = 0;
        blockDataCopyInParams.dstStride = 0;
        blockDataCopyInParams.rsv = 0;

        blockPadParamsCopyIn.isPad = false;
        blockPadParamsCopyIn.leftPadding = 0;
        blockPadParamsCopyIn.rightPadding = 0;
        blockPadParamsCopyIn.paddingValue = 0;

        blockDataCopyOutParams.blockCount = 1;
        blockDataCopyOutParams.blockLen = tLen * sizeof(DTYPE_MAGNITUDE);
        blockDataCopyOutParams.srcStride = 0;
        blockDataCopyOutParams.dstStride = 0;
        blockDataCopyOutParams.rsv = 0;

        gatherIndexLocal = gatherIndexBuf.Get<int32_t>();
        floatBufLocal = floatBuf.Get<float>();
    }


    __aicore__ inline void Process() 
    {
        CreateVecIndex(gatherIndexLocal, 0, tLen);
        Muls(gatherIndexLocal, gatherIndexLocal, int32_t(4), tLen);

        for(uint32_t i=0; i<bLen;i++)
        {
            uint32_t input_gm_offset = i * blockElement;
            uint32_t output_gm_offset = i * tLen;
            CopyIn(input_gm_offset);
            Compute();
            CopyOut(output_gm_offset);
        }
    }


private:
    __aicore__ inline void CopyIn(uint32_t offset){
        LocalTensor<DTYPE_X1> inputLocal = inQueue.AllocTensor<DTYPE_X1>();
        DataCopyPad(inputLocal, x1Gm[offset], blockDataCopyInParams, blockPadParamsCopyIn);
        DataCopyPad(inputLocal[blockHalfAlignElement], x2Gm[offset], blockDataCopyInParams, blockPadParamsCopyIn);
        inQueue.EnQue(inputLocal);
    }

    __aicore__ inline void Compute(){
        LocalTensor<DTYPE_X1> inputLocal = inQueue.DeQue<DTYPE_X1>();
        LocalTensor<DTYPE_MAGNITUDE> ouputLocal = outQueue.AllocTensor<DTYPE_MAGNITUDE>();

        Cast(floatBufLocal, inputLocal, RoundMode::CAST_NONE, blockElement);
        Cast(floatBufLocal[blockFloatAlignElement], inputLocal[blockHalfAlignElement], RoundMode::CAST_NONE, blockElement);

        Mul(floatBufLocal, floatBufLocal, floatBufLocal, blockFloatAlignElement*2);
        Add(floatBufLocal[blockFloatAlignElement], floatBufLocal, floatBufLocal[blockFloatAlignElement], blockFloatAlignElement);

        for (uint32_t band_idx=1; band_idx<bandWide; band_idx++)
        {
            uint32_t gather_base_addr = gatherBaseAddrOffset * band_idx;
            Gather(floatBufLocal, floatBufLocal[blockFloatAlignElement], (LocalTensor<uint32_t> &)gatherIndexLocal, gather_base_addr, tLen);
            Add(floatBufLocal[blockFloatAlignElement], floatBufLocal, floatBufLocal[blockFloatAlignElement], tLen);
        }

        Adds(floatBufLocal[blockFloatAlignElement], floatBufLocal[blockFloatAlignElement], EPS, tLen);
        Sqrt(floatBufLocal, floatBufLocal[blockFloatAlignElement], tLen);
        Cast(ouputLocal, floatBufLocal, RoundMode::CAST_ROUND, tLen);

        outQueue.EnQue(ouputLocal);
        inQueue.FreeTensor(inputLocal);
    }


    __aicore__ inline void CopyOut(uint32_t offset)
    {
        LocalTensor<DTYPE_MAGNITUDE> outputLocal = outQueue.DeQue<DTYPE_MAGNITUDE>();
        DataCopyPad(magnitudeGm[offset], outputLocal, blockDataCopyOutParams);
        outQueue.FreeTensor(outputLocal);
    }

private:
    TPipe *pipe;
    TQue<QuePosition::VECIN, 1> inQueue;
    TQue<QuePosition::VECOUT, 1> outQueue;
    TBuf<QuePosition::VECCALC> gatherIndexBuf, floatBuf;

    GlobalTensor<DTYPE_X1> x1Gm;
    GlobalTensor<DTYPE_X2> x2Gm;
    GlobalTensor<DTYPE_MAGNITUDE> magnitudeGm;

    LocalTensor<int32_t> gatherIndexLocal;
    LocalTensor<float> floatBufLocal;

    uint32_t bLen;
    uint32_t tLen;
    uint32_t bandWide;
    uint32_t blockElement;
    uint32_t blockHalfAlignSize;
    uint32_t blockHalfAlignElement;
    uint32_t blockFloatAlignSize;
    uint32_t blockFloatAlignElement;
    uint32_t gatherBaseAddrOffset;
    float eps;

    DataCopyExtParams blockDataCopyInParams;
    DataCopyPadExtParams<DTYPE_X1> blockPadParamsCopyIn;
    DataCopyExtParams blockDataCopyOutParams;
};

extern "C" __global__ __aicore__ void band_norm_music(GM_ADDR x1, GM_ADDR x2, GM_ADDR magnitude, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);

    TPipe pipe;
    KernalBandNormMusic op;
    op.Init(x1, x2, magnitude, tiling_data, &pipe);
    op.Process();
}