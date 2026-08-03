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
#include "rms_norm_custom_tiling.h"
#include "register/op_def_registry.h"
#include "tiling/platform/platform_ascendc.h"
using namespace std;

namespace optiling {
static ge::graphStatus TilingFunc(gert::TilingContext* context)
{
    RMSNormTilingData rmsNormTilingData;

    rmsNormTilingData.set_originM(64);
    rmsNormTilingData.set_originK(2048);
    rmsNormTilingData.set_epsilon(1e-5);
    rmsNormTilingData.set_hasGamma(1);

    std::vector<int64_t> shape_vec = {1, 1, 2048};
    ge::Shape srcShape(shape_vec);
    ge::Shape originSrcShape(shape_vec);

    optiling::RmsNormTiling myTiling;
    uint32_t maxSize,minSize;

    AscendC::GetRmsNormMaxMinTmpSize(srcShape,2,maxSize,minSize,false);
    AscendC::GetRmsNormTilingInfo(srcShape, originSrcShape, maxSize, 2, rmsNormTilingData.rmsNormTiling, false);

    rmsNormTilingData.tilingDataGm2Ub.set_blockM(64);
    rmsNormTilingData.tilingDataGm2Ub.set_splitM(1);
    rmsNormTilingData.tilingDataGm2Ub.set_splitK(128);
    rmsNormTilingData.tilingDataGm2Ub.set_loopK(15);
    rmsNormTilingData.tilingDataGm2Ub.set_tailK(128);

    rmsNormTilingData.tilingDataLargeReduce.set_reduceSplitK(0);
    rmsNormTilingData.tilingDataLargeReduce.set_reduceLoopK(0);
    rmsNormTilingData.tilingDataLargeReduce.set_reduceTailK(0);

    context->SetBlockDim(1);
    rmsNormTilingData.SaveToBuffer(context->GetRawTilingData()->GetData(), context->GetRawTilingData()->GetCapacity());
    context->GetRawTilingData()->SetDataSize(rmsNormTilingData.GetDataSize());

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
    return GRAPH_SUCCESS;
}
static ge::graphStatus InferDataType(gert::InferDataTypeContext* context)
{
    context->SetOutputDataType(0, context->GetInputDataType(0));
    return GRAPH_SUCCESS;
}
}


namespace ops {
class RmsNormCustom : public OpDef {
public:
    explicit RmsNormCustom(const char* name) : OpDef(name)
    {
        this->Input("x")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Input("gamma")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Output("y")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});

        this->SetInferShape(ge::InferShape);
        this->SetInferDataType(ge::InferDataType);

        this->AICore()
            .SetTiling(optiling::TilingFunc);
        this->AICore().AddConfig("kirinx90");

    }
};

OP_ADD(RmsNormCustom);
}
