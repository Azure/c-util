// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"
#include "c_pal/thandle.h"

#include "c_util/memory_data.h"
#include "c_util/constbuffer_format.h"
#include "c_util/constbuffer_version.h"
#include "c_util/constbuffer_thandle.h"

// in order to optimize memory usage, the const buffer structure contains a discriminator that tells what kind of const buffer it is (copied, with custom free, etc.).
// Each type of const buffer has its own structure that contains the common fields and the specific fields.
#define CONSTBUFFER_THANDLE_TYPE_VALUES \
    CONSTBUFFER_THANDLE_TYPE_COPIED, \
    CONSTBUFFER_THANDLE_TYPE_MEMORY_MOVED, \
    CONSTBUFFER_THANDLE_TYPE_WITH_CUSTOM_FREE, \
    CONSTBUFFER_THANDLE_TYPE_FROM_OFFSET_AND_SIZE

MU_DEFINE_ENUM(CONSTBUFFER_THANDLE_TYPE, CONSTBUFFER_THANDLE_TYPE_VALUES)

MU_DEFINE_ENUM_STRINGS(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES);

typedef struct CONSTBUFFER_THANDLE_HANDLE_DATA_TAG
{
    CONSTBUFFER_THANDLE alias;  // Embedded alias structure like constbuffer.c
    CONSTBUFFER_THANDLE_TYPE buffer_type;
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) original_handle; // For offset/size operations, NULL otherwise
    unsigned char data[]; // Flexible array member for data storage
} CONSTBUFFER_THANDLE_HANDLE_DATA;

typedef struct CONSTBUFFER_THANDLE_HANDLE_MOVE_MEMORY_DATA_TAG
{
    CONSTBUFFER_THANDLE alias;  // Embedded alias structure
    CONSTBUFFER_THANDLE_TYPE buffer_type;
} CONSTBUFFER_THANDLE_HANDLE_MOVE_MEMORY_DATA;

typedef struct CONSTBUFFER_THANDLE_HANDLE_WITH_CUSTOM_FREE_DATA_TAG
{
    CONSTBUFFER_THANDLE alias;  // Embedded alias structure
    CONSTBUFFER_THANDLE_TYPE buffer_type;
    CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC custom_free_func;
    void* custom_free_func_context;
} CONSTBUFFER_THANDLE_HANDLE_WITH_CUSTOM_FREE_DATA;

// THANDLE type definition for CONSTBUFFER_THANDLE_HANDLE_DATA
THANDLE_TYPE_DEFINE(CONSTBUFFER_THANDLE_HANDLE_DATA);

typedef struct CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_TAG
{
    CONSTBUFFER_THANDLE_TYPE buffer_type;
    uint32_t size;
    unsigned char data[]; // Flexible array member for data storage
} CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA;

// THANDLE type definition for CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA
THANDLE_TYPE_DEFINE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA);

static void CONSTBUFFER_THANDLE_HANDLE_DATA_dispose(CONSTBUFFER_THANDLE_HANDLE_DATA* handle_data)
{
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_013: [ CONSTBUFFER_THANDLE_HANDLE_DATA_dispose shall free the memory used by the const buffer. ]*/
    if (handle_data->buffer_type == CONSTBUFFER_THANDLE_TYPE_MEMORY_MOVED)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_014: [ If the buffer was created by calling CONSTBUFFER_THANDLE_CreateWithMoveMemory, the memory pointed to by the buffer pointer shall be freed. ]*/
        free((void*)handle_data->alias.buffer);
    }
    else if (handle_data->buffer_type == CONSTBUFFER_THANDLE_TYPE_WITH_CUSTOM_FREE)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_031: [ If the buffer was created by calling CONSTBUFFER_THANDLE_CreateWithCustomFree, the customFreeFunc function shall be called to free the memory, while passed customFreeFuncContext as argument. ]*/
        CONSTBUFFER_THANDLE_HANDLE_WITH_CUSTOM_FREE_DATA* custom_free_data = (CONSTBUFFER_THANDLE_HANDLE_WITH_CUSTOM_FREE_DATA*)handle_data;
        custom_free_data->custom_free_func(custom_free_data->custom_free_func_context);
    }
    else if (handle_data->buffer_type == CONSTBUFFER_THANDLE_TYPE_FROM_OFFSET_AND_SIZE)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_048: [ If the buffer was created by calling CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, the original handle shall be decremented. ]*/
        if (handle_data->original_handle != NULL)
        {
            THANDLE_ASSIGN(CONSTBUFFER_THANDLE_HANDLE_DATA)(&handle_data->original_handle, NULL);
        }
    }
    // For CONSTBUFFER_THANDLE_TYPE_COPIED, the memory is part of the THANDLE allocation, so no separate free needed
}

