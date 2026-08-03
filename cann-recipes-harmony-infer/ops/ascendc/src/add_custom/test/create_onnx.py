import os
import numpy as np
import onnx
from onnx import helper
from onnx import TensorProto


def create_model():
    AddCustom = helper.make_node(
        "AddCustom",
        inputs = ["input1", "input2"],
        outputs = ["output"],
        name = "add",
        bias = 1
    )

    input1_input = helper.make_tensor_value_info("input1", TensorProto.FLOAT16, [8, 2048])
    input2_input = helper.make_tensor_value_info("input2", TensorProto.FLOAT16, [8, 2048])
    add_output = helper.make_tensor_value_info('output', TensorProto.FLOAT16, [8, 2048])

    graph = helper.make_graph(
        nodes = [AddCustom],
        name = 'custom_graph',
        inputs = [input1_input, input2_input],
        outputs = [add_output]
    )

    model = helper.make_model(graph, producer_name='onnx-example')
    model.opset_import[0].version = 11
    model.ir_version = 6

    return model

model = create_model()
print(onnx.helper.printable_graph(model.graph))
onnx.save(model, "./add_custom.onnx")


