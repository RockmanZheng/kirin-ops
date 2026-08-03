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
#include "sobel_custom_tiling.h"
#include "register/op_def_registry.h"
#include "tiling/platform/platform_ascendc.h"
#include <iostream>


namespace optiling {
const uint32_t USE_CORE_NUM = 1;
const uint32_t BUFFER_NUM = 2;
const uint32_t tileNum = 8;
// tile切分块大小
const uint32_t h = 9;
const uint32_t w = 256;
const uint32_t c = 3;
static ge::graphStatus TilingFunc(gert::TilingContext* context)
{
    auto totalLength = context->GetInputTensor(0)->GetShapeSize();
    auto blockLength = totalLength / USE_CORE_NUM;
    auto tileLength = h * w * c ; 
    auto grayLength = h * w; 

    SobelCustomTilingData tiling;
    const gert::StorageShape* x1_shape = context->GetInputShape(0);
    auto H = x1_shape->GetStorageShape().GetDim(1);
    auto W = x1_shape->GetStorageShape().GetDim(2);
   
    int32_t data_sz = 1;
    for (int i = 0; i < x1_shape->GetStorageShape().GetDimNum(); i++)
        data_sz *= x1_shape->GetStorageShape().GetDim(i);
    tiling.set_size(data_sz);
    tiling.set_totalLength(totalLength);
    tiling.set_blockLength(blockLength);
    tiling.set_tileNum(tileNum);
    tiling.set_tileLength(tileLength);
    tiling.set_grayLength(grayLength);
    tiling.set_H(H);
    tiling.set_W(W);
    
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

    int output_height = x1_shape->GetDim(1) - 2; // 减去卷积核的高度减2
    int output_width = x1_shape->GetDim(2) - 2; // 减去卷积核的宽度减2
    
    // 设置输出的形状
    y_shape->SetDimNum(4);
    y_shape->SetDim(0, x1_shape->GetDim(0)); // batch
    y_shape->SetDim(1, x1_shape->GetDim(3) - 2); // channels
    y_shape->SetDim(2, output_height); // height 20
    y_shape->SetDim(3, output_width); // width 1026

    return GRAPH_SUCCESS;
}
static ge::graphStatus InferDataType(gert::InferDataTypeContext* context)
{
    context->SetOutputDataType(0, context->GetInputDataType(0));
    return GRAPH_SUCCESS;
}
}


namespace ops {
class SobelCustom : public OpDef {
public:
    explicit SobelCustom(const char* name) : OpDef(name)
    {
        this->Input("x")
            .ParamType(REQUIRED)
            .DataType({ge::DT_UINT8})
            .Format({ge::FORMAT_ND});
        this->Output("y")
            .ParamType(REQUIRED)
            .DataType({ge::DT_UINT8})
            .Format({ge::FORMAT_ND});

        this->SetInferShape(ge::InferShape);
        this->SetInferDataType(ge::InferDataType);
        
        OpAICoreConfig aicore_config;
        aicore_config.DynamicCompileStaticFlag(true)
            .DynamicFormatFlag(true)
            .DynamicRankSupportFlag(true)
            .DynamicShapeSupportFlag(true);

        this->AICore()
            .SetTiling(optiling::TilingFunc);
        this->AICore().AddConfig("kirinx90", aicore_config);

    }
};

OP_ADD(SobelCustom);
}