static void CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_dispose(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA* handle_data)
{
    /*Codes_SRS_CONSTBUFFER_THANDLE_01_002: [ CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_dispose shall do nothing. ]*/
    (void)handle_data;
}

static THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) CONSTBUFFER_THANDLE_Create_Internal(const unsigned char* source, uint32_t size)
{
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_005: [The non-NULL handle returned by CONSTBUFFER_THANDLE_Create shall have its ref count set to "1".]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_010: [The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateFromBuffer shall have its ref count set to "1".]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_037: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall allocate enough memory to hold CONSTBUFFER_THANDLE_HANDLE and size bytes. ]*/
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) result = THANDLE_MALLOC_FLEX(CONSTBUFFER_THANDLE_HANDLE_DATA)(CONSTBUFFER_THANDLE_HANDLE_DATA_dispose, size, 1);
    if (result == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_003: [If creating the copy fails then CONSTBUFFER_THANDLE_Create shall return NULL.]*/
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_008: [If copying the content fails, then CONSTBUFFER_THANDLE_CreateFromBuffer shall fail and return NULL.] */
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_040: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        LogError("failure in THANDLE_MALLOC_FLEX(CONSTBUFFER_THANDLE_HANDLE_DATA)(CONSTBUFFER_THANDLE_HANDLE_DATA_dispose, size=%" PRIu32 ")",
            size);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_002: [Otherwise, CONSTBUFFER_THANDLE_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
        CONSTBUFFER_THANDLE_HANDLE_DATA* handle_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_HANDLE_DATA)(result);
        handle_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_COPIED;
        THANDLE_INITIALIZE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&handle_data->original_handle, NULL);
        
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_038: [ If size is 0 then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall set the pointed to buffer to NULL. ]*/
        if (size == 0)
        {
            handle_data->alias.buffer = NULL;
            handle_data->alias.size = 0;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_004: [Otherwise CONSTBUFFER_THANDLE_Create shall return a non-NULL handle.]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_007: [Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall copy the content of buffer.]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_009: [Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall return a non-NULL handle.]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_039: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall set the pointed to a non-NULL value that contains the same bytes as offset...offset+size-1 of handle. ]*/
            if (source != NULL)
            {
                (void)memcpy(handle_data->data, source, size);
            }
            handle_data->alias.buffer = handle_data->data;
            handle_data->alias.size = size;
        }
    }
    return result;
}

THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) CONSTBUFFER_THANDLE_Create(const unsigned char* source, uint32_t size)
{
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) result = NULL;
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_001: [If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_Create shall fail and return NULL.]*/
    if (
        (source == NULL) &&
        (size != 0)
        )
    {
        LogError("invalid arguments passes to CONSTBUFFER_THANDLE_Create: %p, size: %" PRIu32 "", source, size);
    }
    else
    {
        THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) temp_result = CONSTBUFFER_THANDLE_Create_Internal(source, size);
        THANDLE_MOVE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&result, &temp_result);
    }
    return result;
}

