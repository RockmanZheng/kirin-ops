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
from plistlib import FMT_BINARY
import sys
import numpy as np
import json
import codecs
from functools import reduce
import cv2

#固定随机种子
np.random.seed(42)

x = np.random.randint(1, 256, (1, 763, 1024, 3), dtype=np.uint8)
x.tofile(f"./x.bin")

x = np.transpose(x,(0,3,1,2)).astype(np.uint8)
x = x.astype(np.float16)
gray = (0.299 * x[:, 0, :, :] + 0.587 * x[:, 1, :, :] + 0.114 * x[:, 2, :, :])
#移除单维度的轴
gray = np.squeeze(gray)
sobel_x = cv2.Sobel(gray.astype(np.float32), cv2.CV_32F, 1, 0, ksize=3).astype(np.float16)
sobel_y = cv2.Sobel(gray.astype(np.float32), cv2.CV_32F, 0, 1, ksize=3).astype(np.float16)

sobel_x1 = np.abs(sobel_x).astype(np.float16)
sobel_y1 = np.abs(sobel_y).astype(np.float16)

#裁剪图像，去掉边界
x1 = sobel_x1[1:-1, 1:-1]
y1 = sobel_y1[1:-1, 1:-1]

y2 = x1 + y1
#先将y1的值限制在0到255之间， 否则会取模
y2_clipped = np.clip(y2, 0, 255)
y = np.ceil(y2_clipped).astype(np.uint8)
y.tofile(f"./y.bin")