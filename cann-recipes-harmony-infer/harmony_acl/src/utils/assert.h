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

#ifndef UTILS_ASSERT_H
#define UTILS_ASSERT_H

#include "log.h"

///////////////////////////////////////////////////
#define ACL_EXPECT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            ACL_LOGE(#ptr "null, return FAIL."); \
            return ACL_ERROR_FAILURE; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_NOT_NULL_VOID(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            ACL_LOGE(#ptr "null, return FAIL."); \
            return; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_NOT_NULL_R(ptr, ret) \
    do { \
        if ((ptr) == nullptr) { \
            ACL_LOGE(#ptr "null, return FAIL."); \
            return ret; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            ACL_LOGE(#expr "false, return FAIL."); \
            return ACL_ERROR_FAILURE; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_TRUE_VOID(expr) \
    do { \
        if (!(expr)) { \
            ACL_LOGE(#expr "false, return."); \
            return; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_TRUE_WITH_LOG(expr, format, ...) \
    do { \
        if (!(expr)) { \
            ACL_LOGE((format), ##__VA_ARGS__); \
            return ACL_ERROR_FAILURE; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_TRUE_WITH_LOG(expr, format, ...) \
    do { \
        if (!(expr)) { \
            ACL_LOGE((format), ##__VA_ARGS__); \
            return ACL_ERROR_FAILURE; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_TRUE_R(expr, ret) \
    do { \
        if (!(expr)) { \
            ACL_LOGE(#expr "false, return %s.", #ret); \
            return ret; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_TRUE_R_WITH_LOG(expr, ret, format, ...) \
    do { \
        if (!(expr)) { \
            ACL_LOGE((format), ##__VA_ARGS__); \
            return ret; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_EXEC(func) \
    do { \
        aclError status = func; \
        if (status != ACL_SUCCESS) { \
            ACL_LOGE(#func "exec FAIL."); \
            return status; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_EXEC_R(func, ret) \
    do { \
        aclError status = func; \
        if (status != ACL_SUCCESS) { \
            ACL_LOGE(#func "exec FAIL."); \
            return ret; \
        } \
    } while (0)

///////////////////////////////////////////////////
#define ACL_EXPECT_EXEC_VOID(func) \
    do { \
        aclError status = func; \
        if (status != ACL_SUCCESS) { \
            ACL_LOGE(#func "exec FAIL."); \
            return; \
        } \
    } while (0)

#endif
