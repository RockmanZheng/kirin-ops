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
 * @Time    :   2025/05/11 17:20:09
 * @Author  :   Huibin Lin
 * @Contact :   steelbin@126.com
 * @Version :   1.0
 * @Desc    :   Host-side implementation of BandNorm Music op.
 * ************************************************************************************
 */

#include "band_norm_music_tiling.h"
#include "register/op_def_registry.h"
#include "tiling/platform/platform_ascendc.h"

constexpr bool DEBUG = false;
constexpr bool SIM_DEBUG = true;
constexpr int32_t HALF_BYTES = 2;
constexpr int32_t FLOAT_BYTES = 4;
constexpr int32_t MAX_T = 62;
constexpr int32_t MAX_BAND_WIDE = 121;
#define CEIL_ALIGN(N, n) (((N) + (n) - 1) / (n) * (n)) // 功能：将数值N向上对齐到n的整数倍（即≥N的最小n的倍数）CEIL_ALIGN(17,8) → 24
#define FLOOR_ALIGN(N, n) ((N) / (n) * (n))            // 功能：将数值N向下对齐到n的整数倍（即≤N的最大n的倍数）FLOOR_ALIGN(17,8) → 16
#define CEIL(N, n) (((N) + (n) - 1) / (n))             // 功能：计算N除以n的向上取整结果, CEIL(17,8) → 3 (因为17/8=2.125向上取整为3)
#define FLOOR(N, n) ((N) / (n))


namespace optiling {
static ge::graphStatus TilingFunc(gert::TilingContext* context)
{

    BandNormMusicTilingData tiling;

    // get platform info
    if (DEBUG)
    {
        auto ascendcPlatform =
            platform_ascendc::PlatformAscendC(context->GetPlatformInfo());

        auto socVersion = ascendcPlatform.GetSocVersion();
        uint64_t ub_size;
        ascendcPlatform.GetCoreMemSize(platform_ascendc::CoreMemType::UB, ub_size);
        uint64_t l1_size;
        ascendcPlatform.GetCoreMemSize(platform_ascendc::CoreMemType::L1, l1_size);
        printf("socVersion = %d\n", socVersion);
        printf("platform = %d\n", ascendcPlatform);
        printf("ub_size = %lu\n", ub_size);
        printf("l1_size = %lu\n", l1_size);
    }

    const gert::StorageShape* x1_shape = context->GetInputShape(0);
    int32_t data_sz = 1;
    uint32_t x_B = x1_shape->GetStorageShape().GetDim(0);
    uint32_t x_band_wide = x1_shape->GetStorageShape().GetDim(1);
    uint32_t x_T = x1_shape->GetStorageShape().GetDim(2);

    tiling.set_bLen(x_B);
    tiling.set_tLen(x_T);
    tiling.set_bandWide(x_band_wide);
    uint32_t block_len = x_band_wide * x_T;
    uint32_t block_len_half_size = block_len * HALF_BYTES;
    uint32_t block_len_float_size = block_len * FLOAT_BYTES;
    uint32_t block_len_half_align_size = CEIL_ALIGN(block_len_half_size, 32);
    uint32_t block_len_half_align_element = uint32_t(block_len_half_align_size / HALF_BYTES);
    uint32_t block_len_float_align_size = CEIL_ALIGN(block_len_float_size, 32);
    uint32_t block_len_float_align_element = uint32_t(block_len_float_align_size / FLOAT_BYTES);

    tiling.set_blockElement(block_len);
    tiling.set_blockHalfAlignSize(block_len_half_align_size);
    tiling.set_blockHalfAlignElement(block_len_half_align_element);
    tiling.set_blockFloatAlignSize(block_len_float_align_size);
    tiling.set_blockFloatAlignElement(block_len_float_align_element);

    if (DEBUG)
    {
        printf("tiling.bLen = %d\n", tiling.get_bLen());
        printf("tiling.tLen = %d\n", tiling.get_tLen());
        printf("tiling.bandWide = %d\n", tiling.get_bandWide());
        printf("tiling.blockElement = %d\n", tiling.get_blockElement());
        printf("tiling.blockHalfAlignSize = %d\n", tiling.get_blockHalfAlignSize());
        printf("tiling.blockHalfAlignElement = %d\n", tiling.get_blockHalfAlignElement());
        printf("tiling.blockFloatAlignSize = %d\n", tiling.get_blockFloatAlignSize());
        printf("tiling.blockFloatAlignElement = %d\n", tiling.get_blockFloatAlignElement());
    }
    

    context->SetBlockDim(1);
    tiling.SaveToBuffer(context->GetRawTilingData()->GetData(), context->GetRawTilingData()->GetCapacity());
    context->GetRawTilingData()->SetDataSize(tiling.GetDataSize());
    uint32_t sysWorkspaceSize;
    if (SIM_DEBUG)
    {
        printf("SIM_DEBUG = %d\n", SIM_DEBUG);
        sysWorkspaceSize = 16777216; // 16M，for sim
    }
    else
    {
        auto ascendcPlatform =
            platform_ascendc::PlatformAscendC(context->GetPlatformInfo());
        sysWorkspaceSize = ascendcPlatform.GetLibApiWorkSpaceSize();
    }

    size_t *currentWorkspace = context->GetWorkspaceSizes(1);
    currentWorkspace[0] = sysWorkspaceSize;

    if (DEBUG)
    {
        printf("sysWorkspaceSize = %u\n", sysWorkspaceSize);
    }

    return ge::GRAPH_SUCCESS;
}
}


namespace ge {
static ge::graphStatus InferShape(gert::InferShapeContext* context)
{
    const gert::Shape *x1_shape = context->GetInputShape(0);
    const gert::Shape *x2_shape = context->GetInputShape(1);

    // 边界检查
    if (*x1_shape != *x2_shape)
    {
        printf("x1_shape != x2_shape\n");
        return ge::GRAPH_PARAM_INVALID;
    }
    if (x1_shape->GetDim(0) != 2)
    {
        printf("x1_shape->GetDim(0) != 2\n");
        return ge::GRAPH_PARAM_INVALID;
    }
    if (x1_shape->GetDim(1) > MAX_BAND_WIDE)
    {
        printf("x1_shape->GetDim(1) > %d\n", MAX_BAND_WIDE);
        return ge::GRAPH_PARAM_INVALID;
    }
    if (x1_shape->GetDim(2) > MAX_T)
    {
        printf("x1_shape->GetDim(2) > %d\n", MAX_T);
        return ge::GRAPH_PARAM_INVALID;
    }


    gert::Shape *magnitude_shape = context->GetOutputShape(0);
    magnitude_shape->SetDimNum(2);
    magnitude_shape->SetDim(0, x1_shape->GetDim(0));
    magnitude_shape->SetDim(1, x1_shape->GetDim(2));

    return GRAPH_SUCCESS;
}
static ge::graphStatus InferDataType(gert::InferDataTypeContext* context)
{
    context->SetOutputDataType(0, context->GetInputDataType(0));
    return GRAPH_SUCCESS;
}
}


namespace ops {
class BandNormMusic : public OpDef {
public:
    explicit BandNormMusic(const char* name) : OpDef(name)
    {
        this->Input("x1")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Input("x2")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Output("magnitude")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});

        this->SetInferShape(ge::InferShape);
        this->SetInferDataType(ge::InferDataType);

        this->AICore()
            .SetTiling(optiling::TilingFunc);
        this->AICore().AddConfig("kirin9030");

    }
};

OP_ADD(BandNormMusic);
}
