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

namespace {

constexpr uint32_t SOBEL_910B2_SCALAR_KERNEL = 1;
constexpr uint32_t RGB_CHANNELS = 3;

__aicore__ inline int32_t AbsInt32(int32_t value)
{
    return value < 0 ? -value : value;
}

__aicore__ inline uint8_t CeilClampScaledToU8(int32_t value)
{
    if (value <= 0) {
        return 0;
    }
    int32_t rounded = (value + 999) / 1000;
    if (rounded >= 255) {
        rounded = 255;
    }
    return static_cast<uint8_t>(rounded);
}

}  // namespace

template <typename T>
class KernelSobelCustom {
public:
    __aicore__ inline KernelSobelCustom() {}

    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, const SobelCustomTilingData* tilingData)
    {
        H = tilingData->H;
        W = tilingData->W;
        outH = H > 2 ? H - 2 : 0;
        outW = W > 2 ? W - 2 : 0;

        xGm.SetGlobalBuffer((__gm__ T*)x, static_cast<uint64_t>(H) * W * RGB_CHANNELS);
        yGm.SetGlobalBuffer((__gm__ T*)y, static_cast<uint64_t>(outH) * outW);
    }

    __aicore__ inline void Process()
    {
        for (uint32_t row = 0; row < outH; ++row) {
            for (uint32_t col = 0; col < outW; ++col) {
                const int32_t g00 = GrayScaled(row, col);
                const int32_t g01 = GrayScaled(row, col + 1);
                const int32_t g02 = GrayScaled(row, col + 2);
                const int32_t g10 = GrayScaled(row + 1, col);
                const int32_t g12 = GrayScaled(row + 1, col + 2);
                const int32_t g20 = GrayScaled(row + 2, col);
                const int32_t g21 = GrayScaled(row + 2, col + 1);
                const int32_t g22 = GrayScaled(row + 2, col + 2);

                const int32_t dx = -g00 - 2 * g10 - g20 + g02 + 2 * g12 + g22;
                const int32_t dy = -g00 - 2 * g01 - g02 + g20 + 2 * g21 + g22;
                const uint64_t outputOffset = static_cast<uint64_t>(row) * outW + col;
                yGm.SetValue(outputOffset, CeilClampScaledToU8(AbsInt32(dx) + AbsInt32(dy)));
            }
        }
    }

private:
    __aicore__ inline int32_t GrayScaled(uint32_t row, uint32_t col) const
    {
        const uint64_t base = (static_cast<uint64_t>(row) * W + col) * RGB_CHANNELS;
        const int32_t r = xGm.GetValue(base);
        const int32_t g = xGm.GetValue(base + 1);
        const int32_t b = xGm.GetValue(base + 2);
        return 299 * r + 587 * g + 114 * b;
    }

    GlobalTensor<T> xGm;
    GlobalTensor<T> yGm;
    uint32_t H = 0;
    uint32_t W = 0;
    uint32_t outH = 0;
    uint32_t outW = 0;
};

extern "C" __global__ __aicore__ void sobel_custom(GM_ADDR x, GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling)
{
    GET_TILING_DATA(tiling_data, tiling);
    KernelSobelCustom<uint8_t> op;
    op.Init(x, y, &tiling_data);
    op.Process();
}
