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

#include "ndk_proxy.h"

#include "utils/log.h"

namespace hiai {
NDKProxy::NDKProxy()
{
#ifdef __OHOS__
    hiaiHelper_.Init("/system/lib64/ndk/libhiai_foundation.so");
    nncoreHelper_.Init("/system/lib64/ndk/libneural_network_core.so");
#else
    hiaiHelper_.Init("libhiai_foundation.so");
    nncoreHelper_.Init("libneural_network_core.so");
#endif
}

NDKProxy* NDKProxy::GetInstance()
{
    static NDKProxy instance;
    return &instance;
}

NDKProxy::~NDKProxy()
{
}

void* NDKProxy::GetSymbol(std::string symbol)
{
    if (!GetInstance()->nncoreHelper_.IsValid() || !GetInstance()->hiaiHelper_.IsValid()) {
        ACL_LOGE("One or both helpers are invalid");
        return nullptr;
    }
    const std::string OH_NN_PREFIX = "OH_NN";
    if (symbol.substr(0, OH_NN_PREFIX.size()) == OH_NN_PREFIX) {
        return GetInstance()->nncoreHelper_.GetSymbol(symbol.c_str());
    }
    return GetInstance()->hiaiHelper_.GetSymbol(symbol.c_str());
}
} // namespace hiai