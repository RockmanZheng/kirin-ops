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
"""生成 GatherDequantInt8 算子的输入数据与 golden 输出。

落盘文件（与 create_onnx.py 输入/输出顺序对应）：
    table.bin       uint8   [V, E]
    indices.bin     int32   [1, N]
    scale.bin       fp16    [V]
    zero_point.bin  fp16    [V]
    output.bin      fp16    [1, N, E]   golden（fp16 反量化路径，与 device kernel 对齐）
"""
import numpy as np

VOCAB = 1024
EMB_DIM = 256
SEQ_LEN = 30


def quantize_embedding(weight: np.ndarray):
    """per-row 非对称 uint8 量化，返回 q[V,E]uint8, scale[V]fp16, zp[V]fp16。"""
    weight = weight.astype(np.float64)
    vmin = weight.min(axis=1)
    vmax = weight.max(axis=1)
    scale = (vmax - vmin) / 255.0
    scale[scale == 0] = 1e-8
    zp = -vmin / scale
    q = np.clip(np.round(weight / scale[:, None] + zp[:, None]), 0, 255).astype(np.uint8)
    return q, scale.astype(np.float16), zp.astype(np.float16)


def gather_dequant_int8_ref(table_u8, indices, scale_f16, zp_f16):
    """golden：fp16 计算路径，y[i,:] = (half(table[idx,:]) - zp[idx]) * scale[idx]。"""
    idx = indices.reshape(-1).astype(np.int64)
    q = table_u8[idx].astype(np.float16)
    z = zp_f16[idx].astype(np.float16)[:, None]
    s = scale_f16[idx].astype(np.float16)[:, None]
    y = ((q - z) * s).astype(np.float16)
    return y.reshape(*indices.shape, table_u8.shape[1])


if __name__ == "__main__":
    np.random.seed(42)
    weight = (np.random.randn(VOCAB, EMB_DIM).astype(np.float32) * 0.1)
    table, scale, zero_point = quantize_embedding(weight)
    indices = np.random.randint(0, VOCAB, size=(1, SEQ_LEN)).astype(np.int32)

    out = gather_dequant_int8_ref(table, indices, scale, zero_point)

    table.tofile("./table.bin")
    indices.tofile("./indices.bin")
    scale.tofile("./scale.bin")
    zero_point.tofile("./zero_point.bin")
    out.tofile("./output.bin")
    print(f"[gen_data] V={VOCAB} E={EMB_DIM} N={SEQ_LEN}")
    print(f"[gen_data] table{table.shape}u8 indices{indices.shape}i32 "
          f"scale/zp[{VOCAB}]fp16 -> output{out.shape}fp16")
