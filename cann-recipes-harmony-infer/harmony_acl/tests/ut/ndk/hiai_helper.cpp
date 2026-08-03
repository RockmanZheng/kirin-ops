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

#include "hiai_foundation/hiai_helper.h"

static const char* version = "106.630.020.010";

const char* HMS_HiAI_GetVersion()
{
    return version;
}

HiAI_Compatibility HMS_HiAICompatibility_CheckFromFile(const char* file)
{
    (void) file;
    return HIAI_COMPATIBILITY_COMPATIBLE;
}

HiAI_Compatibility HMS_HiAICompatibility_CheckFromBuffer(const void* data, size_t size)
{
    (void) data;
    (void) size;
    return HIAI_COMPATIBILITY_COMPATIBLE;
}