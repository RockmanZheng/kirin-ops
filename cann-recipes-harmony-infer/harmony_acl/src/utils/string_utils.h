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

#ifndef UTILS_STRING_UTILS_H
#define UTILS_STRING_UTILS_H

#include <string>
#include <cctype>

class StringUtils {
public:
    static std::string Trim(const std::string& str)
    {
        if (str.empty()) {
            return str;
        }

        auto start = str.find_first_not_of("\t\n\v\f\r");
        if (start == std::string::npos) {
            return "";
        }

        auto end = str.find_last_not_of("\t\n\v\f\r");
        return str.substr(start, end - start + 1);
    }

    static std::vector<std::string> Split(const std::string& str, const char delimiter)
    {
        std::vector<std::string> results;
        if (str.empty()) {
            return results;
        }

        char tmp[2] = {delimiter, '\0'};
        std::string delimStr = std::string(tmp);
        size_t start = 0;
        size_t end = str.find(delimStr);
        while (end != std::string::npos) {
            results.push_back(str.substr(start, end - start));
            start = end + delimStr.length();
            end = str.find(delimStr, start);
        }
        results.push_back(str.substr(start));
        return results;
    }
};

#endif