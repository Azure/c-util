// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HL_ASYNC_OP_MODULE_CANCEL_ALL_H
#define HL_ASYNC_OP_MODULE_CANCEL_ALL_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
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

typedef struct HL_ASYNC_OP_MODULE_CANCEL_ALL_TAG* HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE;

MOCKABLE_FUNCTION(, HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE, hl_async_op_module_cancel_all_create, EXECUTION_ENGINE_HANDLE, execution_engine, void*, ll_handle, COMMON_ASYNC_OP_MODULE_EXECUTE_ASYNC, ll_execute_async);
MOCKABLE_FUNCTION(, void, hl_async_op_module_cancel_all_destroy, HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE, handle);

MOCKABLE_FUNCTION(, int, hl_async_op_module_cancel_all_open, HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE, handle);
MOCKABLE_FUNCTION(, void, hl_async_op_module_cancel_all_close, HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE, handle);

// Sample functions which use ASYNC_OP

MOCKABLE_FUNCTION(, int, hl_async_op_module_cancel_all_execute_async, HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE, handle, uint32_t, complete_in_ms, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);

// Helper to provide a common interface for testing
MOCKABLE_FUNCTION(, COMMON_ASYNC_OP_MODULE_INTERFACE, hl_async_op_module_cancel_all_get_interface, HL_ASYNC_OP_MODULE_CANCEL_ALL_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /*HL_ASYNC_OP_MODULE_CANCEL_ALL_H*/
