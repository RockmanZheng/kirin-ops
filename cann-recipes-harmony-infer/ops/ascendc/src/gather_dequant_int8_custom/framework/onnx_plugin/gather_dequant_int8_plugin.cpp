/* Copyright (c) 2025 Huawei Technologies Co., Ltd. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* GatherDequantInt8 —— ONNX 前端适配插件
 *
 * 把 ONNX 图里 op_type=GatherDequantInt8 的自定义节点映射到 GE 自定义算子，
 * 使 omg --framework=5 能解析并入图（对应《自定义算子开发》7.2 ONNX 框架适配）。
 *
 * ONNX 侧节点约定（domain 建议用自定义域 "custom"，opset 任意）：
 *   inputs : table(uint8 [V,E]), indices(int32 [...]), scale(fp16 [V]), zero_point(fp16 [V])
 *   outputs: y(fp16 [...,E])
 * 无属性，输入/输出顺序与 GE 原型一致，故直接走 AutoMappingByOpFn。
 */
#include "register/register.h"

namespace domi {
REGISTER_CUSTOM_OP("GatherDequantInt8")
    .FrameworkType(ONNX)
    .OriginOpType({"GatherDequantInt8",
                   "custom::GatherDequantInt8",
                   "custom::1::GatherDequantInt8",
                   "ai.onnx::1::GatherDequantInt8",
                   "ai.onnx::9::GatherDequantInt8",
                   "ai.onnx::10::GatherDequantInt8",
                   "ai.onnx::11::GatherDequantInt8",
                   "ai.onnx::12::GatherDequantInt8",
                   "ai.onnx::13::GatherDequantInt8",
                   "ai.onnx::14::GatherDequantInt8",
                   "ai.onnx::15::GatherDequantInt8",
                   "ai.onnx::16::GatherDequantInt8"})
    .ParseParamsByOperatorFn(AutoMappingByOpFn);
}  // namespace domi
