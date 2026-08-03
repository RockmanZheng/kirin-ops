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

#ifndef INFRA_BASE_DYNAMIC_LOAD_HELPER_H
#define INFRA_BASE_DYNAMIC_LOAD_HELPER_H
#include <string>
#include <mutex>
#include <map>

namespace hiai
{
    class DynamicLoadHelper
    {
    public:
        DynamicLoadHelper();
        ~DynamicLoadHelper();

        bool Init(const std::string &file, bool isNeedClose = true);
        void Deinit();

        bool IsValid();

        template <typename T>
        T* GetFunction(const std::string &symbol)
        {
            auto funcPtr = GetSymbol(symbol);
            return reinterpret_cast<T*>(funcPtr);
        };

        void* GetSymbol(const std::string &symbol);
    private:
        bool isNeedClose_{true};
        void *handle_{nullptr};
        std::mutex funcMapMutex_;
        std::map<std::string, void*> funcMap_;
    };
} // namespace hiai
#endif // INFRA_BASE_DYNAMIC_LOAD_HELPER_H