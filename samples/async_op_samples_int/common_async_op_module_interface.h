// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef COMMON_ASYNC_OP_MODULE_INTERFACE_H
#define COMMON_ASYNC_OP_MODULE_INTERFACE_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "c_util/async_op.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define COMMON_ASYNC_OP_MODULE_RESULT_VALUES \
    COMMON_ASYNC_OP_MODULE_OK, \
    COMMON_ASYNC_OP_MODULE_CANCELED, \
    COMMON_ASYNC_OP_MODULE_ERROR_CAN_RETRY, \
    COMMON_ASYNC_OP_MODULE_ERROR \

MU_DEFINE_ENUM(COMMON_ASYNC_OP_MODULE_RESULT, COMMON_ASYNC_OP_MODULE_RESULT_VALUES);

// Callback from an async operation
typedef void(*COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK)(void* context, COMMON_ASYNC_OP_MODULE_RESULT result);

// Async operation
typedef int(*COMMON_ASYNC_OP_MODULE_EXECUTE_ASYNC)(void* module_context, uint32_t complete_in_ms, THANDLE(ASYNC_OP)* async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK callback, void* context);

#ifdef __cplusplus
}
#endif

#endif /*COMMON_ASYNC_OP_MODULE_INTERFACE_H*/
