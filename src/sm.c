// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include "windows.h"

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_logging/xlogging.h"
#include "azure_c_pal/gballoc.h"
#include "azure_c_pal/interlocked_hl.h"

#include "azure_c_util/sm.h"

#define SM_STATE_VALUES             \
    SM_CREATED,                     \
    SM_OPENING,                     \
    SM_OPENED,                      \
    SM_OPENED_DRAINING_TO_BARRIER,  \
    SM_OPENED_DRAINING_TO_CLOSE,    \
    SM_OPENED_BARRIER,              \
    SM_CLOSING                      \

MU_DEFINE_ENUM(SM_STATE, SM_STATE_VALUES)

#define SM_STATE_MASK       ((1<<7)-1) /*127*/
#define SM_STATE_INCREMENT  (1<<8)  /*256*/ /*at every state change, the state is incremented by this much*/

#define SM_CLOSE_BIT        (1<<7)

/*a PRI macro for the SM state*/
#define PRI_SM_STATE "" PRI_MU_ENUM " SM_CLOSE_BIT=%d"

/*a VALUE macro corresponding to PRI_SM_STATE - it takes SM_HANDLE_DATA.state as argument*/
#define SM_STATE_VALUE(state) MU_ENUM_VALUE(SM_STATE, (SM_STATE)((state) & SM_STATE_MASK)), (((state)&SM_CLOSE_BIT)>0)

typedef struct SM_HANDLE_DATA_TAG
{
    volatile LONG state;
    volatile LONG non_barrier_call_count; /*number of API calls to non-barriers*/
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
        LogError("sm name=%s, failure in malloc(sizeof(SM_HANDLE_DATA)=%zu);",
            MU_P_OR_NULL(name), sizeof(SM_HANDLE_DATA));
        /*return as is*/
    }
    else
    {
        /*Codes_SRS_SM_02_037: [ sm_create shall set state to SM_CREATED and n to 0. ]*/
        (void)InterlockedExchange(&result->state, SM_CREATED);
        (void)InterlockedExchange(&result->non_barrier_call_count, 0);
        (void)memcpy(result->name, name, flexSize);
        /*return as is*/
    }
    return result;
}

/*forwards*/
static SM_RESULT sm_close_begin_internal(SM_HANDLE sm);
static void sm_close_end_internal(SM_HANDLE sm);

void sm_destroy(SM_HANDLE sm)
{
    /*Codes_SRS_SM_02_005: [ If sm is NULL then sm_destroy shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
    }
    else
    {
        /*Codes_SRS_SM_02_038: [ sm_destroy behave as if sm_close_begin would have been called. ]*/
        if (sm_close_begin_internal(sm) == SM_EXEC_GRANTED)
        {
            sm_close_end_internal(sm);
        }

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
        /*Codes_SRS_SM_02_039: [ If the state is not SM_CREATED then sm_open_begin shall return SM_EXEC_REFUSED. ]*/
        LONG state = InterlockedAdd(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_CREATED)
        {
            LogError("sm name=%s. Cannot sm_open_begin that which is in %" PRI_SM_STATE " state", sm->name, SM_STATE_VALUE(state));
            result = SM_EXEC_REFUSED;
        }
        else
        {
            /*Codes_SRS_SM_02_040: [ sm_open_begin shall switch the state to SM_OPENING. ]*/
            if (InterlockedCompareExchange(&sm->state, state - SM_CREATED + SM_OPENING + SM_STATE_INCREMENT, state) != state)
            {
                LogError("sm name=%s. sm_open_begin state changed meanwhile (it was %" PRI_SM_STATE "). likely competing threads.", sm->name, SM_STATE_VALUE(state));
                result = SM_EXEC_REFUSED;
            }
            else
            {
                /*Codes_SRS_SM_02_009: [ sm_open_begin shall return SM_EXEC_GRANTED. ]*/
                result = SM_EXEC_GRANTED;
            }
        }
    }
    return result;
}

