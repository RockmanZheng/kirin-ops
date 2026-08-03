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
#include "slice_gelu_mul_tiling.h"
#include "register/op_def_registry.h"
#include "tiling/platform/platform_ascendc.h"

const uint64_t BUFFER_NUM = 2;
const uint32_t BLOCK_SIZE = 32;
const int32_t DATATYPE1 = 2;
const int32_t DATATYPE2 = 4;
const int32_t BYTESIZE = 8;

namespace optiling {
static ge::graphStatus TilingFunc(gert::TilingContext* context)
{

    SliceGeluMulTilingData tiling;
    int32_t NUM = 2;
    int32_t DATATYPE1 = 2;
    int32_t DATATYPE2 = 4;

    uint32_t sizeofdatatype;
    uint32_t totalLengthAligned;
    auto ascendcPlatform = platform_ascendc::PlatformAscendC(context->GetPlatformInfo());
    uint64_t ub_size;
    ascendcPlatform.GetCoreMemSize(platform_ascendc::CoreMemType::UB, ub_size);
    auto aivNum = ascendcPlatform.GetCoreNum();

    uint32_t totalLength = context->GetInputTensor(0)->GetShapeSize();
    auto dt = context->GetInputTensor(0)->GetDataType();
    if (dt == ge::DT_FLOAT16)
    {
        sizeofdatatype = DATATYPE1;
    }
    else
    {
        sizeofdatatype = DATATYPE2;
    }

    uint32_t ALIGN_NUM = BLOCK_SIZE / sizeofdatatype;
    uint32_t tiling_size = ((ub_size) / BLOCK_SIZE / 2) / NUM;
    tiling_size = tiling_size <= BYTESIZE ? tiling_size : tiling_size / BYTESIZE * BYTESIZE;

    uint32_t block_size = tiling_size * ALIGN_NUM;
    aivNum = (aivNum < totalLength / block_size) ? aivNum : (totalLength / block_size);
    aivNum = aivNum >= 1 ? aivNum : 1;

    uint32_t core_size = (totalLength / aivNum) / (ALIGN_NUM * BYTESIZE) * (ALIGN_NUM * BYTESIZE);
    uint32_t core_remain = totalLength - aivNum * core_size;

    auto x_orishape=context->GetInputShape(0);
    gert::Shape x_shape=x_orishape->GetOriginShape();
    size_t dimValue=x_shape.GetDimNum();
    if(dimValue<2) return ge::GRAPH_FAILED;

    int d2=x_shape.GetDim(dimValue-1)/2;
    int d1=x_shape.GetDim(dimValue-2);

    auto sumSize=4*BUFFER_NUM;
    auto blockFactorNum=ub_size/sumSize;

    tiling.set_blockFactor(blockFactorNum);
    tiling.set_outputDim1(d1);
    tiling.set_outputDim2(d2);
    tiling.set_totalLength(totalLength);
    tiling.set_ALIGN_NUM(ALIGN_NUM);
    tiling.set_tiling_size(tiling_size);
    tiling.set_block_size(block_size);
    tiling.set_aivNum(aivNum);
    tiling.set_core_size(core_size);
    tiling.set_core_remain(core_remain);

    context->SetBlockDim(1);

    tiling.SaveToBuffer(context->GetRawTilingData()->GetData(), context->GetRawTilingData()->GetCapacity());
    context->GetRawTilingData()->SetDataSize(tiling.GetDataSize());
    size_t *currentWorkspace = context->GetWorkspaceSizes(1);
    currentWorkspace[0] = 0;
    return ge::GRAPH_SUCCESS;
}
}


namespace ge {
static ge::graphStatus InferShape(gert::InferShapeContext* context)
{
    const gert::Shape* x1_shape = context->GetInputShape(0);
    gert::Shape* y_shape = context->GetOutputShape(0);
    *y_shape = *x1_shape;

    std::uint32_t dimnum=x1_shape->GetDimNum();
    std::uint32_t dim=x1_shape->GetDim(dimnum-1);
    y_shape->SetDim(dimnum-1,dim/2);
    return GRAPH_SUCCESS;
}
static ge::graphStatus InferDataType(gert::InferDataTypeContext* context)
{
    context->SetOutputDataType(0, context->GetInputDataType(0));
    return GRAPH_SUCCESS;
}
}


namespace ops {
class SliceGeluMul : public OpDef {
public:
    explicit SliceGeluMul(const char* name) : OpDef(name)
    {
        this->Input("a")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16, ge::DT_FLOAT,ge::DT_INT32})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND,ge::FORMAT_ND});
        this->Output("b")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16, ge::DT_FLOAT,ge::DT_INT32})
            .Format({ge::FORMAT_ND, ge::FORMAT_ND,ge::FORMAT_ND});

        this->SetInferShape(ge::InferShape);
        this->SetInferDataType(ge::InferDataType);

        this->AICore()
            .SetTiling(optiling::TilingFunc);
        this->AICore().AddConfig("kirinx90");

    }
};

OP_ADD(SliceGeluMul);
}
