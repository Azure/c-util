// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_macro_utils/macro_utils.h"

#include "azure_c_logging/xlogging.h"

#include "azure_c_pal/gballoc_hl.h"
#include "azure_c_pal/gballoc_hl_redirect.h"

#include "azure_c_util/thandle.h"

#include "azure_c_util/rc_string.h"

#define THANDLE_MALLOC_FUNCTION malloc
#define THANDLE_FREE_FUNCTION free
THANDLE_TYPE_DEFINE(RC_STRING);

#define STRING_STORAGE_TYPE_VALUES \
    STRING_STORAGE_TYPE_COPIED, \
    STRING_STORAGE_TYPE_MOVED, \
    STRING_STORAGE_TYPE_WITH_CUSTOM_FREE

MU_DEFINE_ENUM(STRING_STORAGE_TYPE, STRING_STORAGE_TYPE_VALUES)

// In order not to expose the storage type to the user (as it is something internal to the implementation),
// we're going to have another structure that wraps the RC_STRING and the storage type
// It is imperative that the first member is the RC_STRING structure though, so it matches the structure that
// we return to the user
typedef struct RC_STRING_INTERNAL_TAG
{
    RC_STRING rc_string;
    STRING_STORAGE_TYPE storage_type;
    RC_STRING_FREE_FUNC free_func;
    void* free_func_context;
    char copied_string[];
} RC_STRING_INTERNAL;

#define RC_STRING_INTERNAL_FROM_RC_STRING(rc_string_content) \
    (RC_STRING_INTERNAL*)(void*)(rc_string_content);

