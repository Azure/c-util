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

static void* calls_malloc(size_t size, void* context)
{
    (void)context;
    return malloc(size); /*in all the contexts where this is called, it is verified that the size is 0..UINT32_MAX, which is a subset of 0..SIZE_MAX*/
}

// in order to optimize memory usage, the const buffer structure contains a discriminator that tells what kind of const buffer it is (copied, with custom free, etc.).
// Each type of const buffer has its own structure that contains the common fields and the specific fields.
#define CONSTBUFFER_THANDLE_TYPE_VALUES \
    CONSTBUFFER_THANDLE_TYPE_COPIED, \
    CONSTBUFFER_THANDLE_TYPE_MEMORY_MOVED, \
    CONSTBUFFER_THANDLE_TYPE_WITH_CUSTOM_FREE, \
    CONSTBUFFER_THANDLE_TYPE_FROM_OFFSET_AND_SIZE

MU_DEFINE_ENUM(CONSTBUFFER_THANDLE_TYPE, CONSTBUFFER_THANDLE_TYPE_VALUES)

MU_DEFINE_ENUM_STRINGS(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES);

typedef struct CONSTBUFFER_TAG
{
    CONSTBUFFER_THANDLE alias;  // Embedded alias structure like constbuffer.c
    CONSTBUFFER_THANDLE_TYPE buffer_type;
    THANDLE(CONSTBUFFER) original_handle; // For offset/size operations, NULL otherwise
    unsigned char data[]; // Flexible array member for data storage
} CONSTBUFFER;

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

// THANDLE type definition for CONSTBUFFER
THANDLE_TYPE_DEFINE(CONSTBUFFER);

typedef struct CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_TAG
{
    CONSTBUFFER_THANDLE_TYPE buffer_type;
    uint32_t size;
    unsigned char data[]; // Flexible array member for data storage
} CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA;

// THANDLE type definition for CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA
THANDLE_TYPE_DEFINE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA);

static void CONSTBUFFER_dispose(CONSTBUFFER* handle_data)
{
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_013: [ CONSTBUFFER_dispose shall free the memory used by the const buffer. ]*/
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
            THANDLE_ASSIGN(CONSTBUFFER)(&handle_data->original_handle, NULL);
        }
    }
    // For CONSTBUFFER_THANDLE_TYPE_COPIED, the memory is part of the THANDLE allocation, so no separate free needed
}

static void CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_dispose(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA* handle_data)
{
    // No specific disposal required for writable handle data
    (void)handle_data;
}

static THANDLE(CONSTBUFFER) CONSTBUFFER_THANDLE_Create_Internal(const unsigned char* source, uint32_t size)
{
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_005: [The non-NULL handle returned by CONSTBUFFER_THANDLE_Create shall have its ref count set to "1".]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_010: [The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateFromBuffer shall have its ref count set to "1".]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_030: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithCustomFree shall have its ref count set to "1". ]*/
    THANDLE(CONSTBUFFER) result = THANDLE_MALLOC_FLEX(CONSTBUFFER)(CONSTBUFFER_dispose, size, 1);
    if (result == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_003: [If creating the copy fails then CONSTBUFFER_THANDLE_Create shall return NULL.]*/
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_008: [If copying the content fails, then CONSTBUFFER_THANDLE_CreateFromBuffer shall fail and return NULL.] */
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_055: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        LogError("failure in THANDLE_MALLOC_FLEX(CONSTBUFFER)(CONSTBUFFER_dispose, size=%" PRIu32 ")",
            size);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_002: [Otherwise, CONSTBUFFER_THANDLE_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
        CONSTBUFFER* handle_data = THANDLE_GET_T(CONSTBUFFER)(result);
        handle_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_COPIED;
        THANDLE_INITIALIZE(CONSTBUFFER)(&handle_data->original_handle, NULL);
        
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size. ]*/
        if (size == 0)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_102: [If source is non-NULL and size is 0 then CONSTBUFFER_THANDLE_Create shall create an empty buffer.]*/
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

THANDLE(CONSTBUFFER) CONSTBUFFER_THANDLE_Create(const unsigned char* source, uint32_t size)
{
    THANDLE(CONSTBUFFER) result = NULL;
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
        THANDLE(CONSTBUFFER) temp_result = CONSTBUFFER_THANDLE_Create_Internal(source, size);
        THANDLE_MOVE(CONSTBUFFER)(&result, &temp_result);
    }
    return result;
}

