/* Copyright (c) 2025 Huawei Technologies Co., Ltd. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* GatherDequantInt8 —— Device 侧核函数（AscendC，目标 Kirin9020 单核）
 *
 * 解决工具链痛点：9020 的框架 Gather(GatherV2D) 的 data 仅支持 fp32/fp16/int32，
 * 不接受 uint8/int8，导致"图内 INT8 embedding 查表"无法用标准算子实现。
 * 本自定义算子在 AI Core 上直接以 uint8 读表 + Cast 到 half + 逐行反量化，
 * 使图内 embedding 可压到 INT8(~2.5MB)，对齐外置 bin 的体积。
 *
 *   y[i, :] = (half(table[idx, :]) - zero_point[idx]) * scale[idx],  idx = indices[i]
 *
 * 流水：indices/scale/zp 一次性载入 UB；逐 token 走 CopyIn(uint8 行) -> Cast(half)
 *       -> Adds(-zp) -> Muls(scale) -> CopyOut(half 行)。
 */
#include "kernel_operator.h"

namespace {
constexpr int32_t BUFFER_NUM = 2;  // double buffer：搬运与计算流水并行

__aicore__ inline uint32_t AlignUp(uint32_t a, uint32_t b)
{
    return (b == 0) ? a : ((a + b - 1) / b) * b;
}
}  // namespace

class KernelGatherDequantInt8 {
public:
    __aicore__ inline KernelGatherDequantInt8() {}

    __aicore__ inline void Init(GM_ADDR table, GM_ADDR indices, GM_ADDR scale,
                                GM_ADDR zeroPoint, GM_ADDR y,
                                uint32_t numIndices, uint32_t embDim, uint32_t vocab)
    {
        this->numIndices = numIndices;
        this->embDim = embDim;
        this->vocab = vocab;

        tableGm.SetGlobalBuffer((__gm__ uint8_t*)table, (uint64_t)vocab * embDim);
        indicesGm.SetGlobalBuffer((__gm__ int32_t*)indices, numIndices);
        scaleGm.SetGlobalBuffer((__gm__ half*)scale, vocab);
        zpGm.SetGlobalBuffer((__gm__ half*)zeroPoint, vocab);
        yGm.SetGlobalBuffer((__gm__ half*)y, (uint64_t)numIndices * embDim);

        // 持久驻留 UB：indices / scale / zp（一次载入，按 idx 随机访问）
        pipe.InitBuffer(idxBuf, AlignUp(numIndices * sizeof(int32_t), 32));
        pipe.InitBuffer(scaleBuf, AlignUp(vocab * sizeof(half), 32));
        pipe.InitBuffer(zpBuf, AlignUp(vocab * sizeof(half), 32));
        // 逐 token 流水缓冲
        pipe.InitBuffer(inQueueRow, BUFFER_NUM, AlignUp(embDim * sizeof(uint8_t), 32));
        pipe.InitBuffer(outQueueRow, BUFFER_NUM, AlignUp(embDim * sizeof(half), 32));
        pipe.InitBuffer(calcBuf, AlignUp(embDim * sizeof(half), 32));
    }

