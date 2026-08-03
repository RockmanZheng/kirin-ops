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
 
#include "dynamic_load_helper.h"

#include <dlfcn.h>
#include <linux/limits.h>

#include <cstring>

#include "utils/log.h"

namespace hiai {
DynamicLoadHelper::DynamicLoadHelper()
{
}

DynamicLoadHelper::~DynamicLoadHelper()
{
    handle_ = nullptr;
}

bool DynamicLoadHelper::Init(const std::string &file, bool isNeedClose)
{
    std::lock_guard<std::mutex> lock(funcMapMutex_);
    if (handle_ != nullptr) {
        ACL_LOGE("handle is not null, file: %s", file.c_str());
        return false;
    }

    if (file.empty()) {
        ACL_LOGE("file is empty");
        return false;
    }
    char path[PATH_MAX] = {0x00};
    if (file.find("/") == std::string::npos || file.find("\\") == std::string::npos) {
        if (strncpy(path, file.c_str(), PATH_MAX) != path) {
            ACL_LOGE("invalid file");
            return false;
        }
    } else {
        if (realpath(file.c_str(), path) == nullptr) {
            constexpr int32_t errorBufferLen = 4096;
            char errorBuffer[errorBufferLen] = {0};
            (void) strerror_r(errno, errorBuffer, errorBufferLen);
            ACL_LOGE("realpath failed, file: %s, error: %s", file.c_str(), errorBuffer);
            return false;
        }
    }

    handle_ = dlopen(path, 2);
    if (handle_ == nullptr) {
        ACL_LOGE("dlopen failed, file: %s, error: %s", path, dlerror());
        return false;
    }
    isNeedClose_ = isNeedClose;
    return true;
}

void DynamicLoadHelper::Deinit()
{
    std::lock_guard<std::mutex> lock(funcMapMutex_);
    if (handle_ == nullptr) {
        return;
    }
    if (isNeedClose_ && dlclose(handle_) != 0) {
        ACL_LOGE("dlclose failed.");
    }
    handle_ = nullptr;
}

bool DynamicLoadHelper::IsValid()
{
    return handle_ != nullptr;
}

void* DynamicLoadHelper::GetSymbol(const std::string &symbol)
{
    std::lock_guard<std::mutex> lock(funcMapMutex_);
    if (handle_ == nullptr) {
        ACL_LOGE("handle is null, symbol: %s", symbol.c_str());
        return nullptr;
    }
    std::map<std::string, void*>::const_iterator it = funcMap_.find(symbol);
    if (it != funcMap_.cend()) {
        return it->second;
    }
    auto funcPtr = dlsym(handle_, symbol.c_str());
    if (funcPtr == nullptr) {
        ACL_LOGE("dlsym failed, symbol: %s, error: %s", symbol.c_str(), dlerror());
    }
    funcMap_[symbol] = funcPtr;
    return funcPtr;
}
} // namespace hiai