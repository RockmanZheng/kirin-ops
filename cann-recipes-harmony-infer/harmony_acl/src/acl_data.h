/* Copyright (C) 2020-2026. Huawei Technologies Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Apache License Version 2.0.
 * You may not use this file except in compliance with the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Apache License for more details at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef HIAI_ACL_DATA_H
#define HIAI_ACL_DATA_H

#include <cstdint>

struct aclDataBuffer {
    aclDataBuffer(void* dataIn, const uint64_t len) : data(dataIn), size(len) {}
    void* data;
    uint64_t size;
};

#endif