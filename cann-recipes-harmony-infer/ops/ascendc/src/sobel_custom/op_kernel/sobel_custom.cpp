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
#include "kernel_operator.h"
#include "sobel_custom_base.h"
using namespace AscendC;
constexpr int32_t BUFFER_NUM = 2; // tensor num for each queue
template <typename T>
class KernelSobelCustom {
public:
    __aicore__ inline KernelSobelCustom() {}
__aicore__ inline void Init(GM_ADDR x, GM_ADDR y, const SobelCustomTilingData* tilingData)
    {   
        this->blockLength = tilingData->blockLength; 
        this->tileLength = tilingData->tileLength;  
        this->grayLength = tilingData->grayLength;  
        this->H = tilingData->H;
        this->W = tilingData->W;
        
        xGm.SetGlobalBuffer((__gm__ T*)x + this->blockLength * GetBlockIdx(), this->blockLength);
        yGm.SetGlobalBuffer((__gm__ T*)y + this->blockLength * GetBlockIdx(), this->blockLength);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, tileLength * sizeof(T));
        pipe.InitBuffer(outQueueY, BUFFER_NUM, tileLength * sizeof(T));
        // 为TBuf初始化分配内存，分配内存长度为TILE_LENGTH * sizeof(T)字节
        pipe.InitBuffer(calcBuf, (tileLength * 4 + grayLength * 14 + w * 12));
    }
    __aicore__ inline void Process()
    {
        cntH = SobelCustom::CeilDiv(this->H, h);
        cntW = SobelCustom::CeilDiv(this->W, w);
        for (int32_t i = 0; i < cntH; i++) {
            for (int32_t j = 0; j < cntW; j++) {
                CopyIn(i, j);
                Compute(i, j);
                CopyOut(i, j);
            }
        }
    }
