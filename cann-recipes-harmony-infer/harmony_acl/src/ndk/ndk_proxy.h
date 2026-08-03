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

#ifndef HIAI_FOUNDATION_NDK_PROXY_NDK_PROXY_H
#define HIAI_FOUNDATION_NDK_PROXY_NDK_PROXY_H

#include <string>
#include "dynamic_load_helper.h"

namespace hiai {
class NDKProxy{
    public:
        static void* GetSymbol(std::string symbol);
    private:
        NDKProxy();
        ~NDKProxy();
        NDKProxy(const NDKProxy&) = delete;
        NDKProxy& operator=(const NDKProxy&) = delete;

        static NDKProxy* GetInstance();
    
    private:
        DynamicLoadHelper nncoreHelper_;
        DynamicLoadHelper hiaiHelper_;
};
}
#endif