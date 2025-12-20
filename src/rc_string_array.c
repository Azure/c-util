// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"
#include "c_util/rc_string.h"

#include "c_util/rc_string_array.h"

RC_STRING_ARRAY* rc_string_array_create(uint32_t count)
{
    RC_STRING_ARRAY* result;

    /*Codes_SRS_RC_STRING_ARRAY_42_001: [ rc_string_array_create shall allocate memory for RC_STRING_ARRAY. ]*/
    result = malloc(sizeof(RC_STRING_ARRAY));

    if (result == NULL)
    {
        /*Codes_SRS_RC_STRING_ARRAY_42_002: [ If there are any errors then rc_string_array_create shall fail and return NULL. ]*/
        LogError("malloc(sizeof(RC_STRING_ARRAY)=%zu) failed",
            sizeof(RC_STRING_ARRAY));
    }
    else
    {
        result->string_array = NULL;
        bool failed = false;

        if (count > 0)
        {
            /*Codes_SRS_RC_STRING_ARRAY_42_008: [ rc_string_array_create shall allocate memory for count strings. ]*/
            result->string_array = malloc_2(count, sizeof(THANDLE(RC_STRING)));

            if (result->string_array == NULL)
            {
                /*Codes_SRS_RC_STRING_ARRAY_42_002: [ If there are any errors then rc_string_array_create shall fail and return NULL. ]*/
                LogError("malloc_2(count=%" PRIu32 ", sizeof(THANDLE(RC_STRING))=%zu);",
                    count, sizeof(THANDLE(RC_STRING)));
                failed = true;
            }
        }

        if (failed)
        {
            // already logged
        }
        else
        {
            // Need to cast to assign to the const member
            *(uint32_t*)&result->count = count;

            /*Codes_SRS_RC_STRING_ARRAY_42_007: [ rc_string_array_create shall initialize the strings in the array to NULL. ]*/
            for (uint32_t i = 0; i < count; ++i)
            {
                THANDLE_INITIALIZE(RC_STRING)(&result->string_array[i], NULL);
            }

            /*Codes_SRS_RC_STRING_ARRAY_42_003: [ rc_string_array_create shall succeed and return the allocated RC_STRING_ARRAY. ]*/
            goto all_ok;
        }
        free(result);
        result = NULL;
    }
all_ok:
    return result;
}

void rc_string_array_destroy(RC_STRING_ARRAY* rc_string_array)
{
    if (rc_string_array == NULL)
    {
        /*Codes_SRS_RC_STRING_ARRAY_42_004: [ If rc_string_array is NULL then rc_string_array_destroy shall fail and return. ]*/
        LogError("Invalid args: RC_STRING_ARRAY* rc_string_array=%p",
            rc_string_array);
    }
    else
    {
        /*Codes_SRS_RC_STRING_ARRAY_42_005: [ rc_string_array_destroy shall iterate over all of the elements in string_array and call THANDLE_ASSIGN(RC_STRING) with NULL. ]*/
        for (uint32_t i = 0; i < rc_string_array->count; ++i)
        {
            if (rc_string_array->string_array[i] != NULL)
            {
                THANDLE_ASSIGN(RC_STRING)(&rc_string_array->string_array[i], NULL);
            }
        }
        /*Codes_SRS_RC_STRING_ARRAY_42_006: [ rc_string_array_destroy shall free the memory allocated in rc_string_array. ]*/
        if (rc_string_array->string_array != NULL)
        {
            free((void*)rc_string_array->string_array);
        }
        free(rc_string_array);
    }
}
