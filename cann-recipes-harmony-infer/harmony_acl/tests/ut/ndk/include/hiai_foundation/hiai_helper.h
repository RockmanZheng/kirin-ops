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

/**
 * @addtogroup HiAIFoundation
 * @{
 *
 * @brief Provides APIs for HiAI Foundation model inference.
 *
 * @since 4.1.0(11)
 * @deprecated since 18
 * @useinstead {@link CANNKit/hiai_helper.h}
 */

/**
 * @file hiai_helper.h
 *
 * @brief Defines the APIs for querying the HiAI Foundation version and checking the model compatibility.
 *
 * @library libhiai_foundation.so
 * @syscap SystemCapability.AI.HiAIFoundation
 * @kit HiAIFoundationKit
 * @since 4.1.0(11)
 * @deprecated since 18
 * @useinstead {@link CANNKit/hiai_helper.h}
 */
#ifndef HIAI_FOUNDATION_HELPER_H
#define HIAI_FOUNDATION_HELPER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Obtains the HiAI Foundation version number.
 * @return Returns the HiAI Foundation version number if the operation is successful; returns a null pointer otherwise.
 * @since 4.1.0(11)
 * @deprecated since 18
 * @useinstead {@link CANNKit/HMS_HiAI_GetVersion}
 */
const char* HMS_HiAI_GetVersion(void);

/**
 * @brief Model compatibility.
 * @since 4.1.0(11)
 * @deprecated since 18
 * @useinstead {@link CANNKit/HiAI_Compatibility}
 */
typedef enum {
    /** Model compatible */
    HIAI_COMPATIBILITY_COMPATIBLE = 0,
    /** Model incompatible */
    HIAI_COMPATIBILITY_INCOMPATIBLE = 1
} HiAI_Compatibility;

/**
 * @brief Queries the compatibility of the model stored in the file.
 *
 * @param file Path and name of the model file. The value cannot be empty, and the process must have the permission
 * to access the model file. Otherwise, a message is returned indicating that the model is incompatible.
 * @return Returns {@link HiAI_Compatibility} if the operation is successful; returns a message indicating that the
 * model is incompatible otherwise.
 * @since 4.1.0(11)
 * @deprecated since 18
 * @useinstead {@link CANNKit/HMS_HiAICompatibility_CheckFromFile}
 */
HiAI_Compatibility HMS_HiAICompatibility_CheckFromFile(const char* file);

/**
 * @brief Queries the compatibility of the model stored in the memory.
 *
 * @param data Model data address. The value cannot be null. Otherwise, a message indicating that the model is
 * incompatible is returned.
 * @param size Model data size. The value cannot be null. Otherwise, a message indicating that the model is
 * incompatible is returned.
 * @return Returns {@link HiAI_Compatibility} if the operation is successful; returns a message indicating that the
 * model is incompatible otherwise.
 * @since 4.1.0(11)
 * @deprecated since 18
 * @useinstead {@link CANNKit/HMS_HiAICompatibility_CheckFromBuffer}
 */
HiAI_Compatibility HMS_HiAICompatibility_CheckFromBuffer(const void* data, size_t size);

#ifdef __cplusplus
}
#endif

/** @} */
#endif // HIAI_FOUNDATION_HELPER_H