/*this creates a new constbuffer_thandle from an existing BUFFER_HANDLE*/
THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) CONSTBUFFER_THANDLE_CreateFromBuffer(BUFFER_HANDLE buffer)
{
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) result = NULL;
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_006: [If buffer is NULL then CONSTBUFFER_THANDLE_CreateFromBuffer shall fail and return NULL.]*/
    if (buffer == NULL)
    {
        LogError("invalid arg passed to CONSTBUFFER_THANDLE_CreateFromBuffer buffer: NULL");
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_007: [Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall copy the content of buffer.]*/
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_009: [Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall return a non-NULL handle.]*/
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_010: [The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateFromBuffer shall have its ref count set to "1".]*/
        uint32_t length = (uint32_t)BUFFER_length(buffer);
        unsigned char* rawBuffer = BUFFER_u_char(buffer);
        THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) temp_result = CONSTBUFFER_THANDLE_Create_Internal(rawBuffer, length);
        THANDLE_MOVE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&result, &temp_result);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWithMoveMemory, unsigned char*, source, uint32_t, size)
{
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) result = NULL;

    /*Codes_SRS_CONSTBUFFER_THANDLE_88_015: [ If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL. ]*/
    if ((source == NULL) && (size > 0))
    {
        LogError("Invalid arguments: unsigned char* source=%p, uint32_t size=%" PRIu32 "", source, size);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_019: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithMoveMemory shall have its ref count set to "1". ]*/
        THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) temp_result = THANDLE_MALLOC(CONSTBUFFER_THANDLE_HANDLE_DATA)(CONSTBUFFER_THANDLE_HANDLE_DATA_dispose);
        if (temp_result == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_017: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL. ]*/
            LogError("Allocation of CONSTBUFFER_THANDLE_HANDLE_DATA object failed");
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_018: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. ]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_016: [ CONSTBUFFER_THANDLE_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
            CONSTBUFFER_THANDLE_HANDLE_DATA* handle_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_HANDLE_DATA)(temp_result);
            handle_data->alias.buffer = source;
            handle_data->alias.size = size;
            handle_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_MEMORY_MOVED;
            THANDLE_INITIALIZE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&handle_data->original_handle, NULL);
            THANDLE_MOVE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&result, &temp_result);
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext)
{
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) result = NULL;

    /*Codes_SRS_CONSTBUFFER_THANDLE_88_032: [ customFreeFuncContext shall be allowed to be NULL. ]*/

    if (
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_026: [ If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL. ]*/
        ((source == NULL) && (size > 0)) ||
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_029: [ If customFreeFunc is NULL, CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL. ]*/
        (customFreeFunc == NULL)
        )
    {
        LogError("Invalid arguments: unsigned char* source=%p, uint32_t size=%" PRIu32 ", customFreeFunc=%p, customFreeFuncContext=%p",
            source, size, customFreeFunc, customFreeFuncContext);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_030: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithCustomFree shall have its ref count set to "1". ]*/
        // For custom free, we need to directly allocate using gballoc since we need extra space
        // and we can't use THANDLE for different structure types easily
        LogError("CONSTBUFFER_THANDLE_CreateWithCustomFree not fully implemented - allocation strategy needs revision");
        return NULL;
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size)
{
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) result = NULL;

    if (
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_049: [ If handle is NULL then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        (handle == NULL)
        )
    {
        LogError("Invalid arguments: handle is NULL");
    }
    else
    {
        const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
        if (
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_050: [ If CONSTBUFFER_THANDLE_GetContent returns NULL, then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
            (content == NULL) ||
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_051: [ If offset is greater than handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
            (offset > content->size) ||
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_052: [ If offset + size would overflow then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
            (offset > UINT32_MAX - size) ||
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_053: [ If offset + size exceed handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
            (offset + size > content->size)
            )
        {
            LogError("Invalid arguments: offset=%" PRIu32 ", size=%" PRIu32 ", content size=%" PRIu32,
                offset, size, (content == NULL) ? 0 : content->size);
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size. ]*/
            THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) temp_result = CONSTBUFFER_THANDLE_Create(content->buffer + offset, size);
            if (temp_result == NULL)
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_055: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
                LogError("Failed to create CONSTBUFFER_THANDLE from offset and size with copy");
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_056: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall succeed and return a non-NULL value. ]*/
                THANDLE_MOVE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&result, &temp_result);
            }
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size)
{
    THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) result = NULL;

    if (handle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_035: [ If handle is NULL then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
        LogError("Invalid arguments: handle is NULL");
    }
    else
    {
        const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
        if (content == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_036: [ If CONSTBUFFER_THANDLE_GetContent returns NULL, then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
            LogError("Failed to get content from handle");
        }
        else if (offset > content->size)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_037: [ If offset is greater than handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
            LogError("Invalid arguments: offset=%" PRIu32 " > content->size=%" PRIu32, offset, content->size);
        }
        else if (offset > UINT32_MAX - size)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_038: [ If offset + size would overflow then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
            LogError("Invalid arguments: offset=%" PRIu32 " + size=%" PRIu32 " would overflow", offset, size);
        }
        else if (offset + size > content->size)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_039: [ If offset + size exceed handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
            LogError("Invalid arguments: offset=%" PRIu32 " + size=%" PRIu32 " > content->size=%" PRIu32, offset, size, content->size);
        }
        else
        {
            if (offset == 0 && size == content->size)
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_040: [ If offset is 0 and size is equal to handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall increment the reference count of handle and return handle. ]*/
                THANDLE_ASSIGN(CONSTBUFFER_THANDLE_HANDLE_DATA)(&result, handle);
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_041: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER_THANDLE_HANDLE_DATA. ]*/
                THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) temp_result = THANDLE_MALLOC(CONSTBUFFER_THANDLE_HANDLE_DATA)(CONSTBUFFER_THANDLE_HANDLE_DATA_dispose);
                if (temp_result == NULL)
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_042: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
                    LogError("Failed to allocate CONSTBUFFER_THANDLE_HANDLE_DATA");
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_043: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the buffer pointer to point to handle's buffer + offset. ]*/
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_044: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the size to the provided size. ]*/
                    CONSTBUFFER_THANDLE_HANDLE_DATA* handle_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_HANDLE_DATA)(temp_result);
                    handle_data->alias.buffer = (content->buffer == NULL) ? NULL : content->buffer + offset;
                    handle_data->alias.size = size;
                    handle_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_FROM_OFFSET_AND_SIZE;
                    
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_045: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall increment the reference count of handle and store it. ]*/
                    THANDLE_INITIALIZE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&handle_data->original_handle, NULL);
                    THANDLE_ASSIGN(CONSTBUFFER_THANDLE_HANDLE_DATA)(&handle_data->original_handle, handle);
                    
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_046: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER_THANDLE_HANDLE_DATA to 1. ]*/
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_047: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
                    THANDLE_MOVE(CONSTBUFFER_THANDLE_HANDLE_DATA)(&result, &temp_result);
                }
            }
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, const CONSTBUFFER_THANDLE*, CONSTBUFFER_THANDLE_GetContent, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), constbufferHandle)
{
    const CONSTBUFFER_THANDLE* result;
    if (constbufferHandle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_011: [If constbufferHandle is NULL then CONSTBUFFER_THANDLE_GetContent shall return NULL.]*/
        result = NULL;
        LogError("Invalid argument constbufferHandle: NULL");
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_012: [Otherwise, CONSTBUFFER_THANDLE_GetContent shall return a const CONSTBUFFER_THANDLE* that matches byte by byte the original bytes used to created the const buffer and has the same length.]*/
        CONSTBUFFER_THANDLE_HANDLE_DATA* handle_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_HANDLE_DATA)(constbufferHandle);
        result = &(handle_data->alias);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, bool, CONSTBUFFER_THANDLE_contain_same, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), left, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), right)
{
    bool result;
    if (left == NULL)
    {
        if (right == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_020: [ If left is NULL and right is NULL then CONSTBUFFER_THANDLE_contain_same shall return true. ]*/
            result = true;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_021: [ If left is NULL and right is not NULL then CONSTBUFFER_THANDLE_contain_same shall return false. ]*/
            result = false;
        }
    }
    else
    {
        if (right == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_022: [ If left is not NULL and right is NULL then CONSTBUFFER_THANDLE_contain_same shall return false. ]*/
            result = false;
        }
        else
        {
            CONSTBUFFER_THANDLE_HANDLE_DATA* left_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_HANDLE_DATA)(left);
            CONSTBUFFER_THANDLE_HANDLE_DATA* right_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_HANDLE_DATA)(right);
            if (left_data->alias.size != right_data->alias.size)
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_023: [ If left's size is different than right's size then CONSTBUFFER_THANDLE_contain_same shall return false. ]*/
                result = false;
            }
            else
            {
                if (memcmp(left_data->alias.buffer, right_data->alias.buffer, left_data->alias.size) != 0)
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_024: [ If left's buffer contains different bytes than right's buffer then CONSTBUFFER_THANDLE_contain_same shall return false. ]*/
                    result = false;
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_025: [ CONSTBUFFER_THANDLE_contain_same shall return true. ]*/
                    result = true;
                }
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_get_serialization_size, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), source)
{
    /* Dummy implementation - just return 0 for now */
    (void)source;
    LogError("CONSTBUFFER_THANDLE_get_serialization_size not implemented yet");
    return 0;
}