    __aicore__ inline void Process()
    {
        LoadLookupTables();
        AscendC::LocalTensor<int32_t> idxLocal = idxBuf.Get<int32_t>();
        AscendC::LocalTensor<half> scaleLocal = scaleBuf.Get<half>();
        AscendC::LocalTensor<half> zpLocal = zpBuf.Get<half>();

        for (uint32_t i = 0; i < this->numIndices; ++i) {
            int32_t idx = idxLocal.GetValue(i);
            half s = scaleLocal.GetValue(idx);
            // dav_l310(kirin9020) 不允许标量 half 算术：取负在 float 上做再转回 half
            half negZ = static_cast<half>(-static_cast<float>(zpLocal.GetValue(idx)));
            GatherDequantRow(i, static_cast<uint32_t>(idx), s, negZ);
        }
    }

private:
    // indices / scale / zp 一次性载入（非 32B 对齐用 DataCopyPad），并同步到标量单元
    __aicore__ inline void LoadLookupTables()
    {
        AscendC::LocalTensor<int32_t> idxLocal = idxBuf.Get<int32_t>();
        AscendC::LocalTensor<half> scaleLocal = scaleBuf.Get<half>();
        AscendC::LocalTensor<half> zpLocal = zpBuf.Get<half>();

        AscendC::DataCopyExtParams idxParams{1, this->numIndices * (uint32_t)sizeof(int32_t), 0, 0, 0};
        AscendC::DataCopyPadExtParams<int32_t> idxPad{false, 0, 0, 0};
        AscendC::DataCopyPad(idxLocal, indicesGm, idxParams, idxPad);

        AscendC::DataCopyExtParams vParams{1, this->vocab * (uint32_t)sizeof(half), 0, 0, 0};
        AscendC::DataCopyPadExtParams<half> vPad{false, 0, 0, 0};
        AscendC::DataCopyPad(scaleLocal, scaleGm, vParams, vPad);
        AscendC::DataCopyPad(zpLocal, zpGm, vParams, vPad);

        // 确保以上 MTE2 搬运完成后，标量单元 GetValue 才读取
        AscendC::PipeBarrier<PIPE_ALL>();
    }

    // 取第 idx 行 uint8 -> Cast half -> (q - zp) * scale -> 写到输出第 i 行
    __aicore__ inline void GatherDequantRow(uint32_t i, uint32_t idx, half s, half negZ)
    {
        AscendC::LocalTensor<uint8_t> rowU8 = inQueueRow.AllocTensor<uint8_t>();
        AscendC::DataCopy(rowU8, tableGm[(uint64_t)idx * this->embDim], this->embDim);
        inQueueRow.EnQue(rowU8);

        rowU8 = inQueueRow.DeQue<uint8_t>();
        AscendC::LocalTensor<half> calc = calcBuf.Get<half>();
        AscendC::Cast(calc, rowU8, AscendC::RoundMode::CAST_NONE, this->embDim);
        inQueueRow.FreeTensor(rowU8);

        AscendC::Adds(calc, calc, negZ, this->embDim);     // q - zp
        AscendC::LocalTensor<half> out = outQueueRow.AllocTensor<half>();
        AscendC::Muls(out, calc, s, this->embDim);          // (q - zp) * scale
        outQueueRow.EnQue<half>(out);

        out = outQueueRow.DeQue<half>();
        AscendC::DataCopy(yGm[(uint64_t)i * this->embDim], out, this->embDim);
        outQueueRow.FreeTensor(out);
    }

private:
    AscendC::TPipe pipe;
    AscendC::TBuf<AscendC::TPosition::VECCALC> idxBuf, scaleBuf, zpBuf, calcBuf;
    AscendC::TQue<AscendC::QuePosition::VECIN, BUFFER_NUM> inQueueRow;
    AscendC::TQue<AscendC::QuePosition::VECOUT, BUFFER_NUM> outQueueRow;
    AscendC::GlobalTensor<uint8_t> tableGm;
    AscendC::GlobalTensor<int32_t> indicesGm;
    AscendC::GlobalTensor<half> scaleGm;
    AscendC::GlobalTensor<half> zpGm;
    AscendC::GlobalTensor<half> yGm;
    uint32_t numIndices;
    uint32_t embDim;
    uint32_t vocab;
};

extern "C" __global__ __aicore__ void gather_dequant_int8(
    GM_ADDR table, GM_ADDR indices, GM_ADDR scale, GM_ADDR zero_point,
    GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling)
{
    GET_TILING_DATA(tiling_data, tiling);
    KernelGatherDequantInt8 op;
    op.Init(table, indices, scale, zero_point, y,
            tiling_data.numIndices, tiling_data.embDim, tiling_data.vocab);
    op.Process();
}
