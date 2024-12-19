// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_H
#define ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_H

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

typedef struct ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_TAG* ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE;

MOCKABLE_FUNCTION(, ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE, ml_async_op_module_with_async_chain_create, EXECUTION_ENGINE_HANDLE, execution_engine, COMMON_OP_MODULE_INTERFACE_HANDLE, ll_async_op_module);
MOCKABLE_FUNCTION(, void, ml_async_op_module_with_async_chain_destroy, ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE, handle);

MOCKABLE_FUNCTION(, int, ml_async_op_module_with_async_chain_open, ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE, handle);
MOCKABLE_FUNCTION(, void, ml_async_op_module_with_async_chain_close, ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE, handle);

// Sample functions which use ASYNC_OP

MOCKABLE_FUNCTION(, int, ml_async_op_module_with_async_chain_execute_async, ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, COMMON_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);

// Helper to provide a common interface for testing
MOCKABLE_FUNCTION(, COMMON_ASYNC_OP_MODULE_INTERFACE, ml_async_op_module_with_async_chain_get_interface, ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE, handle);

#ifdef __cplusplus
}
#endif

#endif /*ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_H*/
