// Copyright (c) Microsoft. All rights reserved.

#include <stdint.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/uuid_string.h"

#include "c_util/async_type_helper.h"

DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T, dest, source)
{
    int result;

    /*Codes_SRS_ASYNC_TYPE_HELPER_04_001: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
    if (dest == NULL)
    {
        LogError("Invalid args: UUID_T* dest = %p, const UUID_T source = %" PRI_UUID_T "",
            dest, UUID_T_VALUES_OR_NULL(source));
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_ASYNC_TYPE_HELPER_04_002: [ If source is NULL then the copy handler will set the dest to a zero UUID value. ]*/
        if (source == NULL)
        {
            (void)memcpy(*dest, NIL_UUID, sizeof(UUID_T));
        }
        else
        {
            /*Codes_SRS_ASYNC_TYPE_HELPER_04_003: [ Otherwise, the copy handler shall copy the UUID_T from source to dest. ]*/
            (void)memcpy(*dest, source, sizeof(UUID_T));
        }

        /*Codes_SRS_ASYNC_TYPE_HELPER_04_004: [ The copy handler shall succeed and return 0. ]*/
        result = 0;
    }

    return result;
}

DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T, value)
{
    /*Codes_SRS_ASYNC_TYPE_HELPER_04_005: [This handler shall do nothing.]*/
    (void)value;

    // We do nothing since we don't malloc a UUID_T in the copy handler.
}

int ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)(const_UUID_T* dest, const_UUID_T source)
{
    int result;

    /* Codes_SRS_ASYNC_TYPE_HELPER_01_001: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
    if (dest == NULL)
    {
        LogError("Invalid args: const_UUID_T* dest = %p, const const_UUID_T source = %" PRI_UUID_T "",
            dest, UUID_T_VALUES_OR_NULL(source));
        result = MU_FAILURE;
    }
    else
    {
        if (source == NULL)
        {
            /* Codes_SRS_ASYNC_TYPE_HELPER_01_002: [ If source is NULL then the copy handler will set the dest to a zero UUID value. ]*/
            (void)memcpy((void*)*dest, NIL_UUID, sizeof(const_UUID_T));
        }
        else
        {
            /* Codes_SRS_ASYNC_TYPE_HELPER_01_003: [ Otherwise, the copy handler shall copy the const UUID_T from source to dest. ]*/
            (void)memcpy((void*)*dest, source, sizeof(const_UUID_T));
        }

        /* Codes_SRS_ASYNC_TYPE_HELPER_01_004: [ The copy handler shall succeed and return 0. ]*/
        result = 0;
    }

    return result;
}

void ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T)(const_UUID_T value)
{
    /* Codes_SRS_ASYNC_TYPE_HELPER_01_005: [ This handler shall do nothing. ]*/
    (void)value;

    // We do nothing since we don't malloc a const_UUID_T in the copy handler.
}

DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t, dest, source)
{
    int result;

    if (
        /*Codes_SRS_ASYNC_TYPE_HELPER_42_006: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
        dest == NULL
        )
    {
        LogError("Invalid argument: char** dest = %p, const char* source = %p",
            dest, source);
        result = MU_FAILURE;
    }
    else if (source == NULL)
    {
        /*Codes_SRS_ASYNC_TYPE_HELPER_42_007: [ If source is NULL then the copy handler will set the dest to NULL and return 0. ]*/
        *dest = NULL;
        result = 0;
    }
    else
    {
        /*Codes_SRS_ASYNC_TYPE_HELPER_42_008: [ The copy handler shall allocate a string large enough to hold source, including the terminating NULL. ]*/
        size_t size_needed = strlen(source) + 1;
        *dest = malloc(size_needed);

        if (*dest == NULL)
        {
            /*Codes_SRS_ASYNC_TYPE_HELPER_42_010: [ If there are any failures then the copy handler shall fail and return a non-zero value. ]*/
            LogError("Failed to allocate copy of string malloc(%zu)", size_needed);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_ASYNC_TYPE_HELPER_42_009: [ The copy handler shall copy the string from source to dest. ]*/
            (void)strcpy(*dest, source);

            /*Codes_SRS_ASYNC_TYPE_HELPER_42_011: [ The copy handler shall succeed and return 0. ]*/
            result = 0;
        }
    }

    return result;
}

DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t, value)
{
    if (value == NULL)
    {
        /*Codes_SRS_ASYNC_TYPE_HELPER_42_012: [ If value is NULL then the free handler shall return. ]*/
        LogError("Invalid arg: char* value = %p", value);
    }
    else
    {
        /*Codes_SRS_ASYNC_TYPE_HELPER_42_013: [ The free handler shall free value. ]*/
        free(value);
    }
}

int constbuffer_array_ptr_copy(constbuffer_array_ptr* dest, const constbuffer_array_ptr src, uint32_t item_count)
{
    int result;

    if (
        /*Codes_SRS_ASYNC_TYPE_HELPER_28_001: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
        dest == NULL ||
        /*Codes_SRS_ASYNC_TYPE_HELPER_28_011: [ If item_count is 0, the copy handler will fail and return a non-zero value. ]*/
        item_count == 0)
    {
        LogError("Invalid argument: CONSTBUFFER_ARRAY_HANDLE** dest = %p, const CONSTBUFFER_ARRAY_HANDLE* src = %p, uin32_t item_count = %" PRIu32 "",
            dest, src, item_count);
        result = MU_FAILURE;
    }
    else if (src == NULL)
    {
        /*Codes_SRS_ASYNC_TYPE_HELPER_28_002: [ If src is NULL then the copy handler will set the dest to NULL and return 0. ]*/
        *dest = NULL;
        result = 0;
    }
    else
    {
        /*Codes_SRS_ASYNC_TYPE_HELPER_28_003: [ The copy handler shall allocate an array to store all the constbuffer_array in the src. ]*/
        *dest = malloc_2(item_count, sizeof(CONSTBUFFER_ARRAY_HANDLE));

        if (*dest == NULL)
        {
            /*Codes_SRS_ASYNC_TYPE_HELPER_28_004: [ If there are any failures then the copy handler shall fail and return a non-zero value. ]*/
            LogError("malloc_2(item_count=%" PRIu32 ", sizeof(CONSTBUFFER_ARRAY_HANDLE)=%zu) failed", item_count, sizeof(CONSTBUFFER_ARRAY_HANDLE));
            result = MU_FAILURE;
        }
        else
        {
            for (uint32_t i = 0; i < item_count; i++)
            {
                /*Codes_SRS_ASYNC_TYPE_HELPER_28_005: [ The copy handler shall call constbuffer_array_inc_ref on each constbuffer array in src. ]*/
                constbuffer_array_inc_ref(src[i]);
                /*Codes_SRS_ASYNC_TYPE_HELPER_28_006: [ The copy handler shall copy all the constbuffer_arrays from the src to the dest. ]*/
                (*dest)[i] = src[i];
            }
            /*Codes_SRS_ASYNC_TYPE_HELPER_28_007: [ The copy handler shall succeed and return 0. ]*/
            result = 0;
        }
    }

    return result;
}

void constbuffer_array_ptr_free(const constbuffer_array_ptr value, uint32_t item_count)
{
    if (
        /*Codes_SRS_ASYNC_TYPE_HELPER_28_008: [ If value is NULL then the free handler shall return. ]*/
        value == NULL ||
        /*Codes_SRS_ASYNC_TYPE_HELPER_28_012: [ If item_count is 0, the free handler shall return. ]*/
        item_count == 0)
    {
        LogError("Invalid arg: CONSTBUFFER_ARRAY_HANDLE* value = %p, uin32_t item_count = %" PRIu32 "", value, item_count);
    }
    else
    {
        for (uint32_t i = 0; i < item_count; i++)
        {
            /*Codes_SRS_ASYNC_TYPE_HELPER_28_009: [ The free handler shall call constbuffer_array_dec_ref on each constbuffer array in value. ]*/
            constbuffer_array_dec_ref(value[i]);
        }
        /*Codes_SRS_ASYNC_TYPE_HELPER_28_010: [ The free handler shall free value. ]*/
        free(value);
    }
}
