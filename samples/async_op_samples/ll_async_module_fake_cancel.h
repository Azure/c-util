// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef LL_ASYNC_OP_MODULE_FAKE_CANCEL_H
#define LL_ASYNC_OP_MODULE_FAKE_CANCEL_H

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

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct LL_ASYNC_OP_MODULE_FAKE_CANCEL_TAG* LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE;

#define LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_VALUES \
    LL_ASYNC_OP_MODULE_FAKE_CANCEL_OK, \
    LL_ASYNC_OP_MODULE_FAKE_CANCEL_CANCELLED, \
    LL_ASYNC_OP_MODULE_FAKE_CANCEL_ERROR \

MU_DEFINE_ENUM(LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT, LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT_VALUES);

typedef void(*LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CALLBACK)(void* context, LL_ASYNC_OP_MODULE_FAKE_CANCEL_RESULT result);

MOCKABLE_FUNCTION(, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, ll_async_op_module_fake_cancel_create, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, ll_async_op_module_fake_cancel_destroy, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle);

MOCKABLE_FUNCTION(, int, ll_async_op_module_fake_cancel_open, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle);
MOCKABLE_FUNCTION(, void, ll_async_op_module_fake_cancel_close, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle);

MOCKABLE_FUNCTION(, int, ll_async_op_module_fake_cancel_execute_async, LL_ASYNC_OP_MODULE_FAKE_CANCEL_HANDLE, handle, uint32_t, complete_in_ms, THANDLE(ASYNC_OP)*, async_op_out, LL_ASYNC_OP_MODULE_FAKE_CANCEL_EXECUTE_CALLBACK, callback, void*, context);

#ifdef __cplusplus
}
#endif

#endif /*LL_ASYNC_OP_MODULE_FAKE_CANCEL_H*/
