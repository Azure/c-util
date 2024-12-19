// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef HL_ASYNC_OP_MODULE_H
#define HL_ASYNC_OP_MODULE_H

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

typedef struct HL_ASYNC_OP_MODULE_TAG* HL_ASYNC_OP_MODULE_HANDLE;

MOCKABLE_FUNCTION(, HL_ASYNC_OP_MODULE_HANDLE, hl_async_op_module_create, EXECUTION_ENGINE_HANDLE, execution_engine, COMMON_OP_MODULE_INTERFACE_HANDLE, ll_async_op_module);
MOCKABLE_FUNCTION(, void, hl_async_op_module_destroy, HL_ASYNC_OP_MODULE_HANDLE, handle);

MOCKABLE_FUNCTION(, int, hl_async_op_module_open, HL_ASYNC_OP_MODULE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, hl_async_op_module_close, HL_ASYNC_OP_MODULE_HANDLE, handle);

// Sample functions which use ASYNC_OP

MOCKABLE_FUNCTION(, int, hl_async_op_module_execute_async, HL_ASYNC_OP_MODULE_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);

// Helper to provide a common interface for testing
MOCKABLE_FUNCTION(, COMMON_ASYNC_OP_MODULE_INTERFACE, hl_async_op_module_get_interface, HL_ASYNC_OP_MODULE_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /*HL_ASYNC_OP_MODULE_H*/