/*this creates a new constbuffer_thandle from an existing BUFFER_HANDLE*/
THANDLE(CONSTBUFFER) CONSTBUFFER_THANDLE_CreateFromBuffer(BUFFER_HANDLE buffer)
{
    THANDLE(CONSTBUFFER) result = NULL;
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
        THANDLE(CONSTBUFFER) temp_result = CONSTBUFFER_THANDLE_Create_Internal(rawBuffer, length);
        THANDLE_MOVE(CONSTBUFFER)(&result, &temp_result);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateWithMoveMemory, unsigned char*, source, uint32_t, size)
{
    THANDLE(CONSTBUFFER) result = NULL;

    /*Codes_SRS_CONSTBUFFER_THANDLE_88_015: [ If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL. ]*/
    if ((source == NULL) && (size > 0))
    {
        LogError("Invalid arguments: unsigned char* source=%p, uint32_t size=%" PRIu32 "", source, size);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_019: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithMoveMemory shall have its ref count set to "1". ]*/
        THANDLE(CONSTBUFFER) temp_result = THANDLE_MALLOC(CONSTBUFFER)(CONSTBUFFER_dispose);
        if (temp_result == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_017: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL. ]*/
            LogError("Allocation of CONSTBUFFER object failed");
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_018: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. ]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_016: [ CONSTBUFFER_THANDLE_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
            CONSTBUFFER* handle_data = THANDLE_GET_T(CONSTBUFFER)(temp_result);
            handle_data->alias.buffer = source;
            handle_data->alias.size = size;
            handle_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_MEMORY_MOVED;
            THANDLE_INITIALIZE(CONSTBUFFER)(&handle_data->original_handle, NULL);
            THANDLE_MOVE(CONSTBUFFER)(&result, &temp_result);
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext)
{
    THANDLE(CONSTBUFFER) result = NULL;

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
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_033: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL. ]*/
        // Allocate extra space for the custom free function fields beyond the base structure
        size_t extra_size = sizeof(CONSTBUFFER_THANDLE_HANDLE_WITH_CUSTOM_FREE_DATA) - sizeof(CONSTBUFFER);
        THANDLE(CONSTBUFFER) temp_result = THANDLE_MALLOC_FLEX(CONSTBUFFER)(CONSTBUFFER_dispose, 1, extra_size);
        if (temp_result == NULL)
        {
            LogError("failure in THANDLE_MALLOC_FLEX");
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_027: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. ]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_028: [ CONSTBUFFER_THANDLE_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
            CONSTBUFFER_THANDLE_HANDLE_WITH_CUSTOM_FREE_DATA* custom_free_data = (CONSTBUFFER_THANDLE_HANDLE_WITH_CUSTOM_FREE_DATA*)THANDLE_GET_T(CONSTBUFFER)(temp_result);
            custom_free_data->alias.buffer = source;
            custom_free_data->alias.size = size;
            custom_free_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_WITH_CUSTOM_FREE;

            /*Codes_SRS_CONSTBUFFER_THANDLE_88_034: [ CONSTBUFFER_THANDLE_CreateWithCustomFree shall store customFreeFunc and customFreeFuncContext in order to use them to free the memory when the const buffer resources are freed. ]*/
            custom_free_data->custom_free_func = customFreeFunc;
            custom_free_data->custom_free_func_context = customFreeFuncContext;

            /*Codes_SRS_CONSTBUFFER_THANDLE_88_030: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithCustomFree shall have its ref count set to "1". ]*/
            THANDLE_MOVE(CONSTBUFFER)(&result, &temp_result);
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy, THANDLE(CONSTBUFFER), handle, uint32_t, offset, uint32_t, size)
{
    THANDLE(CONSTBUFFER) result = NULL;

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
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_051: [ If offset is greater than handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
            (offset > content->size) ||
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_052: [ If offset + size would overflow then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
            (offset > UINT32_MAX - size) ||
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_053: [ If offset + size exceed handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
            (offset + size > content->size)
            )
        {
            LogError("Invalid arguments: offset=%" PRIu32 ", size=%" PRIu32 ", content size=%" PRIu32,
                offset, size, content->size);
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size. ]*/
            THANDLE(CONSTBUFFER) temp_result = CONSTBUFFER_THANDLE_Create(content->buffer + offset, size);
            if (temp_result == NULL)
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_055: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
                LogError("Failed to create CONSTBUFFER_THANDLE from offset and size with copy");
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_056: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall succeed and return a non-NULL value. ]*/
                THANDLE_MOVE(CONSTBUFFER)(&result, &temp_result);
            }
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, THANDLE(CONSTBUFFER), handle, uint32_t, offset, uint32_t, size)
{
    THANDLE(CONSTBUFFER) result = NULL;

    if (handle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_035: [ If handle is NULL then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
        LogError("Invalid arguments: handle is NULL");
    }
    else
    {
        const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
        if (offset > content->size)
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
                THANDLE_ASSIGN(CONSTBUFFER)(&result, handle);
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_041: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER. ]*/
                THANDLE(CONSTBUFFER) temp_result = THANDLE_MALLOC(CONSTBUFFER)(CONSTBUFFER_dispose);
                if (temp_result == NULL)
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_042: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL. ]*/
                    LogError("Failed to allocate CONSTBUFFER");
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_043: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the buffer pointer to point to handle's buffer + offset. ]*/
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_044: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the size to the provided size. ]*/
                    CONSTBUFFER* handle_data = THANDLE_GET_T(CONSTBUFFER)(temp_result);
                    handle_data->alias.buffer = (content->buffer == NULL) ? NULL : content->buffer + offset;
                    handle_data->alias.size = size;
                    handle_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_FROM_OFFSET_AND_SIZE;
                    
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_045: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall increment the reference count of handle and store it. ]*/
                    THANDLE_INITIALIZE(CONSTBUFFER)(&handle_data->original_handle, NULL);
                    THANDLE_ASSIGN(CONSTBUFFER)(&handle_data->original_handle, handle);
                    
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_046: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER to 1. ]*/
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_047: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
                    THANDLE_MOVE(CONSTBUFFER)(&result, &temp_result);
                }
            }
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, const CONSTBUFFER_THANDLE*, CONSTBUFFER_THANDLE_GetContent, THANDLE(CONSTBUFFER), constbufferHandle)
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
        CONSTBUFFER* handle_data = THANDLE_GET_T(CONSTBUFFER)(constbufferHandle);
        result = &(handle_data->alias);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, bool, CONSTBUFFER_THANDLE_contain_same, THANDLE(CONSTBUFFER), left, THANDLE(CONSTBUFFER), right)
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
            CONSTBUFFER* left_data = THANDLE_GET_T(CONSTBUFFER)(left);
            CONSTBUFFER* right_data = THANDLE_GET_T(CONSTBUFFER)(right);
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
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_025: [ Otherwise, CONSTBUFFER_THANDLE_contain_same shall return true. ]*/
                    result = true;
                }
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_get_serialization_size, THANDLE(CONSTBUFFER), source)
{
    uint32_t result;
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_057: [ If source is NULL then CONSTBUFFER_THANDLE_get_serialization_size shall fail and return 0. ]*/
    if (source == NULL)
    {
        LogError("invalid argument THANDLE(CONSTBUFFER) source=%p", source);
        result = 0;
    }
    else
    {
        const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(source);
        if (content == NULL)
        {
            LogError("CONSTBUFFER_THANDLE_GetContent failed");
            result = 0;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_058: [ If sizeof(uint8_t) + sizeof(uint32_t) + source's size exceed UINT32_MAX then CONSTBUFFER_THANDLE_get_serialization_size shall fail and return 0. ]*/
            if (content->size > UINT32_MAX - CONSTBUFFER_VERSION_SIZE - CONSTBUFFER_SIZE_SIZE)
            {
                LogError("serialization size exceeds UINT32_MAX=%" PRIu32 ". It is the sum of sizeof(uint8_t)=%zu + sizeof(uint32_t)=%zu + source->size=%" PRIu32 "",
                    UINT32_MAX, CONSTBUFFER_VERSION_SIZE, CONSTBUFFER_SIZE_SIZE, content->size);
                result = 0;
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_059: [ Otherwise CONSTBUFFER_THANDLE_get_serialization_size shall succeed and return sizeof(uint8_t) + sizeof(uint32_t) + source's size. ]*/
                result = CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + content->size;
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_to_buffer, THANDLE(CONSTBUFFER), source, CONSTBUFFER_THANDLE_to_buffer_alloc, alloc, void*, alloc_context, uint32_t*, serialized_size)
{
    unsigned char* result;
    if (
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_060: [ If source is NULL then CONSTBUFFER_THANDLE_to_buffer shall fail and return NULL. ]*/
        (source == NULL) ||
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_061: [ If serialized_size is NULL then CONSTBUFFER_THANDLE_to_buffer shall fail and return NULL. ]*/
        (serialized_size == NULL)
        )
    {
        LogError("invalid arguments THANDLE(CONSTBUFFER) source=%p, CONSTBUFFER_THANDLE_to_buffer_alloc alloc=%p, uint32_t* serialized_size=%p",
            source, alloc, serialized_size);
        result = NULL;
    }
    else
    {
        const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(source);
        if (content == NULL)
        {
            LogError("CONSTBUFFER_THANDLE_GetContent failed");
            result = NULL;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_062: [ If alloc is NULL then CONSTBUFFER_THANDLE_to_buffer shall use malloc as provided by gballoc_hl_redirect.h. ]*/
            if (alloc == NULL)
            {
                alloc = &calls_malloc;
            }

            /*Codes_SRS_CONSTBUFFER_THANDLE_88_068: [ If there are any failures then CONSTBUFFER_THANDLE_to_buffer shall fail and return NULL. ]*/
            if (UINT32_MAX - CONSTBUFFER_VERSION_SIZE - CONSTBUFFER_SIZE_SIZE < content->size)
            {
                /*overflow*/
                LogError("serialization size exceeds UINT32_MAX=%" PRIu32 ". Serialization size is the sum of sizeof(uint8_t)=%zu + sizeof(uint32_t)=%zu + content->size=%" PRIu32 "",
                    UINT32_MAX, CONSTBUFFER_VERSION_SIZE, CONSTBUFFER_SIZE_SIZE, content->size);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_063: [ CONSTBUFFER_THANDLE_to_buffer shall allocate memory using alloc for holding the complete serialization. ]*/
                result = alloc(CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + content->size, alloc_context);
                if (result == NULL)
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_068: [ If there are any failures then CONSTBUFFER_THANDLE_to_buffer shall fail and return NULL. ]*/
                    LogError("failure in alloc=%p, (sizeof(uint8_t)=%zu + sizeof(uint32_t)=%zu + content->size=%" PRIu32 " alloc_context=%p);",
                        alloc, CONSTBUFFER_VERSION_SIZE, CONSTBUFFER_SIZE_SIZE, content->size, alloc_context);
                    /*return as is*/
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_064: [ CONSTBUFFER_THANDLE_to_buffer shall write at offset 0 of the allocated memory the version of the serialization (currently 1). ]*/
                    write_uint8_t(result + CONSTBUFFER_VERSION_OFFSET, CONSTBUFFER_VERSION_V1);

                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_065: [ CONSTBUFFER_THANDLE_to_buffer shall write at offsets 1-4 of the allocated memory the value of source's size in network byte order. ]*/
                    write_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, content->size);
                    
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_066: [ CONSTBUFFER_THANDLE_to_buffer shall write starting at offset 5 of the allocated memory the bytes of source's buffer. ]*/
                    if (content->buffer != NULL && content->size > 0)
                    {
                        (void)memcpy(result + CONSTBUFFER_CONTENT_OFFSET, content->buffer, content->size);
                    }

                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_067: [ CONSTBUFFER_THANDLE_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory. ]*/
                    *serialized_size = CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + content->size;
                    /*return as is*/
                }
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_to_fixed_size_buffer, THANDLE(CONSTBUFFER), source, unsigned char*, destination, uint32_t, destination_size, uint32_t*, serialized_size)
{
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result;

    /*Codes_SRS_CONSTBUFFER_THANDLE_88_069: [ If source is NULL then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_070: [ If destination is NULL then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_071: [ If serialized_size is NULL then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
    if (
        (source == NULL) ||
        (destination == NULL) ||
        (serialized_size == NULL)
        )
    {
        LogError("invalid argument THANDLE(CONSTBUFFER) source=%p, unsigned char* destination=%p, uint32_t destination_size=%" PRIu32 ", uint32_t* serialized_size=%p",
            source, destination, destination_size, serialized_size);
        result = CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG;
    }
    else
    {
        uint32_t needed_size;
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_077: [ If there are any failures then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_ERROR. ]*/
        needed_size = CONSTBUFFER_THANDLE_get_serialization_size(source);
        if (needed_size == 0)
        {
            LogError("failure in CONSTBUFFER_THANDLE_get_serialization_size(source=%p)", source);
            result = CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_ERROR;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_072: [ If the size of serialization exceeds destination_size then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail, write in serialized_size how much it would need and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER. ]*/
            if (needed_size > destination_size)
            {
                *serialized_size = needed_size;
                result = CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER;
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_073: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall write at offset 0 of destination the version of serialization (currently 1). ]*/
                destination[0] = CONSTBUFFER_VERSION_V1;
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_074: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall write at offset 1 of destination the value of source's size in network byte order. ]*/
                const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(source);
                write_uint32_t(destination + CONSTBUFFER_SIZE_OFFSET, content->size);
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_075: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall copy all the bytes of source's buffer in destination starting at offset 5. ]*/
                (void)memcpy(destination + CONSTBUFFER_CONTENT_OFFSET, content->buffer, content->size);
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_076: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall succeed, write in serialized_size how much it used and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK. ]*/
                *serialized_size = needed_size;
                result = CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK;
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_from_buffer, const unsigned char*, source, uint32_t, size, uint32_t*, consumed, THANDLE(CONSTBUFFER)*, destination)
{
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result;

    /*Codes_SRS_CONSTBUFFER_THANDLE_88_078: [ If source is NULL then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_079: [ If consumed is NULL then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
    /*Codes_SRS_CONSTBUFFER_THANDLE_88_080: [ If destination is NULL then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
    if (
        (source == NULL) ||
        (consumed == NULL) ||
        (destination == NULL)
        )
    {
        LogError("invalid arguments const unsigned char* source=%p, uint32_t size=%" PRIu32 ", uint32_t* consumed=%p, THANDLE(CONSTBUFFER)* destination=%p", 
            source, size, consumed, destination);
        result = CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG;
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_081: [ If size is 0 then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
        if (size == 0)
        {
            LogError("cannot deserialize from size=%" PRIu32 "", size);
            result = CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG;
        }
        else
        {
            uint8_t version;
            read_uint8_t(source + CONSTBUFFER_VERSION_OFFSET, &version);
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_082: [ If source byte at offset 0 is not 1 (current version) then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
            if (version != CONSTBUFFER_VERSION_V1)
            {
                LogError("different version (%" PRIu8 ") detected. This module only knows about version %" PRIu8 "", version, CONSTBUFFER_VERSION_V1);
                result = CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA;
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_THANDLE_88_083: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
                if (size < CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE)
                {
                    LogError("cannot deserialize when the number of serialized bytes cannot be determined. size=%" PRIu32 "", size);
                    result = CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA;
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_084: [ CONSTBUFFER_THANDLE_from_buffer shall read the number of serialized content bytes from offset 1 of source. ]*/
                    uint32_t content_size;
                    read_uint32_t(source + CONSTBUFFER_SIZE_OFFSET, &content_size);
                    /*Codes_SRS_CONSTBUFFER_THANDLE_88_085: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) + number of content bytes then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
                    if (size - (CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE) < content_size)
                    {
                        LogError("in the buffer at source=%p of size=%" PRIu32 " there are not enough bytes remaining after version and size to construct content from. Serialized content size was computed as %" PRIu32 " but there are only %" PRIu32 " bytes available",
                            source, size, content_size, (uint32_t)(size - CONSTBUFFER_VERSION_SIZE - CONSTBUFFER_SIZE_SIZE));
                        result = CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA;
                    }
                    else
                    {
                        /*Codes_SRS_CONSTBUFFER_THANDLE_88_086: [ CONSTBUFFER_THANDLE_from_buffer shall create a THANDLE(CONSTBUFFER) from the bytes at offset 5 of source. ]*/
                        THANDLE(CONSTBUFFER) temp_destination = CONSTBUFFER_THANDLE_Create(source + CONSTBUFFER_CONTENT_OFFSET, content_size);
                        if (temp_destination == NULL)
                        {
                            /*Codes_SRS_CONSTBUFFER_THANDLE_88_088: [ If there are any failures then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR. ]*/
                            LogError("failure in CONSTBUFFER_THANDLE_Create(source=%p + CONSTBUFFER_CONTENT_OFFSET=%zu, content_size=%" PRIu32 ")",
                                source, CONSTBUFFER_CONTENT_OFFSET, content_size);
                            result = CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_CONSTBUFFER_THANDLE_88_087: [ CONSTBUFFER_THANDLE_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed THANDLE(CONSTBUFFER) and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK. ]*/
                            THANDLE_ASSIGN(CONSTBUFFER)(destination, temp_destination);
                            *consumed = CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + content_size;
                            result = CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK;
                        }
                        THANDLE_ASSIGN(CONSTBUFFER)(&temp_destination, NULL);
                    }
                }
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWritableHandle, uint32_t, size)
{
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) result = NULL;

    if (size == 0)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_089: [ If size is 0, then CONSTBUFFER_THANDLE_CreateWritableHandle shall fail and return NULL. ]*/
        LogError("Invalid argument: size=%" PRIu32 " (size cannot be 0)", size);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_090: [ CONSTBUFFER_THANDLE_CreateWritableHandle shall allocate memory for the THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA). ]*/
        THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) temp_result = THANDLE_MALLOC_FLEX(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_dispose, size, sizeof(unsigned char));
        if (temp_result == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_091: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWritableHandle shall fail and return NULL. ]*/
            LogError("failure in THANDLE_MALLOC_FLEX(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_dispose, size=%" PRIu32 ", sizeof(unsigned char)=%zu)",
                size, sizeof(unsigned char));
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_092: [ CONSTBUFFER_THANDLE_CreateWritableHandle shall set the ref count of the newly created THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) to 1. ]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_093: [ CONSTBUFFER_THANDLE_CreateWritableHandle shall succeed and return a non-NULL THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA). ]*/
            CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA* handle_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(temp_result);
            handle_data->buffer_type = CONSTBUFFER_THANDLE_TYPE_COPIED;
            handle_data->size = size;
            THANDLE_ASSIGN(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(&result, temp_result);
        }
        THANDLE_ASSIGN(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(&temp_result, NULL);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_GetWritableBuffer, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
{
    unsigned char* result;
    
    if (constbufferWritableHandle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_094: [ If constbufferWritableHandle is NULL, then CONSTBUFFER_THANDLE_GetWritableBuffer shall fail and return NULL. ]*/
        LogError("Invalid argument: THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) constbufferWritableHandle=%p", constbufferWritableHandle);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_095: [ CONSTBUFFER_THANDLE_GetWritableBuffer shall succeed and return a pointer to the non-CONST buffer of constbufferWritableHandle. ]*/
        CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA* handle_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(constbufferWritableHandle);
        result = handle_data->data;
    }
    
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER), CONSTBUFFER_THANDLE_SealWritableHandle, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
{
    THANDLE(CONSTBUFFER) result = NULL;
    
    if (constbufferWritableHandle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_096: [ If constbufferWritableHandle is NULL then CONSTBUFFER_THANDLE_SealWritableHandle shall fail and return NULL. ]*/
        LogError("Invalid argument: THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) constbufferWritableHandle=%p", constbufferWritableHandle);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_097: [ CONSTBUFFER_THANDLE_SealWritableHandle shall create a new THANDLE(CONSTBUFFER) from the contents of constbufferWritableHandle. ]*/
        CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA* writable_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(constbufferWritableHandle);
        THANDLE(CONSTBUFFER) temp_result = CONSTBUFFER_THANDLE_Create(writable_data->data, writable_data->size);
        if (temp_result == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_098: [ If there are any failures then CONSTBUFFER_THANDLE_SealWritableHandle shall fail and return NULL. ]*/
            LogError("failure in CONSTBUFFER_THANDLE_Create(data=%p, size=%" PRIu32 ")", writable_data->data, writable_data->size);
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_099: [ CONSTBUFFER_THANDLE_SealWritableHandle shall succeed and return a non-NULL THANDLE(CONSTBUFFER). ]*/
            THANDLE_ASSIGN(CONSTBUFFER)(&result, temp_result);
        }
        THANDLE_ASSIGN(CONSTBUFFER)(&temp_result, NULL);
    }
    
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_GetWritableBufferSize, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
{
    uint32_t result;
    
    if (constbufferWritableHandle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_100: [ If constbufferWritableHandle is NULL, then CONSTBUFFER_THANDLE_GetWritableBufferSize shall return 0. ]*/
        LogError("Invalid argument: THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) constbufferWritableHandle=%p", constbufferWritableHandle);
        result = 0;
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_101: [ CONSTBUFFER_THANDLE_GetWritableBufferSize shall succeed and return the size of the writable buffer of constbufferWritableHandle. ]*/
        CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA* handle_data = THANDLE_GET_T(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(constbufferWritableHandle);
        result = handle_data->size;
    }
    
    return result;
}
