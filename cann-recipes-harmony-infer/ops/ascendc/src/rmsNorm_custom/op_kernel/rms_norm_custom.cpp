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
#include "lib/normalization/rmsnorm.h"

using namespace AscendC;

namespace AscendC {
template<typename T>
class KernelRmsNormArReduceInsideUb {
    static constexpr uint32_t IN_BUFFER_NUM = 3;
    static constexpr uint32_t OUT_BUFFER_NUM = 2;

public:
    __aicore__ inline KernelRmsNormArReduceInsideUb() {}

    __aicore__ inline void Init(TPipe* pipeIn,
        GM_ADDR srcGm, GM_ADDR gamaGM, GM_ADDR dstGm, const RMSNormTilingData& tilingData)
    {
        pipe_ = pipeIn;
        oriM_ = tilingData.originM;
        oriK_ = tilingData.originK; // 非reduce轴GM->UB轴切分因子splitK_, 与oriK_相等
        splitM_ = tilingData.tilingDataGm2Ub.splitM;

        elementNumPerBlk_ = ONE_BLOCK_SIZE / sizeof(T);
        uint64_t blockM_ = tilingData.tilingDataGm2Ub.blockM; // 核切分, 每个block的计算总量，向上取整，尾块用min处理
        uint32_t blockStart_ = blockM_ * GetBlockIdx(); // 当前block对应的 GM 搬运起始地址

        uint32_t blockEnd_ =
            ((blockStart_ + blockM_) > oriM_) ? oriM_ : blockStart_ + blockM_; // 当前block对应的 GM 搬运结束地址
        uint32_t blockMCount_ = blockEnd_ - blockStart_; // 当前block的计算总量 (M轴上)

        loopM_ = blockMCount_ / splitM_;
        tailM_ = blockMCount_ % splitM_;
        if ((loopM_ != 0) && (tailM_ == 0)) {
            loopM_ = loopM_ - 1;
            tailM_ = splitM_;
        }
        tiling_ = tilingData.rmsNormTiling;
        epsilon_ = static_cast<T>(tilingData.epsilon);
        hasGamma_ = tilingData.hasGamma;

        srcGlobal_.SetGlobalBuffer((__gm__ T*)srcGm + blockStart_ * oriK_, blockMCount_ * oriK_);
        dstGlobal_.SetGlobalBuffer((__gm__ T*)dstGm + blockStart_ * oriK_, blockMCount_ * oriK_);
        if (hasGamma_) {
            gamaGlobal_.SetGlobalBuffer((__gm__ T*)gamaGM, oriK_ * 2);
        }

        uint32_t padKNum = CeilDivision(oriK_, elementNumPerBlk_) * elementNumPerBlk_;
        uint32_t pipeBuffer = padKNum * splitM_;
        if (hasGamma_) {
            pipe_->InitBuffer(inGamaSrc_, 1, padKNum * sizeof(half));
        }
        pipe_->InitBuffer(inQueueSrc_, IN_BUFFER_NUM, pipeBuffer * sizeof(T));
        pipe_->InitBuffer(outQueueDst_, OUT_BUFFER_NUM, pipeBuffer * sizeof(T));
        pipe_->InitBuffer(fp16GamaSrcBuf, oriK_ * sizeof(half));
    }

    __aicore__ inline void Process()
    {
        if (hasGamma_) {
            ProcessHasGamma();
        } else {
            ProcessNoGamma();
        }
    }

private:
    __aicore__ inline void ProcessNoGamma()
    {
        for (int32_t i = 0; i < loopM_; i++) {
            CopyIn(i, splitM_);
            ComputeNoGamma();
            CopyOut(i, splitM_);
        }
        tiling_.sLength = tailM_;
        CopyIn(loopM_, tailM_);
        ComputeNoGamma();
        CopyOut(loopM_, tailM_);
    }

