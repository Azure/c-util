// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef SM_H
#define SM_H

#ifdef __cplusplus
/*C++ has native support for "bool"*/
#else
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "umock_c/umock_c_prod.h"

typedef struct SM_HANDLE_DATA_TAG* SM_HANDLE;

#define SM_RESULT_VALUES    \
    SM_EXEC_GRANTED,        \
    SM_EXEC_REFUSED,        \
    SM_ERROR                \

MU_DEFINE_ENUM(SM_RESULT, SM_RESULT_VALUES);

typedef void(*ON_SM_CLOSING_COMPLETE_CALLBACK)(void* context);

MOCKABLE_FUNCTION(, SM_HANDLE, sm_create, const char*, name);
MOCKABLE_FUNCTION(, void, sm_destroy, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_open_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_open_end, SM_HANDLE, sm, bool, success);

MOCKABLE_FUNCTION(, SM_RESULT, sm_close_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, SM_RESULT, sm_close_begin_with_cb, SM_HANDLE, sm, ON_SM_CLOSING_COMPLETE_CALLBACK, callback, void*, callback_context);
MOCKABLE_FUNCTION(, void, sm_close_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_exec_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_exec_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, SM_RESULT, sm_barrier_begin, SM_HANDLE, sm);
MOCKABLE_FUNCTION(, void, sm_barrier_end, SM_HANDLE, sm);

MOCKABLE_FUNCTION(, void, sm_fault, SM_HANDLE, sm);

#ifdef __cplusplus
}
#endif

#endif /*SM_H*/