void sm_open_end(SM_HANDLE sm, bool success)
{
    /*Codes_SRS_SM_02_010: [ If sm is NULL then sm_open_end shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
    }
    else
    {
        /*Codes_SRS_SM_02_041: [ If state is not SM_OPENING then sm_open_end shall return. ]*/
        LONG state = InterlockedAdd(&sm->state, 0);
        if ((state & SM_STATE_MASK) != SM_OPENING)
        {
            LogError("sm name=%s. cannot sm_open_end that which is in %" PRI_SM_STATE " state", sm->name, SM_STATE_VALUE(state));
        }
        else
        {
            if (success)
            {
                /*Codes_SRS_SM_02_074: [ If success is true then sm_open_end shall switch the state to SM_OPENED. ]*/
                if (InterlockedCompareExchange(&sm->state, state - SM_OPENING + SM_OPENED + SM_STATE_INCREMENT, state) != state)
                {
                    LogError("sm name=%s. sm_open_end state changed meanwhile (it was %" PRI_SM_STATE ", likely competing threads.", sm->name, SM_STATE_VALUE(state));
                }
                else
                {
                    /*we are done*/
                }
            }
            else
            {
                /*Codes_SRS_SM_02_075: [ If success is false then sm_open_end shall switch the state to SM_CREATED. ]*/
                if (InterlockedCompareExchange(&sm->state, state - SM_OPENING + SM_CREATED + SM_STATE_INCREMENT, state) != state)
                {
                    LogError("sm name=%s. sm_open_end state changed meanwhile (it was %" PRI_SM_STATE ", likely competing threads.", sm->name, SM_STATE_VALUE(state));
                }
                else
                {
                    /*we are done*/
                }
            }
        }
    }
}

static SM_RESULT sm_close_begin_internal(SM_HANDLE sm)
{
    SM_RESULT result;

    LONG state;
    /*Codes_SRS_SM_02_045: [ sm_close_begin shall set SM_CLOSE_BIT to 1. ]*/
    if (((state=InterlockedOr(&sm->state, SM_CLOSE_BIT)) & SM_CLOSE_BIT) == SM_CLOSE_BIT)
    {
        /*Codes_SRS_SM_02_046: [ If SM_CLOSE_BIT was already 1 then sm_close_begin shall return SM_EXEC_REFUSED. ]*/
        LogError("sm name=%s. another thread is performing close (state=%" PRI_SM_STATE ")", sm->name, SM_STATE_VALUE(state));
        result = SM_EXEC_REFUSED;
    }
    else
    {
        do
        {
            state = InterlockedAdd(&sm->state, 0);

            /*Codes_SRS_SM_02_047: [ If the state is SM_OPENED then sm_close_begin shall switch it to SM_OPENED_DRAINING_TO_CLOSE. ]*/
            if ((state & SM_STATE_MASK) == SM_OPENED)
            {
                if (InterlockedCompareExchange(&sm->state, state - SM_OPENED + SM_OPENED_DRAINING_TO_CLOSE + SM_STATE_INCREMENT, state) != state)
                {
                    /*go and retry*/
                }
                else
                {
                    /*Codes_SRS_SM_02_048: [ sm_close_begin shall wait for n to reach 0. ]*/
                    if (InterlockedHL_WaitForValue(&sm->non_barrier_call_count, 0, INFINITE) != INTERLOCKED_HL_OK)
                    {
                        /*Codes_SRS_SM_02_071: [ If there are any failures then sm_close_begin shall fail and return SM_ERROR. ]*/
                        LogError("sm name=%s. failure in InterlockedHL_WaitForValue(&sm->non_barrier_call_count=%p, 0, INFINITE), state was %" PRI_SM_STATE "", sm->name, &sm->non_barrier_call_count, SM_STATE_VALUE(state));
                        (void)InterlockedAdd(&sm->state, -SM_OPENED_DRAINING_TO_CLOSE + SM_OPENED + SM_STATE_INCREMENT); /*undo state to SM_OPENED...*/
                        result = SM_ERROR;
                        break;
                    }
                    
                    /*Codes_SRS_SM_02_049: [ sm_close_begin shall switch the state to SM_CLOSING and return SM_EXEC_GRANTED. ]*/
                    (void)InterlockedAdd(&sm->state, -SM_OPENED_DRAINING_TO_CLOSE + SM_CLOSING + SM_STATE_INCREMENT);
                    result = SM_EXEC_GRANTED;
                    break;
                }
            }
            else if (
                /*Codes_SRS_SM_02_050: [ If the state is SM_OPENED_BARRIER then sm_close_begin shall re-evaluate the state. ]*/
                ((state & SM_STATE_MASK) == SM_OPENED_BARRIER) ||
                /*Codes_SRS_SM_02_051: [ If the state is SM_OPENED_DRAINING_TO_BARRIER then sm_close_begin shall re-evaluate the state. ]*/
                ((state & SM_STATE_MASK) == SM_OPENED_DRAINING_TO_BARRIER)
                )
            {
                Sleep(1);
            }
            else
            {
                /*Codes_SRS_SM_02_052: [ If the state is any other value then sm_close_begin shall return SM_EXEC_REFUSED. ]*/
                result = SM_EXEC_REFUSED;
                break;
            }
        } while (1);

        /*Codes_SRS_SM_02_053: [ sm_close_begin shall set SM_CLOSE_BIT to 0. ]*/
        (void)InterlockedAnd(&sm->state, ~(ULONG)SM_CLOSE_BIT);
    }
    return result;
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
        result = sm_close_begin_internal(sm);
    }
    
    return result;
}

