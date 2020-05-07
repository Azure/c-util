// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "windows.h"

#include "azure_c_util/xlogging.h"
#include "azure_c_util/gballoc.h"
#include "azure_c_util/interlocked_hl.h"

#include "azure_c_util/sm.h"

typedef struct SM_HANDLE_DATA_TAG
{
    volatile LONG64 b_now; /*where's the barrier at, starts at 0. 1 is when barrier is active. 2,4,6... barrier inactive. 3,5,7... barrier active*/
    volatile LONG64 n; /*number of API calls to barriers and non-barriers alike*/
    volatile LONG64 e; /*how many of the APIs have executed. Both abrriers and non-barriers set this*/
    volatile LONG e_done; /*varible that is signalled when e == n-1 (so a barrier is signaled that it might be allowed to proceed with execution */
#ifdef _MSC_VER
/*warning C4200: nonstandard extension used: zero-sized array in struct/union : looks very standard in C99 and it is called flexible array. Documentation-wise is a flexible array, but called "unsized" in Microsoft's docs*/ /*https://msdn.microsoft.com/en-us/library/b6fae073.aspx*/
#pragma warning(disable:4200)
#endif

    char name[]; /*used in printing "who this is"*/
}SM_HANDLE_DATA;

static const char NO_NAME[] = "NO_NAME";

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
        (void)InterlockedExchange64(&result->b_now, -1);
        (void)InterlockedExchange64(&result->n, 0);
        (void)InterlockedExchange64(&result->e, 0);
        (void)InterlockedExchange(&result->e_done, 0);
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
    int result;
    if (sm == NULL)
    {
        /*Codes_SRS_SM_02_007: [ If sm is NULL then sm_open_begin shall fail and return a non-zero value. ]*/
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_SM_02_008: [ If b_now is not -1 then sm_open_begin shall fail and return a non-zero value. ]*/
        /*Codes_SRS_SM_02_009: [ sm_open_begin shall set b_now to 0, succeed and return 0. ]*/
        LONG64 b_now = InterlockedCompareExchange64(&sm->b_now, 0, -1);
        if (b_now != -1)
        {
            /*Codes_SRS_SM_02_012: [ If sm_open_end doesn't follow a call to sm_open_begin then sm_open_end shall return. ]*/
            LogError("cannot execute, name=%s, b_now=%" PRId64 "", sm->name, b_now);
            result = MU_FAILURE;
        }
        else
        {
            InterlockedIncrement64(&sm->n);
            result = 0;
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
        /*Codes_SRS_SM_02_011: [ sm_open_end shall set b_now to INT64_MAX and return. ]*/
        InterlockedIncrement64(&sm->e);
        InterlockedExchange64(&sm->b_now, 0);
    }
}

/*aims at getting a stable "e" for a stable "n" and returns the difference between them*/
static LONG64 get_n_minus_e(SM_HANDLE sm)
{
    LONG64 e1, e2, n1, n2;
    do
    {
        e1 = InterlockedAdd64(&sm->e, 0);
        do
        {
            n1 = InterlockedAdd64(&sm->n, 0);
            e2 = InterlockedAdd64(&sm->e, 0);
            n2 = InterlockedAdd64(&sm->n, 0);
        } while (n1 != n2);
    } while (e1 != e2);

    return n1 - e1;
}

/*sm_barrier_begin calls this. Also sm_close calls this, because _close is nothing else but another barrier*/
static int sm_barrier_begin_internal(SM_HANDLE sm)
{
    int result;
    LONG64 b_now = InterlockedOr64(&sm->b_now, 1);

    if ((b_now & 1) == 0)
    {
        InterlockedExchange(&sm->e_done, 0);

        LONG64 n = InterlockedIncrement64(&sm->n);
        (void)n;

        result = 0;

        /*drain previous executions*/
        while (get_n_minus_e(sm) != 1)
        {
            if (InterlockedHL_WaitForValue(&sm->e_done, 1, INFINITE) != INTERLOCKED_HL_OK)
            {
                LogError("failure in InterlockedHL_WaitForValue(&sm->e, n - 1, INFINITE), name=%s, n=%" PRId64 "", sm->name, n);
                InterlockedIncrement64(&sm->e); /*this is pretty fatal here - the wait failed... */
                result = MU_FAILURE;
                break;
            }
            else
            {
                /*will be checked up*/
            }
        }
    }
    else
    {
        //LogError("there's a barrier already name=%s, b_now=%" PRId64 "", sm->name, b_now);
        result = MU_FAILURE;
    }
    return result;
}


