// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef ML_ASYNC_OP_MODULE_H
#define ML_ASYNC_OP_MODULE_H

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

typedef struct ML_ASYNC_OP_MODULE_TAG* ML_ASYNC_OP_MODULE_HANDLE;

MOCKABLE_FUNCTION(, ML_ASYNC_OP_MODULE_HANDLE, ml_async_op_module_create, EXECUTION_ENGINE_HANDLE, execution_engine, void*, ll_handle, COMMON_ASYNC_OP_MODULE_EXECUTE_ASYNC, ll_execute_async);
MOCKABLE_FUNCTION(, void, ml_async_op_module_destroy, ML_ASYNC_OP_MODULE_HANDLE, handle);

MOCKABLE_FUNCTION(, int, ml_async_op_module_open, ML_ASYNC_OP_MODULE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, ml_async_op_module_close, ML_ASYNC_OP_MODULE_HANDLE, handle);

// Sample functions which use ASYNC_OP

MOCKABLE_FUNCTION(, int, ml_async_op_module_execute_async, ML_ASYNC_OP_MODULE_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);

// Helper to provide a common interface for testing
MOCKABLE_FUNCTION(, COMMON_ASYNC_OP_MODULE_INTERFACE, ml_async_op_module_get_interface, ML_ASYNC_OP_MODULE_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /*ML_ASYNC_OP_MODULE_H*/
