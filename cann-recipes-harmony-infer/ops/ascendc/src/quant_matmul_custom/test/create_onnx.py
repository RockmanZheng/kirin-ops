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
from onnx import helper
from onnx import AttributeProto, TensorProto, GraphProto
import onnx
import os
import numpy as np

A = helper.make_tensor_value_info("A", TensorProto.FLOAT16, [1, 1280])

B = helper.make_tensor_value_info("B", TensorProto.UINT8, [97239040])

P2 = helper.make_tensor_value_info("P2", TensorProto.FLOAT16, [6077440])

P3 = helper.make_tensor_value_info("P3", TensorProto.FLOAT16, [6077440])

Y = helper.make_tensor_value_info("Y", TensorProto.FLOAT16, [1, 151936])

custom_op = onnx.helper.make_node(
    "QuantMatmulCustom",
    name = "QuantMatmulCustom",
    inputs = ['A', 'B', 'P2', 'P3'],
    outputs = ['Y'],
    input1_shape = [1280, 151936],
)

graph_def = helper.make_graph(
    [custom_op],
    "test-model",
    [A, B, P2, P3],
    [Y],
    initializer = [],
)

model_def = helper.make_model(graph_def, producer_name='onnx-example')

model_def.opset_import[0].version = 11

model_def.ir_version = 6

new_model_path = os.path.join("./", 'QuantMatmulCustom.onnx')

onnx.save(model_def, new_model_path)