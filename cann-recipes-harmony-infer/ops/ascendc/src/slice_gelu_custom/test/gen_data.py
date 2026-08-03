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
import os
import sys
import torch
import torch.nn as nn
import numpy as np
import math

#1.构建模型
d0: int = 1
d1: int = 5
d2: int = 5120

def my_gelu(x):
    x_value = x.numpy()
    y_value = []
    for value0 in x_value:
        for value1 in value0:
            for value in value1:
                y_value.append(value / (1 + math.exp(-1.702 * value)))
    y = torch.tensor(y_value).reshape(x.shape)
    return y
class SlicedGELUModel(nn.Module):
    def __init__(self):
        super().__init__()
        self.gelu = my_gelu

        #切片参数配置（可修改区域）
        self.slice_dim = 2      #切片维度（示例中切第2维，即5120维度）
        self.slice_start = 0    #起始索引
        self.slice_end = int(d2 / 2)   #结束索引（不包括该位置）
        self.slice_end_1 = d2

    def forward(self, x):
        #执行切片操作 [1， 1024， 5120] =》 [1， 1024， 2560]
        y = x[:, :, self.slice_end:self.slice_end_1]
        x = x[:, :, self.slice_start:self.slice_end]
        w = x * self.gelu(y)
        return w

model = SlicedGELUModel()
model.eval()

dummy_input = torch.randn(d0, d1, d2, dtype=torch.float16) #原始输入形状
output = model(dummy_input)
output_shape = model(dummy_input).shape
print(f"\n切片后输出形状：{output_shape}") #应显示 torch.Size（[1， 1024， 2560]）

dummy_input.numpy().tofile(f"./a.bin")
output.numpy().tofile(f"./b.bin")
