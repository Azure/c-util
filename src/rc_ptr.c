// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"

#include "c_util/rc_ptr.h"

THANDLE_TYPE_DEFINE(RC_PTR);

static void rc_ptr_dispose(RC_PTR* rc_ptr)
{
    /*Codes_SRS_RC_PTR_43_006: [ If free_func is not NULL, rc_ptr_dispose shall call free_func with the ptr. ]*/
    if (rc_ptr->free_func != NULL)
    {
        rc_ptr->free_func((void*)rc_ptr->ptr);
    }
}

THANDLE(RC_PTR) rc_ptr_create_with_move_pointer(void* ptr, RC_PTR_FREE_FUNC free_func)
{
    THANDLE(RC_PTR) result = NULL;
    /*Codes_SRS_RC_PTR_43_001: [ If ptr is NULL, rc_ptr_create_with_move_pointer shall fail and return NULL. ]*/
    if (ptr == NULL)
    {
        LogError("Invalid arguments: void* ptr=%p, RC_PTR_FREE_FUNC free_func = %p", ptr, free_func);
    }
    else
    {
        /*Codes_SRS_RC_PTR_43_002: [ rc_ptr_create_with_move_pointer create a THANDLE(RC_PTR) by calling THANDLE_MALLOC with rc_ptr_dispose as the dispose function. ]*/
        THANDLE(RC_PTR) thandle = THANDLE_MALLOC(RC_PTR)(rc_ptr_dispose);
        if (thandle == NULL)
        {
            /*Codes_SRS_RC_PTR_43_004: [ If there are any failures, rc_ptr_create_with_move_pointer shall fail and return NULL. ]*/
            LogError("Failure in THANDLE_MALLOC(RC_PTR)(rc_ptr_dispose=%p)", rc_ptr_dispose);
        }
        else
        {
            RC_PTR* rc_ptr = THANDLE_GET_T(RC_PTR)(thandle);

            /*Codes_SRS_RC_PTR_43_003: [ rc_ptr_create_with_move_pointer shall store the given ptr and free_func in the created THANDLE(RC_PTR). ]*/
            rc_ptr->ptr = ptr;
            rc_ptr->free_func = free_func;

            /*Codes_SRS_RC_PTR_43_005: [ rc_ptr_create_with_move_pointer shall succeed and return a non-NULL value. ]*/
            THANDLE_INITIALIZE_MOVE(RC_PTR)(&result, &thandle);
        }
    }
    return result;
}
