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

#include "ml_async_op_module.h"
#include "ml_async_op_module_with_async_chain.h"
#include "ml_async_op_module_with_retries.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct HL_ASYNC_OP_MODULE_TAG* HL_ASYNC_OP_MODULE_HANDLE;

#define HL_ASYNC_OP_MODULE_RESULT_VALUES \
    HL_ASYNC_OP_MODULE_OK, \
    HL_ASYNC_OP_MODULE_CANCELED, \
    HL_ASYNC_OP_MODULE_ERROR \

MU_DEFINE_ENUM(HL_ASYNC_OP_MODULE_RESULT, HL_ASYNC_OP_MODULE_RESULT_VALUES);

typedef void(*HL_ASYNC_OP_MODULE_EXECUTE_CALLBACK)(void* context, HL_ASYNC_OP_MODULE_RESULT result);

MOCKABLE_FUNCTION(, HL_ASYNC_OP_MODULE_HANDLE, hl_async_op_module_create, EXECUTION_ENGINE_HANDLE, execution_engine, ML_ASYNC_OP_MODULE_HANDLE, ml_handle, ML_ASYNC_OP_MODULE_WITH_ASYNC_CHAIN_HANDLE, ml_chain_handle, ML_ASYNC_OP_MODULE_WITH_RETRIES_HANDLE, ml_retries_handle);
MOCKABLE_FUNCTION(, void, hl_async_op_module_destroy, HL_ASYNC_OP_MODULE_HANDLE, handle);

MOCKABLE_FUNCTION(, int, hl_async_op_module_open, HL_ASYNC_OP_MODULE_HANDLE, handle);
MOCKABLE_FUNCTION(, void, hl_async_op_module_close, HL_ASYNC_OP_MODULE_HANDLE, handle);

// Sample functions which use ASYNC_OP

MOCKABLE_FUNCTION(, int, hl_async_op_module_execute_underlying_fake_cancel_async, HL_ASYNC_OP_MODULE_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, HL_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);
MOCKABLE_FUNCTION(, int, hl_async_op_module_execute_underlying_real_cancel_async, HL_ASYNC_OP_MODULE_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, HL_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);

MOCKABLE_FUNCTION(, int, hl_async_op_module_execute_underlying_fake_cancel_and_retries_async, HL_ASYNC_OP_MODULE_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, HL_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);
MOCKABLE_FUNCTION(, int, hl_async_op_module_execute_underlying_real_cancel_and_retries_async, HL_ASYNC_OP_MODULE_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, HL_ASYNC_OP_MODULE_EXECUTE_CALLBACK, callback, void*, context);

#ifdef __cplusplus
}
#endif

#endif /*HL_ASYNC_OP_MODULE_H*/
