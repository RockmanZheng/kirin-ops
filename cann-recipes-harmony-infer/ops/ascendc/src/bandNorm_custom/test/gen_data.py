#!/usr/bin/env/ python

import numpy as np
import torch

def cal_band_power(x1, x2):
    eps = torch.finfo(torch.float32).eps
    x1_square = np.square(x1)
    x2_square = np.square(x2)
    power_1 = x1_square + x2_square
    power_2 = np.sum(power_1, axis=1)
    power = np.sqrt(power_2 + eps)
    return power

if __name__ == "__main__":

    input_shape = [2, 121, 47]
    x1_data = np.random.randn(*input_shape).astype(np.float16)
    x2_data = np.random.randn(*input_shape).astype(np.float16)
    power = cal_band_power(x1_data, x2_data)

    x1_data.tofile("./x1.bin")
    x2_data.tofile("./x2.bin")
    power.tofile("./target.bin")

