#!/usr/bin/env/ python
import os
import sys
import json
import numpy as np

def gen_data_sample():
    output_path = sys.argv[1]

    inputx_data = np.random.uniform(-100, 100, [8, 2048]).astype(np.float16)
    inputx_data.tofile(os.path.join(output_path, "x.bin"))

    inputy_data = np.random.uniform(-100, 100, [8, 2048]).astype(np.float16)
    inputy_data.tofile(os.path.join(output_path, "y.bin"))

    golden_data = (inputx_data + inputy_data).astype(np.float16)
    golden_data.tofile(os.path.join(output_path, "z.bin"))

if __name__ == "__main__":
    gen_data_sample()