static void sm_close_end_internal(SM_HANDLE sm)
{
    /*Codes_SRS_SM_02_043: [ If the state is not SM_CLOSING then sm_close_end shall return. ]*/
    LONG state = InterlockedAdd(&sm->state, 0);
    if ((state & SM_STATE_MASK) != SM_CLOSING)
    {
        LogError("sm name=%s. cannot sm_close_end_internal that which is in %" PRI_SM_STATE " state", sm->name, SM_STATE_VALUE(state));
    }
    else
    {
        /*Codes_SRS_SM_02_044: [ sm_close_end shall switch the state to SM_CREATED. ]*/
        if (InterlockedCompareExchange(&sm->state, state - SM_CLOSING + SM_CREATED + SM_STATE_INCREMENT, state) != state)
        {
            LogError("sm name=%s. state changed meanwhile (it was %" PRI_SM_STATE "), likely competing threads", sm->name, SM_STATE_VALUE(state));
        }
        else
        {

        }
    }
}

void sm_close_end(SM_HANDLE sm)
{
    /*Codes_SRS_SM_02_018: [ If sm is NULL then sm_close_end shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
    }
    else
    {
        sm_close_end_internal(sm);
    }
}

SM_RESULT sm_exec_begin(SM_HANDLE sm)
{
    SM_RESULT result;
    /*Codes_SRS_SM_02_021: [ If sm is NULL then sm_exec_begin shall fail and return SM_ERROR. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = SM_ERROR;
    }
    else
    {
        LONG state1 = InterlockedAdd(&sm->state, 0);
        if (
            /*Codes_SRS_SM_02_054: [ If state is not SM_OPENED then sm_exec_begin shall return SM_EXEC_REFUSED. ]*/
            ((state1 & SM_STATE_MASK) != SM_OPENED) ||
            /*Codes_SRS_SM_02_055: [ If SM_CLOSE_BIT is 1 then sm_exec_begin shall return SM_EXEC_REFUSED. ]*/
            ((state1 & SM_CLOSE_BIT) == SM_CLOSE_BIT)
            )
        {
            LogError("sm name=%s. cannot call sm_exec_begin when state is %" PRI_SM_STATE "", sm->name, SM_STATE_VALUE(state1));
            result = SM_EXEC_REFUSED;
        }
        else
        {
            /*Codes_SRS_SM_02_056: [ sm_exec_begin shall increment n. ]*/
            (void)InterlockedIncrement(&sm->non_barrier_call_count);
            LONG state2 = InterlockedAdd(&sm->state, 0);

            /*Codes_SRS_SM_02_057: [ If the state changed after incrementing n then sm_exec_begin shall return SM_EXEC_REFUSED. ]*/
            if (state1 != state2)
            {
                LogError("sm name=%s. state changed meanwhile from %" PRI_SM_STATE " to %" PRI_SM_STATE "", sm->name, SM_STATE_VALUE(state1), SM_STATE_VALUE(state2));
                LONG n = InterlockedDecrement(&sm->non_barrier_call_count);
                if (n == 0)
                {
                    WakeByAddressSingle((void*)&sm->non_barrier_call_count);
                }
                result = SM_EXEC_REFUSED;
            }
            else
            {
                /*Codes_SRS_SM_02_058: [ sm_exec_begin shall return SM_EXEC_GRANTED. ]*/
                result = SM_EXEC_GRANTED;
            }
        }
    }
    return result;
}

