# coding=utf-8
# Copyright (c) 2025, HUAWEI CORPORATION.  All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""生成 GatherDequantInt8 单算子 ONNX 测试模型。

算子语义（per-row 非对称 INT8 embedding 查表 + 反量化）：
    y[i, :] = (half(table[indices[i], :]) - zero_point[indices[i]]) * scale[indices[i]]

输入：
    table       uint8   [V, E]   逐行非对称量化的 embedding 表
    indices     int32   [1, N]   token id
    scale       fp16    [V]      逐行 scale
    zero_point  fp16    [V]      逐行 zero_point
输出：
    y           fp16    [1, N, E]
"""
import os

import onnx
from onnx import TensorProto, helper

# 测试规格（与 gen_data.py 保持一致）
VOCAB = 1024
EMB_DIM = 256
SEQ_LEN = 30

table = helper.make_tensor_value_info("table", TensorProto.UINT8, [VOCAB, EMB_DIM])
indices = helper.make_tensor_value_info("indices", TensorProto.INT32, [1, SEQ_LEN])
scale = helper.make_tensor_value_info("scale", TensorProto.FLOAT16, [VOCAB])
zero_point = helper.make_tensor_value_info("zero_point", TensorProto.FLOAT16, [VOCAB])
y = helper.make_tensor_value_info("y", TensorProto.FLOAT16, [1, SEQ_LEN, EMB_DIM])

custom_op = helper.make_node(
    "GatherDequantInt8",
    name="GatherDequantInt8",
    inputs=["table", "indices", "scale", "zero_point"],
    outputs=["y"],
)

graph_def = helper.make_graph(
    [custom_op],
    "test-model",
    [table, indices, scale, zero_point],
    [y],
    initializer=[],
)

model_def = helper.make_model(graph_def, producer_name="onnx-example")
model_def.opset_import[0].version = 11
model_def.ir_version = 6

new_model_path = os.path.join("./", "GatherDequantInt8.onnx")
onnx.save(model_def, new_model_path)
print(f"[create_onnx] -> {new_model_path}  (V={VOCAB}, E={EMB_DIM}, N={SEQ_LEN})")
