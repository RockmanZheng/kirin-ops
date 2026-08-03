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
import numpy as np

#设置随机种子以便复现
np.random.seed(42)

#定义形状
input_shape = (64, 2048)
gamma_shape = (2048,)

#生成输入数据（float16）
input_data = np.random.randn(*input_shape).astype(np.float16)
input_data = input_data / np.max(np.abs(input_data))
#生成gamma参数（float16）
gamma_data = np.random.randn(*gamma_shape).astype(np.float16)
gamma_data = gamma_data / np.max(np.abs(gamma_data))

#模拟RMSNorm计算
#RMSNorm：output  = gamma * input / sqrt(mean(input * 2) + eps))
eps = 1e-5
input_squared = input_data ** 2
mean_squared = input_squared.mean(axis=-1, keepdims=True) #[64,1]
denominator = np.sqrt(mean_squared + eps) #[64,1]

#归一化并乘以gamma
output_data = (input_data / denominator) * gamma_data #[64, 2048]

#保存为bin文件（二进制，小端）
def save_as_bin(data, filename):
    data.tofile(filename)
    print(f"Saved {filename} with shape {data.shape}, dtype {data.dtype}")

#保存三个数据
save_as_bin(input_data, "x.bin")
save_as_bin(gamma_data, "gamma.bin")
save_as_bin(output_data, "y.bin")

print("All bin files generated!")