void sm_exec_end(SM_HANDLE sm)
{
    /*Codes_SRS_SM_02_024: [ If sm is NULL then sm_exec_end shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid arg SM_HANDLE sm=%p", sm);
    }
    else
    {
        LONG state = InterlockedAdd(&sm->state, 0);
        if (
            /*Codes_SRS_SM_02_059: [ If state is not SM_OPENED then sm_exec_end shall return. ]*/
            ((state & SM_STATE_MASK) != SM_OPENED) &&
            /*Codes_SRS_SM_02_060: [ If state is not SM_OPENED_DRAINING_TO_BARRIER then sm_exec_end shall return. ]*/
            ((state & SM_STATE_MASK) != SM_OPENED_DRAINING_TO_BARRIER) &&
            /*Codes_SRS_SM_02_061: [ If state is not SM_OPENED_DRAINING_TO_CLOSE then sm_exec_end shall return. ]*/
            ((state & SM_STATE_MASK) != SM_OPENED_DRAINING_TO_CLOSE)
            )
        {
            LogError("sm name=%s. cannot execute exec end when state is %" PRI_SM_STATE "", sm->name, SM_STATE_VALUE(state));
        }
        else
        {
            /*Codes_SRS_SM_02_062: [ sm_exec_end shall decrement n with saturation at 0. ]*/
            do /*a rather convoluted loop to make _end be idempotent (too many _end will be ignored)*/
            {
                LONG n = InterlockedAdd(&sm->non_barrier_call_count, 0);
                if (n <= 0)
                {
                    break;
                }
                else
                {
                    if (InterlockedCompareExchange(&sm->non_barrier_call_count, n - 1, n) != n)
                    {
                        /*well - retry sort of...*/
                    }
                    else
                    {
                        /*Codes_SRS_SM_02_063: [ If n reaches 0 then sm_exec_end shall signal that. ]*/
                        if (n - 1 == 0)
                        {
                            WakeByAddressSingle((void*)&sm->non_barrier_call_count);
                        }
                        break;
                    }
                }
            } while (1);
        }
    }
}

