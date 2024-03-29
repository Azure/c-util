// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"

#include "c_util/rc_string.h"

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

static THANDLE(RC_STRING) rc_string_create_impl(const char* string)
{
    THANDLE(RC_STRING) result = NULL;

    /* Codes_SRS_RC_STRING_01_002: [ Otherwise, rc_string_create shall determine the length of string. ]*/
    size_t string_length = strlen(string);
    size_t string_length_with_terminator = string_length + 1;

    if (string_length_with_terminator > SIZE_MAX - (sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING)))
    {
        /* Codes_SRS_RC_STRING_07_010: [ If the resulting memory size requested for the THANDLE(RC_STRING)and string results in an size_t overflow, rc_string_create shall fail and return NULL. ]*/
        LogError("Size_t overflowed, extra size is %zu, string_length_with_terminator=%zu", sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + string_length_with_terminator, string_length_with_terminator);
    }
    else
    {
        /* Codes_SRS_RC_STRING_01_003: [ rc_string_create shall allocate memory for the THANDLE(RC_STRING), ensuring all the bytes in string can be copied (including the zero terminator). ]*/
        THANDLE(RC_STRING) temp_result = THANDLE_MALLOC_FLEX(RC_STRING)(rc_string_dispose, sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + string_length_with_terminator, 1);
        if (temp_result == NULL)
        {
            /* Codes_SRS_RC_STRING_01_006: [ If any error occurs, rc_string_create shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC_FLEX(RC_STRING) failed, extra size is %zu, string_length_with_terminator=%zu", sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + string_length_with_terminator, string_length_with_terminator);
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
        return rc_string_create_impl(string);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create_with_vformat, const char*, format, va_list, va)
{
    THANDLE(RC_STRING) result = NULL;

    if (format == NULL)
    {
        /*Codes_SRS_RC_STRING_07_001: [ If format is NULL, rc_string_create_with_vformat shall fail and return NULL. ]*/
        LogError("Invalid arguments: const char* format=%s", MU_P_OR_NULL(format));
    }
    else
    {
        /*Codes_SRS_RC_STRING_07_002: [ Otherwise, rc_string_create_with_vformat shall determine the total number of characters written using the variable number of arguments. ]*/
        va_list args_copy;
        va_copy(args_copy, va);

        int string_length = vsnprintf(NULL, 0, format, va);
        int string_length_with_terminator = string_length + 1;

        if (string_length < 0)
        {
            /*Codes_SRS_RC_STRING_07_003: [ If vsnprintf failed to determine the total number of characters written, rc_string_create_with_vformat shall fail and return NULL. ]*/
            LogError("vsnprintf failed to determine the total number of characters written.");
        }
        else
        {
            if (string_length == INT_MAX)
            {
                /*Codes_SRS_RC_STRING_07_009: [ If the resulting memory size requested for the THANDLE(RC_STRING)and the resulting formatted string results in an size_t overflow in malloc_flex, rc_string_create_with_vformat shall failand return NULL. ]*/
                LogError("int overflowed, extra size is %zu, string_length_with_terminator=%zu", sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + (size_t)string_length + 1, (size_t)string_length + 1);
            }
            else
            {
                /*Codes_SRS_RC_STRING_07_004: [ rc_string_create_with_vformat shall allocate memory for the THANDLE(RC_STRING)and the number of bytes for the resulting formatted string. ]*/
                THANDLE(RC_STRING) temp_result = THANDLE_MALLOC_FLEX(RC_STRING)(rc_string_dispose, sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + (size_t)string_length_with_terminator, 1);
                if (temp_result == NULL)
                {
                    /*Codes_SRS_RC_STRING_07_008: [ If any error occurs, rc_string_create_with_vformat shall fail and return NULL. ]*/
                    LogError("THANDLE_MALLOC_FLEX(RC_STRING) failed, extra size is %zu, string_length_with_terminator=%d", sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING) + (size_t)string_length_with_terminator, string_length_with_terminator);
                }
                else
                {
                    RC_STRING_INTERNAL* rc_string_internal = RC_STRING_INTERNAL_FROM_RC_STRING(THANDLE_GET_T(RC_STRING)(temp_result));
                    rc_string_internal->rc_string.string = rc_string_internal->copied_string;
                    rc_string_internal->storage_type = STRING_STORAGE_TYPE_COPIED;

                    /*Codes_SRS_RC_STRING_07_005: [ rc_string_create_with_vformat shall fill in the bytes of the string by using vsnprintf. ]*/
                    int copy_string_length = vsnprintf(rc_string_internal->copied_string, string_length_with_terminator, format, args_copy);
                    if (copy_string_length < 0)
                    {
                        /*Codes_SRS_RC_STRING_07_006: [ If vsnprintf failed to construct the resulting formatted string, rc_string_create_with_vformat shall fail and return NULL. ]*/
                        LogError("vsnprintf failed to get the resulting formatted string.");
                        THANDLE_FREE(RC_STRING)((void*)temp_result);
                    }
                    else
                    {
                        /*Codes_SRS_RC_STRING_07_007: [ rc_string_create_with_vformat shall succeed and return a non - NULL handle. ]*/
                        THANDLE_MOVE(RC_STRING)(&result, &temp_result);
                    }
                }
            }
        }
        va_end(args_copy);
    }
    return result;
}

THANDLE(RC_STRING) rc_string_create_with_format_function(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    THANDLE(RC_STRING) result = rc_string_create_with_vformat(format, va);
    va_end(va);
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
        THANDLE(RC_STRING) temp_result = THANDLE_MALLOC_FLEX(RC_STRING)(rc_string_dispose, sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING), 1);
        if (temp_result == NULL)
        {
            /* Codes_SRS_RC_STRING_01_011: [ If any error occurs, rc_string_create_with_move_memory shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC_FLEX(RC_STRING) failed");
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
        THANDLE(RC_STRING) temp_result = THANDLE_MALLOC_FLEX(RC_STRING)(rc_string_dispose, sizeof(RC_STRING_INTERNAL) - sizeof(RC_STRING), 1);
        if (temp_result == NULL)
        {
            /* Codes_SRS_RC_STRING_01_019: [ If any error occurs, rc_string_create_with_custom_free shall fail and return NULL. ]*/
            LogError("THANDLE_MALLOC_FLEX(RC_STRING) failed");
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


THANDLE(RC_STRING) rc_string_recreate(THANDLE(RC_STRING) self)
{
    THANDLE(RC_STRING) result = NULL;
    if (self == NULL)
    {
        /*Codes_SRS_RC_STRING_02_001: [ If source is NULL then rc_string_recreate shall return NULL. ]*/
        LogError("invalid argument THANDLE(RC_STRING) self=%p", self);
        /*return as is, that is, NULL*/
    }
    else
    {
        /*Codes_SRS_RC_STRING_02_002: [ rc_string_recreate shall perform same steps as rc_string_create to return a THANDLE(RC_STRING) with the same content as source. ]*/
        THANDLE(RC_STRING) temp = rc_string_create_impl(self->string);

        if (temp == NULL)
        {
            LogError("unable to recreate the string %" PRI_RC_STRING " with individual storage", RC_STRING_VALUE(self));
        }

        /*in any case (NULL, non-NULL) return it*/
        THANDLE_INITIALIZE_MOVE(RC_STRING)(&result, &temp);
    }
    return result;
}

