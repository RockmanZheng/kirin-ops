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

def pack_int4_to_uint8(arr_float16: np.ndarray) -> np.ndarray:

    arr_i8 = arr_float16.astype(np.int8)
    arr_i8 = np.clip(arr_i8, -8, 7)

    nibbles = (arr_i8 & 0xF).astype(np.uint8)

    flat = nibbles.ravel()
    L = flat.size

    if L % 2 != 0:
        flat = np.pad(flat, (0,1), constant_values=0)
        L += 1

    low = flat[0::2]
    high = flat[1::2]
    packed = (high << 4) | low #uint8

    return packed.astype(np.uint8)

def unpack_uint8_to_int4(packed: np.ndarray) -> np.ndarray:
    low = (packed & 0x0F).astype(np.uint8)
    high = ((packed >> 4) & 0x0F).astype(np.uint8)

    def to_int4_signed(x):
        x = x.astype(np.int8)
        x[x >= 8] -= 16
        return x
    low_s = to_int4_signed(low)
    high_s = to_int4_signed(high)

    out = np.empty((packed.size * 2,), dtype=np.int8)
    out[0::2] = low_s
    out[1::2] = high_s
    return out

def dequantize_weight_group32(weight_flat_fp16, scale_fp16, offset_fp16, K, N, group_size=32):

    total = K * N
    assert weight_flat_fp16.dtype == np.float16
    assert scale_fp16.dtype == np.float16
    assert offset_fp16.dtype == np.float16
    assert total % group_size == 0, "K*N必须能被group_size整除"
    assert scale_fp16.size == total // group_size
    assert offset_fp16.size == total // group_size
    assert weight_flat_fp16.size == total

    w = weight_flat_fp16.astype(np.float16)
    s = scale_fp16.astype(np.float32).reshape(K // group_size, -1)
    s = np.repeat(s, group_size, axis=0)
    o = offset_fp16.astype(np.float32).reshape(K // group_size, -1)
    o = np.repeat(o, group_size, axis=0)

    w_deq = ((w + o) * s).reshape(K,N) #float32

    return w_deq

def matmul_input_weight_dequant(
        inputA_fp16, weight_flat_fp16, scale_fp16, offset_fp16, group_size=32
):

    assert inputA_fp16.dtype == np.float16
    M, K = inputA_fp16.shape
    total = weight_flat_fp16.size
    assert total % K == 0, "weight_flat长度必须能被K整除以确定 N"
    N = total // K

    w_deq_f32 = dequantize_weight_group32(weight_flat_fp16, scale_fp16, offset_fp16, K, N, group_size)

    out_f32 = inputA_fp16.astype(np.float32) @ w_deq_f32 #(M,N)
    return out_f32.astype(np.float16)

if __name__ == "__main__":
    M, K, N = 1, 1280, 151936
    group_size =32
    assert (K * N) % group_size == 0
    np.random.seed(42)
    inputA = np.random.rand(M, K).astype(np.float16)

    int_data = np.random.randint(-8, 8, size=(K, N))

    weight_flat = int_data.astype(np.float16)

    scale = np.random.rand((K*N)//group_size,).astype(np.float16) * 0.01
    offset = np.ones((K*N)//group_size,).astype(np.float16)

    out = matmul_input_weight_dequant(inputA, weight_flat, scale, offset, group_size)

    packed_uint8 = pack_int4_to_uint8(weight_flat)

    unpacked = unpack_uint8_to_int4(packed_uint8)
    print("match:", np.all(unpacked[:weight_flat.size] == weight_flat.astype(np.int8).ravel()))

    weight = packed_uint8
    inputA.tofile("./inputA.bin")
    weight.tofile("./weight.bin")
    scale.tofile("./scale.bin")
    offset.tofile("./offset.bin")
    out.tofile("./output.bin")