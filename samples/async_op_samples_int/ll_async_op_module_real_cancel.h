// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LL_ASYNC_OP_MODULE_REAL_CANCEL_H
#define LL_ASYNC_OP_MODULE_REAL_CANCEL_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/execution_engine.h"
#include "c_pal/thandle.h"

#include "c_util/async_op.h"

#include "common_async_op_module_interface.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct LL_ASYNC_OP_MODULE_REAL_CANCEL_TAG* LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE;

MOCKABLE_FUNCTION(, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, ll_async_op_module_real_cancel_create, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, ll_async_op_module_real_cancel_destroy, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle);

MOCKABLE_FUNCTION(, int, ll_async_op_module_real_cancel_open, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle);
MOCKABLE_FUNCTION(, void, ll_async_op_module_real_cancel_close, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle);

// Sample functions which use ASYNC_OP

MOCKABLE_FUNCTION(, int, ll_async_op_module_real_cancel_execute_async, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);

// Helpers for test

MOCKABLE_FUNCTION(, void, ll_async_op_module_real_cancel_next_call_completes_synchronously, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle, bool, is_synchronous);
MOCKABLE_FUNCTION(, void, ll_async_op_module_real_cancel_set_report_retry_result_count, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle, uint32_t, retry_result_count);
MOCKABLE_FUNCTION(, void, ll_async_op_module_real_cancel_set_next_async_result, LL_ASYNC_OP_MODULE_REAL_CANCEL_HANDLE, handle, COMMON_ASYNC_OP_MODULE_RESULT, next_result);

#ifdef __cplusplus
}
#endif

#endif /*LL_ASYNC_OP_MODULE_REAL_CANCEL_H*/
