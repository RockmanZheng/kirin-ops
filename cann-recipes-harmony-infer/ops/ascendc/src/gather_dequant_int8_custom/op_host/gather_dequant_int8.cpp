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

/* GatherDequantInt8 —— Host 侧：Tiling / Shape&DType 推导 / 算子原型注册
 *
 * 语义：per-row(per-token) 非对称 INT8 embedding 查表 + 反量化
 *   y[i, :] = (half(table[indices[i], :]) - zero_point[indices[i]]) * scale[indices[i]]
 * 其中
 *   table       uint8   [V, E]   逐行非对称量化的 embedding 表
 *   indices     int32   [...]    token id（任意 shape，按元素查表）
 *   scale       fp16    [V]      逐行 scale
 *   zero_point  fp16    [V]      逐行 zero_point
 *   y           fp16    [..., E] 反量化后的 embedding
 */
#include "gather_dequant_int8_tiling.h"
#include "register/op_def_registry.h"
#include "tiling/platform/platform_ascendc.h"

namespace optiling {
static ge::graphStatus TilingFunc(gert::TilingContext* context)
{
    GatherDequantInt8TilingData tiling;

    // 输入 0: table[V, E]
    const gert::StorageShape* tableShape = context->GetInputShape(0);
    const gert::Shape& ts = tableShape->GetStorageShape();
    uint32_t vocab = static_cast<uint32_t>(ts.GetDim(0));
    uint32_t embDim = static_cast<uint32_t>(ts.GetDim(ts.GetDimNum() - 1));

    // 输入 1: indices[...]，总元素数即查表次数
    const gert::StorageShape* idxShape = context->GetInputShape(1);
    const gert::Shape& is = idxShape->GetStorageShape();
    uint32_t numIndices = 1;
    for (size_t i = 0; i < is.GetDimNum(); ++i) {
        numIndices *= static_cast<uint32_t>(is.GetDim(i));
    }

    tiling.set_numIndices(numIndices);
    tiling.set_embDim(embDim);
    tiling.set_vocab(vocab);

    // Kirin9020 AI Core 单核
    context->SetBlockDim(1);
    tiling.SaveToBuffer(context->GetRawTilingData()->GetData(),
                        context->GetRawTilingData()->GetCapacity());
    context->GetRawTilingData()->SetDataSize(tiling.GetDataSize());

    size_t* currentWorkspace = context->GetWorkspaceSizes(1);
    currentWorkspace[0] = 0;
    return ge::GRAPH_SUCCESS;
}
}  // namespace optiling

namespace ge {
static ge::graphStatus InferShape(gert::InferShapeContext* context)
{
    const gert::Shape* tableShape = context->GetInputShape(0);
    const gert::Shape* idxShape = context->GetInputShape(1);
    gert::Shape* yShape = context->GetOutputShape(0);

    int64_t embDim = tableShape->GetDim(tableShape->GetDimNum() - 1);
    size_t idxRank = idxShape->GetDimNum();

    // y.shape = indices.shape ++ [E]
    yShape->SetDimNum(idxRank + 1);
    for (size_t i = 0; i < idxRank; ++i) {
        yShape->SetDim(i, idxShape->GetDim(i));
    }
    yShape->SetDim(idxRank, embDim);
    return GRAPH_SUCCESS;
}

static ge::graphStatus InferDataType(gert::InferDataTypeContext* context)
{
    // 输出固定 fp16（与 scale/zero_point 一致）
    context->SetOutputDataType(0, ge::DT_FLOAT16);
    return GRAPH_SUCCESS;
}
}  // namespace ge

namespace ops {
class GatherDequantInt8 : public OpDef {
public:
    explicit GatherDequantInt8(const char* name) : OpDef(name)
    {
        this->Input("table")
            .ParamType(REQUIRED)
            .DataType({ge::DT_UINT8})
            .Format({ge::FORMAT_ND});
        this->Input("indices")
            .ParamType(REQUIRED)
            .DataType({ge::DT_INT32})
            .Format({ge::FORMAT_ND});
        this->Input("scale")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Input("zero_point")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});
        this->Output("y")
            .ParamType(REQUIRED)
            .DataType({ge::DT_FLOAT16})
            .Format({ge::FORMAT_ND});

        this->SetInferShape(ge::InferShape);
        this->SetInferDataType(ge::InferDataType);

        this->AICore().SetTiling(optiling::TilingFunc);
        this->AICore().AddConfig("kirin9020");
    }
};

OP_ADD(GatherDequantInt8);
}  // namespace ops
