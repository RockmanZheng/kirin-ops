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
// #include "nlohmann/json.hpp"

namespace domi {
// Onnx ParseParams
Status ParseParamSliceGeluMul(const ge::Operator& op_src, ge::Operator& op_dest) {
    // To do: Implement the operator plugin by referring to the Onnx Operator Development Guide.
    // ge::AscendString attrs_string;
    // if (ge::GRAPH_SUCCESS == op_src.GetAttr("attribute", attrs_string)) {
        // nlohmann::json attrs = nlohmann::json::parse(attrs_string.GetString());
        // for (nlohmann::json attr : attrs["attribute"]) {
            // if (attr["name"] == "negative_slope0" && attr["type"] == 2) {
                // int64_t negative_slope0  = attr["i"];
                // op_dest.SetAttr("negative_slope0", negative_slope0);
            // }
            // if (attr["name"] == "negative_slope4" && attr["type"] == 7) {
                // std::vector<int64_t> attrvalue;
                // for (auto value : attr["ints"]) {
                    // attrvalue.push_back(value);
                // }
                // op_dest.SetAttr("negative_slope4", attrvalue);
            // }
        // }
    // }
    return SUCCESS;
}

// register SliceGeluMul op info to GE
REGISTER_CUSTOM_OP("SliceGeluMul")     // Set the registration name of operator
    .FrameworkType(ONNX)   // Operator name with the original framework
    .OriginOpType("ai.onnx::11::SliceGeluMul")      // Set the original frame type of the operator
    .ParseParamsByOperatorFn(AutoMappingByOpFn); // Registering the callback function for parsing operator parameters
}  // namespace domi
