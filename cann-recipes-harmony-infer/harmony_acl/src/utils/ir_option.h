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

#ifndef UTILS_IR_OPTION_H
#define UTILS_IR_OPTION_H

namespace ge {
namespace ir_option {
static const char* const INPUT_SHAPE = "input_shape";
static const char* const EXEC_DEVICE_CONFIG_PATH = "ge.exec.exec_device_config_path";
static const char* const EXEC_DISABLE_REUSED_MEMORY = "ge.exec.disableReuseMemory";
static const char* const PRECISION_MODE_V2 = "ge.exec.precision_mode_v2";
}
}

#endif