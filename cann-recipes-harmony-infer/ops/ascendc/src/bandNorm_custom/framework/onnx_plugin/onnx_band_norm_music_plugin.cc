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
// tf ParseParams
Status ParseParamBandNormMusic(const ge::Operator& op_src, ge::Operator& op_dest) {
    // To do: Implement the operator plugin by referring to the Onnx Operator Development Guide.
    //ge::AscendString attrs_string;
    //if (ge::GRAPH_SUCCESS == op_src.GetAttr("attr", attrs_string)) {
        // nlohmann::json attrs = nlohmann::json::parse(attrs_string.GetString());
        // for (nlohmann::json attr : attrs["attr"]) {
            // std::cout << "name:" << attr["key"] << std::endl;
            // if (attr["key"] == "T" && attr["value"]["type"] == 1) {
                // int64_t negative_slope0  = 3;
                // op_dest.SetAttr("T", negative_slope0);
            // }
        // }
    // }
    return SUCCESS;
}

REGISTER_CUSTOM_OP("BandNormMusic")
    .FrameworkType(ONNX)   // type: CAFFE, TENSORFLOW
    .OriginOpType("ai.onnx::11::BandNormMusic")      // name in tf module
    .ParseParamsByOperatorFn(AutoMappingByOpFn);
}  // namespace domi
