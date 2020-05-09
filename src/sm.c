// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "windows.h"

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_util/xlogging.h"
#include "azure_c_util/gballoc.h"
#include "azure_c_util/interlocked_hl.h"

#include "azure_c_util/sm.h"

#define SM_STATE_VALUES             \
    SM_CREATED,               \
    SM_OPENING,               \
    SM_OPENED,                \
    SM_OPENED_DRAINING_TO_BARRIER,                \
    SM_OPENED_DRAINING_TO_CLOSE,                \
    SM_OPENED_BARRIER,          \
    SM_CLOSING                \

MU_DEFINE_ENUM(SM_STATE, SM_STATE_VALUES)

#define SM_STATE_MASK       255
#define SM_STATE_INCREMENT 256 /*at every state change, the state is incremented by this much*/

#undef LogError
#define LogError(...) {}

typedef struct SM_HANDLE_DATA_TAG
{
    volatile LONG64 state;
    volatile LONG64 n; /*number of API calls to non-barriers*/
#ifdef _MSC_VER
/*warning C4200: nonstandard extension used: zero-sized array in struct/union : looks very standard in C99 and it is called flexible array. Documentation-wise is a flexible array, but called "unsized" in Microsoft's docs*/ /*https://msdn.microsoft.com/en-us/library/b6fae073.aspx*/
#pragma warning(disable:4200)
#endif

    char name[]; /*used in printing "who this is"*/
}SM_HANDLE_DATA;

static const char NO_NAME[] = "NO_NAME";

MU_DEFINE_ENUM_STRINGS(SM_RESULT, SM_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(SM_STATE, SM_STATE_VALUES);

SM_HANDLE sm_create(const char* name)
{
    SM_HANDLE result;
    
    /*Codes_SRS_SM_02_001: [ If name is NULL then sm_create shall behave as if name was "NO_NAME". ]*/
    if (name == NULL)
    {
        name = NO_NAME; 
    }
    
    size_t flexSize = strlen(name) + 1;

    /*Codes_SRS_SM_02_002: [ sm_create shall allocate memory for the instance. ]*/
    result = malloc(sizeof(SM_HANDLE_DATA) + flexSize);

    if (result == NULL)
    {
        /*Codes_SRS_SM_02_004: [ If there are any failures then sm_create shall fail and return NULL. ]*/
        LogError("SM name=%s, failure in malloc(sizeof(SM_HANDLE_DATA)=%zu);",
            MU_P_OR_NULL(name), sizeof(SM_HANDLE_DATA));
        /*return as is*/
    }
    else
    {
        /*Codes_SRS_SM_02_003: [ sm_create shall set b_now to -1, n to 0, and e to 0 succeed and return a non-NULL value. ]*/
        (void)InterlockedExchange64(&result->state, SM_CREATED);
        (void)InterlockedExchange64(&result->n, 0);
        (void)memcpy(result->name, name, flexSize);
        /*return as is*/
    }
    return result;
}


void sm_destroy(SM_HANDLE sm)
{
    /*Codes_SRS_SM_02_005: [ If sm is NULL then sm_destroy shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
    }
    else
    {
        /*Codes_SRS_SM_02_006: [ sm_destroy shall free all used resources. ]*/
        free(sm);
    }
}

int sm_open_begin(SM_HANDLE sm)
{
    SM_RESULT result;
    if (sm == NULL)
    {
        /*Codes_SRS_SM_02_007: [ If sm is NULL then sm_open_begin shall fail and return SM_ERROR. ]*/
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = SM_ERROR;
    }
    else
    {
        LONG64 state = InterlockedAdd64(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_CREATED)
        {
            LogError("cannot begin to open that which is in %" PRI_MU_ENUM " state", MU_ENUM_VALUE(SM_STATE, state & SM_STATE_MASK));
            result = SM_EXEC_REFUSED;
        }
        else
        {
            if (InterlockedCompareExchange64(&sm->state, state - SM_CREATED + SM_OPENING + SM_STATE_INCREMENT, state) != state)
            {
                LogError("state changed meanwhile, maybe some other thread...");
                result = SM_EXEC_REFUSED;
            }
            else
            {
                result = SM_EXEC_GRANTED;
            }
        }
    }
    return result;
}

void sm_open_end(SM_HANDLE sm)
{
    if (sm == NULL)
    {
        /*Codes_SRS_SM_02_010: [ If sm is NULL then sm_open_end shall return. ]*/
        LogError("invalid argument SM_HANDLE sm=%p", sm);
    }
    else
    {
        LONG64 state = InterlockedAdd64(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_OPENING)
        {
            LogError("cannot end to open that which is in %" PRI_MU_ENUM " state", MU_ENUM_VALUE(SM_STATE, state & SM_STATE_MASK));
        }
        else
        {
            if (InterlockedCompareExchange64(&sm->state, state - SM_CREATED + SM_OPENING + SM_STATE_INCREMENT, state) != state)
            {
                LogError("state changed meanwhile, very straaaaange");
            }
            else
            {
            }
        }
    }
}

