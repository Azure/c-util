// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <inttypes.h>
#include <stdlib.h>
#include <stdalign.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/interlocked.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"

#define COMPILING_ASYNC_OP_C
#include "c_util/async_op.h"
#undef COMPILING_ASYNC_OP_C

MU_DEFINE_ENUM_STRINGS(ASYNC_OP_STATE, ASYNC_OP_STATE_VALUES);

THANDLE_TYPE_DEFINE(ASYNC_OP);

typedef struct CONTEXT_TO_ASYNC_OP_TAG
{
    ASYNC_OP* async_op; /*pointer to the ASYNC_OP containinf the context*/
} CONTEXT_TO_ASYNC_OP;

static void async_op_dispose(ASYNC_OP* async_op)
{
    if (async_op->dispose != NULL)
    {
        async_op->dispose(async_op->context);
    }
}

#define IS_POWER_OF_2(x) \
    (((x) & ((x) - 1)) == 0)

static void* get_next_address(void* start, uint32_t align)
{
    uint32_t m = (uintptr_t)start % align;
    if (m == 0)
    {
        return start;
    }
    else
    {
        return (void*)((uintptr_t)start - m + align);
    }
}


static void* get_prev_address(void* start, uint32_t size, uint32_t align)
{
    uintptr_t first_prev_address = (uintptr_t)start - size;
    uint32_t m = first_prev_address % align;
    if (m == 0)
    {
        return (void*)first_prev_address;
    }
    else
    {
        return (void*)((uintptr_t)first_prev_address - align);
    }
}


THANDLE(ASYNC_OP) async_op_create(ASYNC_OP_CANCEL_IMPL cancel, uint32_t context_size, uint32_t context_align, ASYNC_OP_DISPOSE dispose)
{
    THANDLE(ASYNC_OP) result = NULL;
    if (
        /*Codes_SRS_ASYNC_OP_02_001: [ If context_align is not a power of 2 then async_op_create shall fail and return NULL. ]*/
        (context_align == 0) ||
        (!IS_POWER_OF_2(context_align)) ||
        /*Codes_SRS_ASYNC_OP_02_004: [ If there are any failures then async_op_create shall fail and return NULL. ]*/ /*Note: overflow*/
        (context_size > UINT32_MAX - sizeof(CONTEXT_TO_ASYNC_OP) - alignof(CONTEXT_TO_ASYNC_OP) - 1  - context_size - 1 )
        )
    {
        LogError("invalid arguments ASYNC_OP_CANCEL_IMPL cancel=%p, uint32_t context_size=%" PRIu32 ", size_t context_align=%" PRIu32 ", ASYNC_OP_DISPOSE dispose=%p",
            cancel, context_size, context_align, dispose);
        /*return as is*/
    }
    else
    {
        /*Codes_SRS_ASYNC_OP_02_002: [ async_op_create shall call THANDLE_MALLOC_FLEX with the extra size set as (context_size > 0) * (context_size + context_align - 1).]*/
        THANDLE(ASYNC_OP) async_op = THANDLE_MALLOC_FLEX(ASYNC_OP)(async_op_dispose, 
            sizeof(CONTEXT_TO_ASYNC_OP) +
            alignof(CONTEXT_TO_ASYNC_OP) +
            context_size + context_align - 1 -1
            , 1);
        if (async_op == NULL)
        {
            /*Codes_SRS_ASYNC_OP_02_004: [ If there are any failures then async_op_create shall fail and return NULL. ]*/
            LogError("failure in THANDLE_MALLOC_FLEX(ASYNC_OP)(async_op_dispose=%p, sizeof(CONTEXT_TO_ASYNC_OP)=%zu + alignof(CONTEXT_TO_ASYNC_OP)=%zu + context_size=%" PRIu32 " + context_align=%" PRIu32 " - 1 - 1, 1);",
                async_op_dispose, sizeof(CONTEXT_TO_ASYNC_OP), alignof(CONTEXT_TO_ASYNC_OP) ,context_size, context_align);
            /*return as is*/
        }
        else
        {
            /*Codes_SRS_ASYNC_OP_02_003: [ async_op_create shall compute context (that the user is supposed to use), record cancel, dispose, set state to ASYNC_RUNNING and return a non-NULL value. ]*/

            ASYNC_OP* async_op_data = THANDLE_GET_T(ASYNC_OP)(async_op);

            /*all is fine, just set the fields*/
            async_op_data->cancel = cancel;
            async_op_data->dispose = dispose;

            /*find first byte in private_context that matches alignof(CONTEXT_TO_ASYNC_OP)*/
            //async_op_data->context = (void*)((((uintptr_t)async_op_data->private_context) + context_align - 1) / context_align * context_align);
            
            (void)memset(async_op_data->private_context, 0x33, sizeof(CONTEXT_TO_ASYNC_OP) +
                alignof(CONTEXT_TO_ASYNC_OP) +
                context_size + context_align - 1 - 1); /*vld.h: remove*/

            CONTEXT_TO_ASYNC_OP* context_to_async_op = get_next_address(async_op_data->private_context, alignof(CONTEXT_TO_ASYNC_OP));
            

            /*find the fist byte past CONTEXT_TO_ASYNC_OP that matches context_align*/
            async_op_data->context = get_next_address(context_to_async_op + 1, context_align);

            context_to_async_op = get_prev_address(async_op_data->context, sizeof(CONTEXT_TO_ASYNC_OP), alignof(CONTEXT_TO_ASYNC_OP));
            context_to_async_op->async_op = async_op_data;

            (void)memset(async_op_data->context, 0xAA, context_size);/*vld.h*/

            (void)interlocked_exchange(&async_op_data->cancel_state, ASYNC_RUNNING);

            THANDLE_INITIALIZE_MOVE(ASYNC_OP)(&result, &async_op);
        }

    }
    return result;
}

