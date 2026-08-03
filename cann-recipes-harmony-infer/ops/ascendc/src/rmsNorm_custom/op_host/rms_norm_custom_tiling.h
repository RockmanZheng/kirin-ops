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
#include "register/tilingdata_base.h"
#include "tiling/rmsnorm/rmsnorm_tiling.h"
#include "tiling/platform/platform_ascendc.h"
#include "register/op_def_registry.h"

namespace optiling {
BEGIN_TILING_DATA_DEF(TilingRmsNormGm2Ub)
    TILING_DATA_FIELD_DEF(uint64_t, blockM);
    TILING_DATA_FIELD_DEF(uint32_t, splitM);
    TILING_DATA_FIELD_DEF(uint32_t, splitK);
    TILING_DATA_FIELD_DEF(uint32_t, loopK);
    TILING_DATA_FIELD_DEF(uint32_t, tailK);
END_TILING_DATA_DEF;
REGISTER_TILING_DATA_CLASS(TilingRmsNormGm2UbOp, TilingRmsNormGm2Ub)

BEGIN_TILING_DATA_DEF(TilingRmsNormLargeReduce)
    TILING_DATA_FIELD_DEF(uint32_t, reduceSplitK);
    TILING_DATA_FIELD_DEF(uint32_t, reduceLoopK);
    TILING_DATA_FIELD_DEF(uint32_t, reduceTailK);
END_TILING_DATA_DEF;
REGISTER_TILING_DATA_CLASS(TilingRmsNormLargeReduceOp, TilingRmsNormLargeReduce)

BEGIN_TILING_DATA_DEF(RMSNormTilingData)
    TILING_DATA_FIELD_DEF(uint64_t, originM);
    TILING_DATA_FIELD_DEF(uint64_t, originK);
    TILING_DATA_FIELD_DEF(uint32_t, hasGamma);
    TILING_DATA_FIELD_DEF(float, epsilon);
    TILING_DATA_FIELD_DEF_STRUCT(RmsNormTiling, rmsNormTiling);
    TILING_DATA_FIELD_DEF_STRUCT(TilingRmsNormGm2Ub, tilingDataGm2Ub);
    TILING_DATA_FIELD_DEF_STRUCT(TilingRmsNormLargeReduce, tilingDataLargeReduce);
END_TILING_DATA_DEF;

REGISTER_TILING_DATA_CLASS(RmsNormCustom, RMSNormTilingData)
}