SM_RESULT sm_close_begin(SM_HANDLE sm)
{

    SM_RESULT result;
    /*Codes_SRS_SM_02_013: [ If sm is NULL then sm_close_begin shall fail and return SM_ERROR. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = SM_ERROR;
    }
    else
    {
        LONG64 state = InterlockedAdd64(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_OPENED)
        {
            LogError("cannot begin to close that which is in %" PRI_MU_ENUM " state", MU_ENUM_VALUE(SM_STATE, state & SM_STATE_MASK));
            result = SM_EXEC_REFUSED;
        }
        else
        {
            LONG64 draining_state;
            if ((draining_state = InterlockedCompareExchange64(&sm->state, state - SM_OPENED + SM_OPENED_DRAINING_TO_CLOSE + SM_STATE_INCREMENT, state)) != state)
            {
                LogError("state changed meanwhile, this thread cannot close");
                result = SM_EXEC_REFUSED;
            }
            else
            {
                InterlockedHL_WaitForValue64(&sm->n, 0, INFINITE);

                InterlockedAdd64(&sm->state, - SM_OPENED_DRAINING_TO_CLOSE + SM_CLOSING + SM_STATE_INCREMENT);
                result = SM_EXEC_GRANTED;
            }
        }
    }
    
    return result;
}

void sm_close_end(SM_HANDLE sm)
{
    if (sm == NULL)
    {
        /*Codes_SRS_SM_02_010: [ If sm is NULL then sm_open_end shall return. ]*/
        LogError("invalid argument SM_HANDLE sm=%p", sm);
    }
    else
    {
        LONG64 state = InterlockedAdd64(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_CLOSING)
        {
            LogError("cannot end to close that which is in %" PRI_MU_ENUM " state", MU_ENUM_VALUE(SM_STATE, state & SM_STATE_MASK));
        }
        else
        {
            if (InterlockedCompareExchange64(&sm->state, state - SM_CLOSING + SM_CREATED + SM_STATE_INCREMENT, state) != state)
            {
                LogError("state changed meanwhile, very straaaaange");
            }
            else
            {

            }
        }
    }
}

SM_RESULT sm_begin(SM_HANDLE sm)
{
    SM_RESULT result;
    LONG64 state1 = InterlockedAdd64(&sm->state, 0);
    if ((state1 & SM_STATE_MASK) != SM_OPENED)
    {
        LogError("cannot execute begin when state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_STATE, state1 & SM_STATE_MASK));
        result = SM_EXEC_REFUSED;
    }
    else
    {
        InterlockedIncrement64(&sm->n);
        LONG64 state2 = InterlockedAdd64(&sm->state, 0);
        if (state2 != state1)
        {
            LONG64 n = InterlockedDecrement64(&sm->n);
            if (n == 0)
            {
                WakeByAddressSingle((void*)&sm->n);
            }
            result = SM_EXEC_REFUSED;
        }
        else
        {
            result = SM_EXEC_GRANTED;
        }
    }
    return result;
}

void sm_end(SM_HANDLE sm)
{
    if (sm == NULL)
    {
        LogError("return");
    }
    else
    {
        LONG64 state1 = InterlockedAdd64(&sm->state, 0);
        if (
            ((state1 & SM_STATE_MASK) != SM_OPENED) &&
            ((state1 & SM_STATE_MASK) != SM_OPENED_DRAINING_TO_BARRIER) &&
            ((state1 & SM_STATE_MASK) != SM_OPENED_DRAINING_TO_CLOSE)
            )
        {
            LogError("cannot execute end when state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_STATE, state1 & SM_STATE_MASK));
        }
        else
        {
            LONG64 n = InterlockedDecrement64(&sm->n);
            if (n == 0)
            {
                WakeByAddressSingle((void*)&sm->n);
            }
        }
    }
}

SM_RESULT sm_barrier_begin(SM_HANDLE sm)
{
    SM_RESULT result;
    if (sm == NULL)
    {
        LogError("return");
        result = SM_ERROR;
    }
    else
    {
        LONG64 state = InterlockedAdd64(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_OPENED)
        {
            LogError("cannot execute barrier begin when state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_STATE, state & SM_STATE_MASK));
            result = SM_EXEC_REFUSED;
        }
        else
        {
            LONG64 draining_state;
            if ((draining_state=InterlockedCompareExchange64(&sm->state, state - SM_OPENED + SM_OPENED_DRAINING_TO_BARRIER + SM_STATE_INCREMENT, state)) != state)
            {
                LogError("state changed meanwhile, this thread cannot start a barrier");
                result = SM_EXEC_REFUSED;
            }
            else
            {
                InterlockedHL_WaitForValue64(&sm->n, 0, INFINITE);
                
                InterlockedAdd64(&sm->state, - SM_OPENED_DRAINING_TO_BARRIER + SM_OPENED_BARRIER + SM_STATE_INCREMENT);
                result = SM_EXEC_GRANTED;
            }
        }
    }
    return result;
}

void sm_barrier_end(SM_HANDLE sm)
{
    if (sm == NULL)
    {
        LogError("return");
    }
    else
    {
        LONG64 state = InterlockedAdd64(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_OPENED_BARRIER)
        {
            LogError("cannot execute barrier end when state is %" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_STATE, state & SM_STATE_MASK));
        }
        else
        {
            if (InterlockedCompareExchange64(&sm->state, state - SM_OPENED_BARRIER + SM_OPENED + SM_STATE_INCREMENT, state) != state)
            {
                LogError("state changed meanwhile, very straaaaange");
            }
            else
            {
                /*it's all fine, we're back to SM_OPENED*/
            }
        }
    }
}
