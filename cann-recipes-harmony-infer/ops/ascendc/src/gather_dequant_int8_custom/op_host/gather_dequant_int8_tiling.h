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

#ifndef OP_HOST_GATHER_DEQUANT_INT8_TILING_H
#define OP_HOST_GATHER_DEQUANT_INT8_TILING_H
#include "register/tilingdata_base.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(GatherDequantInt8TilingData)
  TILING_DATA_FIELD_DEF(uint32_t, numIndices);  // 待查表 token 总数 = prod(indices.shape)
  TILING_DATA_FIELD_DEF(uint32_t, embDim);      // embedding 维度 E = table.shape[1]
  TILING_DATA_FIELD_DEF(uint32_t, vocab);       // 词表大小 V = table.shape[0]
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(GatherDequantInt8, GatherDequantInt8TilingData)
}

#endif  // OP_HOST_GATHER_DEQUANT_INT8_TILING_H
