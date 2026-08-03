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

#include "acl_data.h"

#include "acl/acl_base.h"
#include "utils/assert.h"

#include <new>

aclDataBuffer* aclCreateDataBuffer(void* data, size_t size)
{
    aclDataBuffer *dataBuffer = new (std::nothrow) aclDataBuffer(data, size);
    ACL_EXPECT_NOT_NULL_R(dataBuffer, nullptr);
    return dataBuffer;
}

aclError aclDestroyDataBuffer(const aclDataBuffer* dataBuffer)
{
    ACL_EXPECT_TRUE_R_WITH_LOG(dataBuffer != nullptr, ACL_ERROR_INVALID_PARAM, "the dataBuffer to destroy is nullptr");
    delete dataBuffer;
    dataBuffer = nullptr;
    return ACL_SUCCESS;
}

void* aclGetDataBufferAddr(const aclDataBuffer* dataBuffer)
{
    ACL_EXPECT_NOT_NULL_R(dataBuffer, nullptr);
    return dataBuffer->data;
}

size_t aclGetDataBufferSizeV2(const aclDataBuffer *dataBuffer)
{
    ACL_EXPECT_NOT_NULL(dataBuffer);
    return static_cast<size_t>(dataBuffer->size);
}