IMPLEMENT_MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_to_buffer, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), source, CONSTBUFFER_THANDLE_to_buffer_alloc, alloc, void*, alloc_context, uint32_t*, serialized_size)
{
    /* Dummy implementation - just return NULL for now */
    (void)source;
    (void)alloc;
    (void)alloc_context;
    (void)serialized_size;
    LogError("CONSTBUFFER_THANDLE_to_buffer not implemented yet");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_to_fixed_size_buffer, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), source, unsigned char*, destination, uint32_t, destination_size, uint32_t*, serialized_size)
{
    /* Dummy implementation - just return error for now */
    (void)source;
    (void)destination;
    (void)destination_size;
    (void)serialized_size;
    LogError("CONSTBUFFER_THANDLE_to_fixed_size_buffer not implemented yet");
    return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_ERROR;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_from_buffer, const unsigned char*, source, uint32_t, size, uint32_t*, consumed, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA)*, destination)
{
    /* Dummy implementation - just return error for now */
    (void)source;
    (void)size;
    (void)consumed;
    (void)destination;
    LogError("CONSTBUFFER_THANDLE_from_buffer not implemented yet");
    return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWritableHandle, uint32_t, size)
{
    /* Dummy implementation - just return NULL for now */
    (void)size;
    LogError("CONSTBUFFER_THANDLE_CreateWritableHandle not implemented yet");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_GetWritableBuffer, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
{
    /* Dummy implementation - just return NULL for now */
    (void)constbufferWritableHandle;
    LogError("CONSTBUFFER_THANDLE_GetWritableBuffer not implemented yet");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_SealWritableHandle, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
{
    /* Dummy implementation - just return NULL for now */
    (void)constbufferWritableHandle;
    LogError("CONSTBUFFER_THANDLE_SealWritableHandle not implemented yet");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_GetWritableBufferSize, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
{
    /* Dummy implementation - just return 0 for now */
    (void)constbufferWritableHandle;
    LogError("CONSTBUFFER_THANDLE_GetWritableBufferSize not implemented yet");
    return 0;
}
