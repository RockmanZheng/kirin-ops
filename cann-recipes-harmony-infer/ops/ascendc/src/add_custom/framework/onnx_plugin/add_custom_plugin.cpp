/* Copyright (C) 2020-2021. Huawei Technologies Co., Ltd. All
rights reserved.
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

#include "register/register.h"
#include "json.hpp"

namespace domi {
// Onnx ParseParams
Status ParseParamAddCustom(const ge::Operator& op_src, ge::Operator& op_dest) {
    // To do: Implement the operator plugin by referring to the Onnx Operator Development Guide.
    ge::AscendString attrs_string;
    if (ge::GRAPH_SUCCESS == op_src.GetAttr("attribute", attrs_string)) {
        nlohmann::json attrs = nlohmann::json::parse(attrs_string.GetString());
        for (nlohmann::json attr : attrs["attribute"]) {
            if (attr["name"] == "bias") {
                int64_t bias  = attr["i"];
                op_dest.SetAttr("bias", bias);
            }
        }
    }
    return SUCCESS;
}

// 将AddCustom向ONNX框架注册，将ONNX中的算子和实现的自定义算子关联起来。
// OriginOpType为ONNX中算子的类型，可以不与REGISTER_CUSTOM_OP中的类型相同。
REGISTER_CUSTOM_OP("AddCustom")     // Set the registration name of operator
    .FrameworkType(ONNX)   // Operator name with the original framework
    .OriginOpType("ai.onnx::11::AddCustom")      // Set the original frame type of the operator
    .ParseParamsByOperatorFn(ParseParamAddCustom); // Registering the callback function for parsing operator parameters
}  // namespace domi