SM_RESULT sm_barrier_begin(SM_HANDLE sm)
{
    SM_RESULT result;
    /*Codes_SRS_SM_02_027: [ If sm is NULL then sm_barrier_begin shall fail and return SM_ERROR. ]*/
    if (sm == NULL)
    {
        LogError("invalid arg SM_HANDLE sm=%p", sm);
        result = SM_ERROR;
    }
    else
    {
        LONG state = InterlockedAdd(&sm->state, 0);
        if (
            /*Codes_SRS_SM_02_064: [ If state is not SM_OPENED then sm_barrier_begin shall return SM_EXEC_REFUSED. ]*/
            ((state & SM_STATE_MASK) != SM_OPENED) ||
            /*Codes_SRS_SM_02_065: [ If SM_CLOSE_BIT is set to 1 then sm_barrier_begin shall return SM_EXEC_REFUSED. ]*/
            ((state & SM_CLOSE_BIT) == SM_CLOSE_BIT)
            )
        {
            LogError("sm name=%s. cannot execute barrier begin when state is %" PRI_SM_STATE "", sm->name, SM_STATE_VALUE(state));
            result = SM_EXEC_REFUSED;
        }
        else
        {
            /*Codes_SRS_SM_02_066: [ sm_barrier_begin shall switch the state to SM_OPENED_DRAINING_TO_BARRIER. ]*/
            if (InterlockedCompareExchange(&sm->state, state - SM_OPENED + SM_OPENED_DRAINING_TO_BARRIER + SM_STATE_INCREMENT, state) != state)
            {
                /*Codes_SRS_SM_02_067: [ If the state changed meanwhile then sm_barrier_begin shall return SM_EXEC_REFUSED. ]*/
                LogError("sm name=%s. state changed meanwhile (it was %" PRI_SM_STATE "), this thread cannot start a barrier, likely competing threads", sm->name, SM_STATE_VALUE(state));
                result = SM_EXEC_REFUSED;
            }
            else
            {
                /*Codes_SRS_SM_02_068: [ sm_barrier_begin shall wait for n to reach 0. ]*/
                if (InterlockedHL_WaitForValue(&sm->non_barrier_call_count, 0, INFINITE) != INTERLOCKED_HL_OK)
                {
                    /*switch back the state*/
                    /*Codes_SRS_SM_02_070: [ If there are any failures then sm_barrier_begin shall return SM_ERROR. ]*/
                    (void)InterlockedAdd(&sm->state, -SM_OPENED_DRAINING_TO_BARRIER + SM_OPENED + SM_STATE_INCREMENT);
                    LogError("sm name=%s. failure in InterlockedHL_WaitForValue(&sm->non_barrier_call_count=%p, 0, INFINITE), state was %" PRI_SM_STATE "", sm->name, &sm->non_barrier_call_count, SM_STATE_VALUE(state));
                    result = SM_ERROR;
                }
                else
                {
                    /*Codes_SRS_SM_02_069: [ sm_barrier_begin shall switch the state to SM_OPENED_BARRIER and return SM_EXEC_GRANTED. ]*/
                    (void)InterlockedAdd(&sm->state, -SM_OPENED_DRAINING_TO_BARRIER + SM_OPENED_BARRIER + SM_STATE_INCREMENT);
                    result = SM_EXEC_GRANTED;
                }
            }
        }
    }
    return result;
}

void sm_barrier_end(SM_HANDLE sm)
{
    /*Codes_SRS_SM_02_032: [ If sm is NULL then sm_barrier_end shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid arg SM_HANDLE sm=%p", sm);
    }
    else
    {
        LONG state = InterlockedAdd(&sm->state, 0);
        /*Codes_SRS_SM_02_072: [ If state is not SM_OPENED_BARRIER then sm_barrier_end shall return. ]*/
        if ((state & SM_STATE_MASK) != SM_OPENED_BARRIER)
        {
            LogError("sm name=%s. cannot call sm_barrier_end when state is %" PRI_SM_STATE "", sm->name, SM_STATE_VALUE(state));
        }
        else
        {
            /*Codes_SRS_SM_02_073: [ sm_barrier_end shall switch the state to SM_OPENED. ]*/
            if (InterlockedCompareExchange(&sm->state, state - SM_OPENED_BARRIER + SM_OPENED + SM_STATE_INCREMENT, state) != state)
            {
                LogError("sm name=%s. state changed meanwhile (it was %" PRI_SM_STATE "), likely competing threads", sm->name, SM_STATE_VALUE(state));
            }
            else
            {
                /*it's all fine, we're back to SM_OPENED*/ /*let a close know about that, if any*/
            }
        }
    }
}
