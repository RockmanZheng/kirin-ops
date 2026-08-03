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

#ifndef UTILS_SECURESTL_H
#define UTILS_SECURESTL_H

#include <memory>

namespace hiai {
template<typename T, typename... Ts>
std::shared_ptr<T> make_shared_nothrow(Ts&&... params)
{
    return std::shared_ptr<T>(new (std::nothrow) T(std::forward<Ts>(params)...));
}

template<typename T, typename... Ts>
std::unique_ptr<T> make_unique_nothrow(Ts&&... params)
{
    return std::unique_ptr<T>(new (std::nothrow) T(std::forward<Ts>(params)...));
}
}

#endif