ASYNC_OP_STATE async_op_cancel(THANDLE(ASYNC_OP) async_op)
{
    ASYNC_OP_STATE result;
    /*Codes_SRS_ASYNC_OP_02_005: [ If async_op is NULL then async_op_cancel shall return ASYNC_INVALID_ARG. ]*/
    if (async_op == NULL)
    {
        LogError("invalid arguments THANDLE(ASYNC_OP) async_op=%p", async_op);
        result = ASYNC_INVALID_ARG;
    }
    else
    {
        /*Codes_SRS_ASYNC_OP_02_006: [ async_op_cancel shall atomically switch the state to ASYNC_CANCELLING if the current state is ASYNC_RUNNING by using interlocked_compare_exchange. ]*/
        if ((result = interlocked_compare_exchange((void*) &async_op->cancel_state, ASYNC_CANCELLING, ASYNC_RUNNING)) == ASYNC_RUNNING)
        {
            /*all is fine, just call the callback*/
            result = ASYNC_CANCELLING;

            /*Codes_SRS_ASYNC_OP_02_007: [ If async_op's cancel is non-NULL then async_op_cancel shall call it with async_op->context as parameter. ]*/
            if (async_op->cancel != NULL)
            {
                async_op->cancel(async_op->context);
            }
        }
        else
        {
            /*return as it was recorded already*/
        }
    }

    /*Codes_SRS_ASYNC_OP_02_008: [ async_op_cancel shall return the state of the operation. ]*/
    return result;
}

/*given a "context" (void*) returned to the user embedded into the THANDLE(ASYNC_OP) this function computes the ASYNC_OP's address*/
ASYNC_OP* async_op_from_context(void* context)
{
    ASYNC_OP* result;
    if (context == NULL)
    {
        LogError("invalid arguments void* context=%p", context);
        result = NULL;
    }
    else
    {
        /*find the first byte before context that's a multiple of CONTEXT_TO_ASCYN_OP align and the difference between that address and context is at least sizeof(CONTEXT_ASYNC_OP)*/

        uintptr_t start = (uintptr_t)context;

        uintptr_t prev = start - sizeof(CONTEXT_TO_ASYNC_OP);

        if (prev % alignof(CONTEXT_TO_ASYNC_OP) == 0)
        {
            result = ((CONTEXT_TO_ASYNC_OP*)prev)->async_op;
        }
        else
        {
            result = ((CONTEXT_TO_ASYNC_OP*)(prev - alignof(CONTEXT_TO_ASYNC_OP)))->async_op;
        }

    }
    return result;
}