    __aicore__ inline void ProcessHasGamma()
    {
        CopyInGamma();
        LocalTensor<half> gammaLocal = inGamaSrc_.DeQue<half>();
        for (int32_t i = 0; i < loopM_; i++) {
            CopyIn(i, splitM_);
            ComputeHasGamma(gammaLocal);
            CopyOut(i, splitM_);
        }
        tiling_.sLength = tailM_;
        CopyIn(loopM_, tailM_);
        ComputeHasGamma(gammaLocal);
        CopyOut(loopM_, tailM_);
        inGamaSrc_.FreeTensor(gammaLocal);
    }

    __aicore__ inline void CopyIn(int32_t progress, uint32_t LengthM)
    {
        LocalTensor<T> srcLocal = inQueueSrc_.AllocTensor<T>();
        DataCopyExtParams dataCopyParams(LengthM, oriK_ * sizeof(T), 0, 0, 0);
        DataCopyPadExtParams<T> padParams(true, 0, 0, 0);
        DataCopyPad(srcLocal, srcGlobal_[progress * splitM_ * oriK_], dataCopyParams, padParams);
        inQueueSrc_.EnQue(srcLocal);
    }

    __aicore__ inline void ComputeHasGamma(const LocalTensor<T> &gammaLocal)
    {
        LocalTensor<T> srcLocal = inQueueSrc_.DeQue<T>();
        LocalTensor<T> dstLocal = outQueueDst_.AllocTensor<T>();
        RmsNorm<T, false>(dstLocal, srcLocal, gammaLocal, epsilon_, tiling_);
        outQueueDst_.EnQue<T>(dstLocal);
        inQueueSrc_.FreeTensor(srcLocal);
    }

    __aicore__ inline void ComputeNoGamma()
    {
        LocalTensor<T> srcLocal = inQueueSrc_.DeQue<T>();
        LocalTensor<T> dstLocal = outQueueDst_.AllocTensor<T>();
        LocalTensor<T> gammaLocal;
        RmsNorm<T, false>(dstLocal, srcLocal, gammaLocal, epsilon_, tiling_);
        outQueueDst_.EnQue<T>(dstLocal);
        inQueueSrc_.FreeTensor(srcLocal);
    }

    __aicore__ inline void CopyOut(int32_t progress, uint32_t LengthM)
    {
        LocalTensor<T> dstLocal = outQueueDst_.DeQue<T>();
        DataCopyExtParams dataCopyParams(LengthM, oriK_ * sizeof(T), 0, 0, 0);
        DataCopyPad(dstGlobal_[progress * splitM_ * oriK_], dstLocal, dataCopyParams);
        outQueueDst_.FreeTensor(dstLocal);
    }

    __aicore__ inline void CopyInGamma()
    {
        LocalTensor<T> gammaLocal = inGamaSrc_.AllocTensor<T>();
        DataCopyPadExtParams<T> padParams(true, 0, 0, 0);
        DataCopyExtParams dataCopyParams1(1, oriK_ * sizeof(T), 0, 0, 0);
        DataCopyPad(gammaLocal, gamaGlobal_, dataCopyParams1, padParams);
        inGamaSrc_.EnQue(gammaLocal);
    }

    private:
    TPipe* pipe_;
    TQue<QuePosition::VECIN, 1> inQueueSrc_;
    TQue<QuePosition::VECIN, 1> inGamaSrc_;
    TQue<QuePosition::VECOUT, 1> outQueueDst_;
    TBuf<TPosition::VECCALC> fp16GamaSrcBuf;
    GlobalTensor<T> srcGlobal_, dstGlobal_;
    GlobalTensor<T> gamaGlobal_;

    uint32_t elementNumPerBlk_ = 0;

    uint64_t oriM_;
    uint64_t oriK_;

    uint32_t splitM_;
    uint32_t loopM_;
    uint32_t tailM_;

    RmsNormTiling tiling_;
    T epsilon_;
    bool hasGamma_;
};
}

extern "C" __global__ __aicore__ void rms_norm_custom(GM_ADDR x, GM_ADDR gamma, GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);

    AscendC::TPipe pipe;
    AscendC::KernelRmsNormArReduceInsideUb<half> op;
    op.Init(&pipe, x, gamma, y, tiling_data);
    op.Process();
}