private:
    __aicore__ inline void CopyIn(int32_t i, int32_t j)
    {
        LocalTensor<T> xLocal = inQueueX.AllocTensor<T>();
        DataCopyExtParams dataCopyParams;
        uint32_t offset;
        if( (0 <= i && i < cntH - 1) && j == 0){//首列无最后一块
            dataCopyParams.blockCount = h;
            dataCopyParams.blockLen = w * c * sizeof(T);
            dataCopyParams.srcStride = this->W  * c * sizeof(T) - dataCopyParams.blockLen;
            dataCopyParams.dstStride = SobelCustom::CeilDiv(w * c * sizeof(T) , 32) -  SobelCustom::CeilDiv(dataCopyParams.blockLen , 32);
            offset = (i * (h - 2) * this->W + j * (w - 2)) * c;
        }
        else if( (0 <= i && i < cntH - 1) && (0 < j < cntW - 1)){//非首块非尾块
            dataCopyParams.blockCount = h;
            dataCopyParams.blockLen =  w * c * sizeof(T);
            dataCopyParams.srcStride = this->W  * c * sizeof(T) - dataCopyParams.blockLen;
            dataCopyParams.dstStride = SobelCustom::CeilDiv(w * c * sizeof(T) , 32) -  SobelCustom::CeilDiv(dataCopyParams.blockLen , 32);
            offset = (i * (h - 2) * this->W + j * (w - 2)) * c;
        }
        else if( (0 <= i && i < cntH - 1) && j == cntW - 1){//最后一列尾块
            dataCopyParams.blockCount = h;
            dataCopyParams.blockLen = SobelCustom::Min(this->W - j * (w - 2), w) * c * sizeof(T);
            dataCopyParams.srcStride = this->W * c * sizeof(T) - dataCopyParams.blockLen;
            dataCopyParams.dstStride = SobelCustom::CeilDiv(w * c * sizeof(T) , 32) -  SobelCustom::CeilDiv(dataCopyParams.blockLen , 32);
            offset = (i * (h - 2) * this->W + j * (w - 2)) * c;
        }
        else if(i == cntH - 1 &&  (0 <= j && j < cntW)){//最后一行
            dataCopyParams.blockCount = SobelCustom::Min(this->H - i * (h - 2), h);
            dataCopyParams.blockLen = SobelCustom::Min(this->W - j * (w - 2), w) * c * sizeof(T);
            dataCopyParams.srcStride = this->W * c * sizeof(T) - dataCopyParams.blockLen;
            dataCopyParams.dstStride = SobelCustom::CeilDiv(w * c * sizeof(T) , 32) -  SobelCustom::CeilDiv(dataCopyParams.blockLen , 32);
            offset = (i * (h - 2) * this->W + j * (w - 2)) * c;
        }
        DataCopyPadExtParams<T> padParams(false, 0, 0, 0);
        DataCopyPad(xLocal, xGm[offset], dataCopyParams, padParams);
        inQueueX.EnQue(xLocal);
     
    }
     __aicore__ inline void Compute(int32_t i, int32_t j)
    {
        LocalTensor<T> xLocal = inQueueX.DeQue<T>();
        LocalTensor<T> yLocal = outQueueY.AllocTensor<T>();
        // 从buf获取Tensor,Tensor为pipe分配的所有内存大小，为TILE_LENGTH * sizeof(out_dtype)字节
        LocalTensor<uint8_t> stackBuffer = calcBuf.AllocTensor<uint8_t>();
        LocalTensor<T> tempTensor0 = calcBuf.GetWithOffset<T>(tileLength, tileLength * 1);
        LocalTensor<half> tempTensor1 = calcBuf.GetWithOffset<half>(tileLength, tileLength * 2);

        LocalTensor<half> dx = calcBuf.GetWithOffset<half>(grayLength, tileLength * 4);
        LocalTensor<half> dy = calcBuf.GetWithOffset<half>(grayLength, tileLength * 4 + grayLength * 2);

        LocalTensor<uint32_t> newindex = calcBuf.GetWithOffset<uint32_t>(w, tileLength * 4 + grayLength * 4 + w * 2);
        LocalTensor<int32_t> index = newindex.ReinterpretCast<int32_t>();
        LocalTensor<uint32_t> newindex1 = calcBuf.GetWithOffset<uint32_t>(w, tileLength * 4 + grayLength * 4 + w * 8);
        LocalTensor<int32_t> index1 = newindex1.ReinterpretCast<int32_t>();
        
        LocalTensor<half> tmpBuf0 = calcBuf.GetWithOffset<half>(grayLength, tileLength * 4 + grayLength * 4 + w * 12);
        LocalTensor<half> tmpBuf1 = calcBuf.GetWithOffset<half>(grayLength, tileLength * 4 + grayLength * 6 + w * 12);
        LocalTensor<half> tmpBuf2 = calcBuf.GetWithOffset<half>(grayLength, tileLength * 4 + grayLength * 8 + w * 12);
        
        LocalTensor<half> tmpBuf3 = calcBuf.GetWithOffset<half>(grayLength, tileLength * 4 + grayLength * 10 + w * 12);
        LocalTensor<half> tmpBuf4 = calcBuf.GetWithOffset<half>(grayLength, tileLength * 4 + grayLength * 12 + w * 12);
        
        // 上述TBuf共占用tileLength*(1+1+2) + grayLength*(2+2+2+2+2+2+2) + w*(2+4+2+4) = 125952 
        TransposeParamsExt transposeParams;
        transposeParams.nSize = n;
        transposeParams.hSize = h;
        transposeParams.wSize = w;
        transposeParams.cSize = c;
        transposeParams.transposeType = transposetype;
        // NHWC->NCHW
        AscendC::Transpose(tempTensor0, xLocal, stackBuffer, transposeParams);
        // u8->half
        AscendC::Cast(tempTensor1, tempTensor0, AscendC::RoundMode::CAST_NONE, tileLength);
        // RGB2Gray
        LocalTensor<half> grayTensor0 = tempTensor1;
        LocalTensor<half> grayTensor1 = tempTensor1[w * h];
        LocalTensor<half> grayTensor2 = tempTensor1[w * h * 2];
        // 0.299 * r + 0.587 * g + 0.114 * b
        AscendC::Muls(grayTensor0, grayTensor0, half(0.299), w * h);
        AscendC::Muls(grayTensor1, grayTensor1, half(0.587), w * h);
        AscendC::Muls(grayTensor2, grayTensor2, half(0.114), w * h);
        AscendC::Add(grayTensor1, grayTensor0, grayTensor1, w * h);
        AscendC::Add(grayTensor2, grayTensor1, grayTensor2, w * h);
        // Sobel-x、Sobel-y
        LocalTensor<half> data = grayTensor2;
        

        // Sobel-x
        // 生成index: [2, 3, 4, ..., w - 1]
        AscendC::CreateVecIndex(index, int32_t(2), w - 2); // 固定操作移到for循环外面去，优化性能
        AscendC::Muls(index, index, int32_t(2), w - 2);
        
        // index:int16->uint32
        AscendC::Cast(newindex, index, AscendC::RoundMode::CAST_NONE, w - 2);
        for(int i = 0; i < h - 2; i++) {
            // 将data[i][2,3,4,...,w-1]存储到tmpBuf0[0, 1, 2, ..., w-2]
            AscendC::Gather(tmpBuf0, data[i * w], newindex, 0 , w - 2); 
            AscendC::Gather(tmpBuf1, data[(i + 1) * w], newindex, 0 , w - 2);
            AscendC::Gather(tmpBuf2, data[(i + 2) * w], newindex, 0 , w - 2);
            // part1
            AscendC::Muls(dx[i * w], data[i * w], half(-1), w - 2);
            AscendC::Muls(tmpBuf3, data[(i + 1) * w], half(-2), w - 2);
            AscendC::Muls(tmpBuf4, data[(i + 2) * w], half(-1), w - 2);
            AscendC::Add(dx[i * w], dx[i * w], tmpBuf3, w - 2);
            AscendC::Add(dx[i * w], dx[i * w], tmpBuf4, w - 2);
            // part2: 由于data[i][j + 2]的地址不是32Byte对齐的，所以需要用gather
            // 生成index: [2, 3, 4, ..., w - 1]
            AscendC::Muls(tmpBuf1, tmpBuf1, half(2), w - 2);
            AscendC::Add(dx[i * w], dx[i * w], tmpBuf0, w - 2);
            AscendC::Add(dx[i * w], dx[i * w], tmpBuf1, w - 2);
            AscendC::Add(dx[i * w], dx[i * w], tmpBuf2, w - 2);
            // 注：这里用到的tmpBuf0，不需要额外申请，可复用图中XY融合后的那个buf。
        }
        // Sobel-y
        // 生成index: [1, 2, 3, ..., w - 2]和[2, 3, 4, ..., w - 1]
        AscendC::CreateVecIndex(index1, int32_t(1), w - 2);
        AscendC::Muls(index1, index1, int32_t(2), w - 2);
        // index:int16->uint32
       
        AscendC::Cast(newindex1, index1, AscendC::RoundMode::CAST_NONE, w - 2);
        for(int j = 0; j < h - 2; j++){
            // 将data[i][1,2,3,...,w-2]存储到tmpBuf1[0, 1, 2, ..., w-3]
            AscendC::Gather(tmpBuf1, data[j * w], newindex1, 0 , w - 2); 
            AscendC::Gather(tmpBuf3, data[(j + 2) * w], newindex1, 0 , w - 2);
            // 将data[i][2,3,4,...,w-1]存储到tmpBuf0[0, 1, 2, ..., w-3]
            AscendC::Gather(tmpBuf0, data[j * w], newindex, 0 , w - 2); 
            AscendC::Gather(tmpBuf2, data[(j + 2) * w], newindex, 0 , w - 2);
            // part1
            AscendC::Muls(dy[j * w], data[j * w], half(-1), w - 2);
            AscendC::Muls(tmpBuf4, data[(j + 2) * w], half(1), w - 2);
            AscendC::Add(dy[j * w], dy[j * w], tmpBuf4, w - 2);
            // part2: 由于data[i][j + 1], data[i][j + 2]的地址不是32Byte对齐的，所以需要用gather
            AscendC::Muls(tmpBuf1, tmpBuf1, half(-2), w - 2);
            AscendC::Muls(tmpBuf3, tmpBuf3, half(2), w - 2);
            AscendC::Add(dy[j * w], dy[j * w], tmpBuf1, w - 2);
            AscendC::Add(dy[j * w], dy[j * w], tmpBuf3, w - 2);
            // part3
            AscendC::Muls(tmpBuf0, tmpBuf0, half(-1), w - 2);
            AscendC::Add(dy[j * w], dy[j * w], tmpBuf0, w - 2);
            AscendC::Add(dy[j * w], dy[j * w], tmpBuf2, w - 2);
        }
        // Sobelxy融合
        AscendC::Abs(dx, dx, w * (h - 2));
        AscendC::Abs(dy, dy, w * (h - 2));
        AscendC::Add(tmpBuf0, dx, dy, w * (h - 2));
        // half->u8
        AscendC::Cast(yLocal, tmpBuf0, AscendC::RoundMode::CAST_CEIL, w * (h - 2));
        outQueueY.EnQue<T>(yLocal);
        inQueueX.FreeTensor(xLocal);
        
    }
    __aicore__ inline void CopyOut(int32_t i, int32_t j)
    {
        LocalTensor<T> yLocal = outQueueY.DeQue<T>();
        DataCopyExtParams dataCopyParams;
        uint32_t offset;
        
        if( (0 <= i && i < cntH - 1) && (0 <= j && j < cntW - 1)){//左上
            dataCopyParams.blockCount = h - 2;
            dataCopyParams.blockLen = (w - 2) * sizeof(T);
            dataCopyParams.srcStride = 0;
            dataCopyParams.dstStride = (this->W - 2)  * sizeof(T) - dataCopyParams.blockLen;
            offset = i * (h - 2) * (this->W - 2) + j * (w - 2);
        }
        else if( (0 <= i && i < cntH) && (j == cntW - 1)){//最后一列
            dataCopyParams.blockCount = SobelCustom::Min(this->H - 2 - i * (h - 2), h - 2);
            dataCopyParams.blockLen = SobelCustom::Min(this->W - 2 - j * (w - 2), w - 2) * sizeof(T);
            dataCopyParams.srcStride = SobelCustom::CeilDiv(w * sizeof(T) , 32) -  SobelCustom::CeilDiv(dataCopyParams.blockLen , 32);
            dataCopyParams.dstStride = (this->W - 2)  * sizeof(T) - dataCopyParams.blockLen;
            offset = i * (h - 2) * (this->W - 2) + j * (w - 2);
            
        }
        else if( (i == cntH - 1) && (0 <= j && j < cntW - 1)){//最后一行块
            dataCopyParams.blockCount = SobelCustom::Min(this->H - 2 - i * (h - 2), h - 2);
            dataCopyParams.blockLen = (w - 2) * sizeof(T);
            dataCopyParams.srcStride = 0;
            dataCopyParams.dstStride = (this->W - 2)  * sizeof(T) - dataCopyParams.blockLen;
            offset = i * (h - 2) * (this->W - 2) + j * (w - 2);
            
        }
        DataCopyPad(yGm[offset], yLocal, dataCopyParams);
        outQueueY.FreeTensor(yLocal);
        
    }
private:
    TPipe pipe;
    TQue<QuePosition::VECIN, BUFFER_NUM> inQueueX;
    TQue<QuePosition::VECOUT, BUFFER_NUM> outQueueY;
    TBuf<TPosition::VECCALC> calcBuf;
    GlobalTensor<T> xGm;
    GlobalTensor<T> yGm;
    uint32_t n = 1; 
    uint32_t h = 9;
    uint32_t w = 256;
    uint32_t c = 3;
    TransposeType transposetype = TransposeType::TRANSPOSE_NHWC2NCHW;
    uint32_t blockLength;
    uint32_t tileLength;
    uint32_t grayLength;
    uint32_t H;
    uint32_t W;
    uint32_t cntH;
    uint32_t cntW;
};

extern "C" __global__ __aicore__ void sobel_custom(GM_ADDR x, GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);
    KernelSobelCustom<uint8_t> op;
    op.Init(x, y, &tiling_data);
    op.Process();
}