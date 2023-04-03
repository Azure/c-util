// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#ifndef ASYNC_OP_H
#define ASYNC_OP_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ASYNC_OP_STATE_VALUES \
    ASYNC_RUNNING /*initial state*/, \
    ASYNC_CANCELLING /*set when cancel is called.*/, \
    ASYNC_INVALID_ARG /*returned when called with invalid arguments*/ \

MU_DEFINE_ENUM(ASYNC_OP_STATE, ASYNC_OP_STATE_VALUES);

typedef void(*ASYNC_OP_CANCEL_IMPL)(void* context); /*async_op calls this function when it needs to cancel. params */
typedef void(*ASYNC_OP_DISPOSE)(void* context);

typedef struct ASYNC_OP_TAG
{
    void* context; /*this is supposed to be used by the user*/
    struct /* structure of fields that the user should never use or care about */
    {
        ASYNC_OP_CANCEL_IMPL cancel;

        ASYNC_OP_DISPOSE dispose;
        union
        {
            ASYNC_OP_STATE cancel_state_e; /*just for seeing the state as string instead of numbers in a debugger if needed*/
            volatile_atomic int32_t cancel_state;
        };
        unsigned char private_context[]; /*not for use. context is the only field in ASYNC_OP that is user accesible*/
    }
#ifndef COMPILING_ASYNC_OP_C
    HERE_BE_DRAGONS_DO_NOT_USE /*this preprocessor trick here will introduce a field called HERE_BE_DRAGONS_DO_NOT_USE in the structure as seen by the user. Internally (in async_op.c), it does not*/
#endif
    ;
} ASYNC_OP;

THANDLE_TYPE_DECLARE(ASYNC_OP);

MOCKABLE_FUNCTION(, THANDLE(ASYNC_OP), async_op_create, ASYNC_OP_CANCEL_IMPL, cancel, uint32_t, context_size, uint32_t, context_align, ASYNC_OP_DISPOSE, dispose);
MOCKABLE_FUNCTION(, ASYNC_OP_STATE, async_op_cancel, THANDLE(ASYNC_OP), async_op);

#ifdef __cplusplus
}
#endif

#endif /*ASYNC_OP_H*/
