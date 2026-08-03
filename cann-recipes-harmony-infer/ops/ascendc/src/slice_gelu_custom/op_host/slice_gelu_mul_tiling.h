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
#ifndef OP_HOST_SLICEGELUMUL_TILING_H
#define OP_HOST_SLICEGELUMUL_TILING_H
#include "register/tilingdata_base.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(SliceGeluMulTilingData)
  TILING_DATA_FIELD_DEF(uint32_t, totalLength);
  TILING_DATA_FIELD_DEF(uint32_t, tileNum);
  TILING_DATA_FIELD_DEF(uint32_t, ALIGN_NUM);
  TILING_DATA_FIELD_DEF(uint32_t, tiling_size);
  TILING_DATA_FIELD_DEF(uint32_t, block_size);
  TILING_DATA_FIELD_DEF(uint32_t, aivNum);
  TILING_DATA_FIELD_DEF(uint32_t, core_size);
  TILING_DATA_FIELD_DEF(uint32_t, core_remain);
  TILING_DATA_FIELD_DEF(uint32_t, outputDim2);
  TILING_DATA_FIELD_DEF(uint32_t, outputDim1);
  TILING_DATA_FIELD_DEF(uint32_t, blockFactor);
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(SliceGeluMul, SliceGeluMulTilingData)
}

#endif // OP_HOST_SLICEGELUMUL_TILING_H