static void rc_string_dispose(RC_STRING* content)
{
    RC_STRING_INTERNAL* rc_string_internal = ((void*)content);
    switch (rc_string_internal->storage_type)
    {
    default:
        LogError("String created by Chuck Norris");
        break;

    case STRING_STORAGE_TYPE_COPIED:
        break;

    case STRING_STORAGE_TYPE_MOVED:
        /* Codes_SRS_RC_STRING_01_020: [ When the THANDLE(RC_STRING) reference count reaches 0, string shall be free with free. ]*/
        free((void*)rc_string_internal->rc_string.string);
        break;

    case STRING_STORAGE_TYPE_WITH_CUSTOM_FREE:
        /* Codes_SRS_RC_STRING_01_018: [ When the THANDLE(RC_STRING) reference count reaches 0, free_func shall be called with free_func_context to free the memory used by string. ]*/
        rc_string_internal->free_func(rc_string_internal->free_func_context);
        break;
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create, const char*, string)
{
    THANDLE(RC_STRING) result = NULL;

    if (string == NULL)
    {
        /*Codes_SRS_RC_STRING_01_001: [ If string is NULL, rc_string_create shall fail and return NULL. ]*/
        LogError("Invalid arguments: const char* string=%s", MU_P_OR_NULL(string));
    }
    else
    {
        /* Codes_SRS_RC_STRING_01_002: [ Otherwise, rc_string_create shall determine the length of string. ]*/
        size_t string_length = strlen(string);
        size_t string_length_with_terminator = string_length + 1;

        /* Codes_SRS_RC_STRING_01_003: [ rc_string_create shall allocate memory for the THANDLE(RC_STRING), ensuring all the bytes in string can be copied (including the zero terminator). ]*/
        THANDLE(RC_STRING) temp_result = THANDLE_MALLOC_WITH_EXTRA_SIZE(RC_STRING)(rc_string_dispose, sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + string_length_with_terminator);
        if (temp_result == NULL)
        {
            /* Codes_SRS_RC_STRING_01_006: [ If any error occurs, rc_string_create shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC_WITH_EXTRA_SIZE(RC_STRING) failed, extra size is %zu, string_length_with_terminator=%zu", sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + string_length_with_terminator, string_length_with_terminator);
        }
        else
        {
            RC_STRING_INTERNAL* rc_string_internal = RC_STRING_INTERNAL_FROM_RC_STRING(THANDLE_GET_T(RC_STRING)(temp_result));
            rc_string_internal->rc_string.string = rc_string_internal->copied_string;
            rc_string_internal->storage_type = STRING_STORAGE_TYPE_COPIED;

            /* Codes_SRS_RC_STRING_01_004: [ rc_string_create shall copy the string memory (including the NULL terminator). ]*/
            (void)memcpy(rc_string_internal->copied_string, string, string_length_with_terminator);

            /* Codes_SRS_RC_STRING_01_005: [ rc_string_create shall succeed and return a non-NULL handle. ]*/
            THANDLE_MOVE(RC_STRING)(&result, &temp_result);
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_move_memory, const char*, string)
{
    THANDLE(RC_STRING) result = NULL;

    if (string == NULL)
    {
        /* Codes_SRS_RC_STRING_01_007: [ If string is NULL, rc_string_create_with_move_memory shall fail and return NULL. ]*/
        LogError("Invalid arguments: const char* string=%s", MU_P_OR_NULL(string));
    }
    else
    {
        /* Codes_SRS_RC_STRING_01_008: [ Otherwise, rc_string_create_with_move_memory shall allocate memory for the THANDLE(RC_STRING). ]*/
        THANDLE(RC_STRING) temp_result = THANDLE_MALLOC_WITH_EXTRA_SIZE(RC_STRING)(rc_string_dispose, sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING));
        if (temp_result == NULL)
        {
            /* Codes_SRS_RC_STRING_01_011: [ If any error occurs, rc_string_create_with_move_memory shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC_WITH_EXTRA_SIZE(RC_STRING) failed");
        }
        else
        {
            RC_STRING_INTERNAL* rc_string_internal = RC_STRING_INTERNAL_FROM_RC_STRING(THANDLE_GET_T(RC_STRING)(temp_result));

            /* Codes_SRS_RC_STRING_01_009: [ rc_string_create_with_move_memory shall associate string with the new handle. ]*/
            rc_string_internal->rc_string.string = string;
            rc_string_internal->storage_type = STRING_STORAGE_TYPE_MOVED;

            /* Codes_SRS_RC_STRING_01_010: [ rc_string_create_with_move_memory shall succeed and return a non-NULL handle. ]*/
            THANDLE_MOVE(RC_STRING)(&result, &temp_result);
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_custom_free, const char*, string, RC_STRING_FREE_FUNC, free_func, void*, free_func_context)
{
    THANDLE(RC_STRING) result = NULL;

    /* Codes_SRS_RC_STRING_01_014: [ free_func_context shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_RC_STRING_01_012: [ If string is NULL, rc_string_create_with_custom_free shall fail and return NULL. ]*/
        (string == NULL) ||
        /* Codes_SRS_RC_STRING_01_013: [ If free_func is NULL, rc_string_create_with_custom_free shall fail and return NULL. ]*/
        (free_func == NULL)
        )
    {
        LogError("Invalid arguments: const char* string=%s, RC_STRING_FREE_FUNC free_func=%p, void* free_func_context=%p", MU_P_OR_NULL(string), free_func, free_func_context);
    }
    else
    {
        /* Codes_SRS_RC_STRING_01_015: [ rc_string_create_with_custom_free shall allocate memory for the THANDLE(RC_STRING). ]*/
        THANDLE(RC_STRING) temp_result = THANDLE_MALLOC_WITH_EXTRA_SIZE(RC_STRING)(rc_string_dispose, sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING));
        if (temp_result == NULL)
        {
            /* Codes_SRS_RC_STRING_01_019: [ If any error occurs, rc_string_create_with_custom_free shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC_WITH_EXTRA_SIZE(RC_STRING) failed");
        }
        else
        {
            RC_STRING_INTERNAL* rc_string_internal = RC_STRING_INTERNAL_FROM_RC_STRING(THANDLE_GET_T(RC_STRING)(temp_result));

            /* Codes_SRS_RC_STRING_01_016: [ rc_string_create_with_custom_free shall associate string, free_func and free_func_context with the new handle. ]*/
            rc_string_internal->rc_string.string = string;
            rc_string_internal->storage_type = STRING_STORAGE_TYPE_WITH_CUSTOM_FREE;
            rc_string_internal->free_func = free_func;
            rc_string_internal->free_func_context = free_func_context;

            /* Codes_SRS_RC_STRING_01_017: [ rc_string_create_with_custom_free shall succeed and return a non-NULL handle. ]*/
            THANDLE_MOVE(RC_STRING)(&result, &temp_result);
        }
    }

    return result;
}