int sm_close_begin(SM_HANDLE sm)
{

    int result;
    /*Codes_SRS_SM_02_013: [ If sm is NULL then sm_close_begin shall fail and return a non-zero value. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = MU_FAILURE;
    }
    else
    {
        result = sm_barrier_begin_internal(sm);
    }
    return result;
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
        /*cannot really check that it is paired with a _close_begin without moving b_now through user land, not going to do that. That solution woould push the b_now of the "close" in user land. And user has to return with the same b_now here and check that it is the same*/
        /*Codes_SRS_SM_02_019: [ sm_close_end shall switch b_now to -1. ]*/
        (void)InterlockedExchange64(&sm->b_now, -1);
        (void)InterlockedExchange64(&sm->n, 0);
        (void)InterlockedExchange64(&sm->e, 0);
        (void)InterlockedExchange(&sm->e_done, 0);
    }
}

int sm_begin(SM_HANDLE sm)
{
    int result;
    /*Codes_SRS_SM_02_021: [ If sm is NULL then sm_begin shall fail and return a non-zero value. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = MU_FAILURE;
    }
    else
    {
        LONG64 b_now_1 = InterlockedAdd64(&sm->b_now, 0);

        if ((b_now_1 & 1)==1)
        {
            //LogError("there's a barrier already name=%s, b_now=%" PRId64 "", sm->name, b_now_1);
            result = MU_FAILURE;
        }
        else
        {
            (void)InterlockedIncrement64(&sm->n);

            LONG64 b_now_2 = InterlockedAdd64(&sm->b_now, 0);
            if (b_now_2 != b_now_1)
            {
                InterlockedIncrement64(&sm->e);
                if (get_n_minus_e(sm) == 1)
                {
                    InterlockedHL_SetAndWake((void*)&sm->e_done, 1);
                }
                result = MU_FAILURE;
            }
            else
            {
                result = 0;
            }
        }
    }
    
    return result;
}

void sm_end(SM_HANDLE sm)
{
    int result;
    /*Codes_SRS_SM_02_024: [ If sm is NULL then sm_end shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_SM_02_025: [ sm_end shall increment the number of executed APIs (e). ]*/
        (void)InterlockedIncrement64(&sm->e);

        /*Codes_SRS_SM_02_026: [ If the number of executed APIs matches the waiting barrier then sm_end shall wake up the waiting barrier. ]*/
        if (get_n_minus_e(sm) == 1)
        {
            InterlockedHL_SetAndWake((void*)&sm->e_done, 1);
        }
    }
}

int sm_barrier_begin(SM_HANDLE sm)
{
    int result;
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
        result = MU_FAILURE;
    }
    else
    {
        result=sm_barrier_begin_internal(sm);
    }

    return result;
}

void sm_barrier_end(SM_HANDLE sm)
{
    /*Codes_SRS_SM_02_032: [ If sm is NULL then `sm_barrier_end shall return. ]*/
    if (sm == NULL)
    {
        LogError("invalid argument SM_HANDLE sm=%p", sm);
    }
    else
    {
        /*Codes_SRS_SM_02_033: [ sm_barrier_end shall increment the number of executed operations (e), switch b_now to INT64_MAX and return, ]*/
        InterlockedIncrement64(&sm->e);
        InterlockedExchange(&sm->e_done, 0);
        InterlockedIncrement64(&sm->b_now);
    }
}