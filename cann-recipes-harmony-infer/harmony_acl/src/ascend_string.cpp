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

#include "graph/ascend_string.h"

#include "utils/log.h"

namespace ge {
AscendString::AscendString(const char_t* const name)
{
    if (name != nullptr) {
        name_ = std::make_shared<std::string>(name);
        if (name_ == nullptr) {
            ACL_LOGE("[New][String]AscendString[%s] make shared failed", name);
        }
        
    }
}

AscendString::AscendString(const char_t* const name, size_t length)
{
    if (name != nullptr) {
        name_ = std::make_shared<std::string>(name, length);
        if (name_ == nullptr) {
            ACL_LOGE("[New][String]AscendString make shared failed, length=%zu.", length);
        }
    }
}

const char_t* AscendString::GetString() const
{
    if (name_ == nullptr) {
        const static char* empty_value = "";
        return empty_value;
    }
    return (*name_).c_str();
}

size_t AscendString::GetLength() const
{
    if (name_ == nullptr) {
        return 0UL;
    }
    return (*name_).length();
}

size_t AscendString::Hash() const
{
    if (name_ == nullptr) {
        const static size_t kEmptyStringHash = std::hash<std::string>()("");
        return kEmptyStringHash;
    }
    return std::hash<std::string>()(*name_);
}

bool AscendString::operator<(const AscendString& d) const
{
    if ((name_ == nullptr) && (d.name_ == nullptr)) {
        return false;
    } else if (name_ == nullptr) {
        return true;
    } else if (d.name_ == nullptr) {
        return false;

    } else {
        return (*name_) < (*(d.name_));
    }
}

bool AscendString::operator>(const AscendString& d) const
{
    if ((name_ == nullptr) && (d.name_ == nullptr)) {
        return false;
    } else if (name_ == nullptr) {
        return false;
    } else if (d.name_ == nullptr) {
        return true;
    } else {
        return (*name_) > (*(d.name_));
    }
}

bool AscendString::operator==(const AscendString& d) const
{
    if ((name_ == nullptr) && (d.name_ == nullptr)) {
        return true;
    } else if (name_ == nullptr) {
        return false;
    } else if (d.name_ == nullptr) {
        return false;
    } else {
        return (*name_) == (*(d.name_));
    }
}

bool AscendString::operator<=(const AscendString& d) const
{
    if (name_ == nullptr) {
        return true;
    } else if (d.name_ == nullptr) {
        return false;
    } else {
        return (*name_) <= (*(d.name_));
    }
}

bool AscendString::operator>=(const AscendString& d) const
{
    if (d.name_ == nullptr) {
        return true;
    } else if (name_ == nullptr) {
        return false;
    } else {
        return (*name_) >= (*(d.name_));
    }
}

bool AscendString::operator!=(const AscendString& d) const
{
    if ((name_ == nullptr) && (d.name_ == nullptr)) {
        return false;
    } else if (name_ == nullptr) {
        return true;
    } else if (d.name_ == nullptr) {
        return true;
    } else {
        return (*name_) != (*(d.name_));
    }
}

bool AscendString::operator==(const char_t* const d) const
{
    if ((name_ == nullptr) && (d == nullptr)) {
        return true;
    } else if (name_ == nullptr || d == nullptr) {
        return false;
    } else {
        return (strcmp((*name_).c_str(), d) == 0);
    }
}

bool AscendString::operator!=(const char_t* const d) const
{
    if ((name_ == nullptr) && (d == nullptr)) {
        return false;
    } else if (name_ == nullptr || d == nullptr) {
        return true;
    } else {
        return (strcmp((*name_).c_str(), d) != 0);
    }
}

size_t AscendString::Find(const AscendString& ascend_string) const
{
    if ((name_ == nullptr) || (ascend_string.name_ == nullptr)) {
        return std::string::npos;
    }
    return name_->find(*(ascend_string.name_));
}
} //namespace ge