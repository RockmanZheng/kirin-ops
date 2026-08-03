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

input1 = helper.make_tensor_value_info("input1", TensorProto.FLOAT16, [1, 5, 5120])

output1 = helper.make_tensor_value_info("output1", TensorProto.FLOAT16, [1, 5, 2560])

custom_op = onnx.helper.make_node(
    "SliceGeluMul",
    name = "sliceGeluMul",
    inputs = ['input1'],
    outputs = ['output1'],
)

graph_def = helper.make_graph(
    [custom_op],
    "test-model",
    [input1],
    [output1],
    initializer = [],
)

model_def = helper.make_model(graph_def, producer_name='onnx-example')

model_def.opset_import[0].version = 11

model_def.ir_version = 6

new_model_path = os.path.join("./", 'SliceGeluMul.onnx')

onnx.save(model_def, new_model_path)