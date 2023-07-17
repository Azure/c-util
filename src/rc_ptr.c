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

static void dispose(RC_PTR* rc_ptr)
{
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
        /*Codes_SRS_RC_PTR_43_002: [ rc_ptr_create_with_move_pointer shall allocate memory by calling THANDLE_MALLOC. ]*/
        THANDLE(RC_PTR) thandle = THANDLE_MALLOC(RC_PTR)(dispose);
        if (thandle == NULL)
        {
            /*Codes_SRS_RC_PTR_43_003: [ If there are any failures, rc_ptr_create_with_move_pointer shall fail and return NULL. ]*/
            LogError("Failure in THANDLE_MALLOC(RC_PTR)(dispose=%p)", dispose);
        }
        else
        {
            RC_PTR* rc_ptr = THANDLE_GET_T(RC_PTR)(thandle);
            rc_ptr->ptr = ptr;
            rc_ptr->free_func = free_func;
            /*Codes_SRS_RC_PTR_43_004: [ rc_ptr_create_with_move_pointer shall succeed and return a non-NULL value. ]*/
            THANDLE_INITIALIZE_MOVE(RC_PTR)(&result, &thandle);
        }
    }
    return result;
}
