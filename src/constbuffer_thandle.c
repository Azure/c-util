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
    CONSTBUFFER_THANDLE_TYPE buffer_type;
    uint32_t size;
    unsigned char data[]; // Flexible array member for data storage
} CONSTBUFFER_THANDLE_HANDLE_DATA;

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
    /*Codes_SRS_CONSTBUFFER_THANDLE_01_001: [ CONSTBUFFER_THANDLE_HANDLE_DATA_dispose shall free the memory used by the const buffer. ]*/
    if (handle_data->buffer_type == CONSTBUFFER_THANDLE_TYPE_MEMORY_MOVED)
    {
        // For memory moved, we would need to free the buffer
        // For now, log an error as this is not implemented
        LogError("Memory moved disposal not implemented yet");
    }
    else if (handle_data->buffer_type == CONSTBUFFER_THANDLE_TYPE_WITH_CUSTOM_FREE)
    {
        // Custom free logic would go here but for now we don't support it
        LogError("Custom free not implemented yet");
    }
    else if (handle_data->buffer_type == CONSTBUFFER_THANDLE_TYPE_FROM_OFFSET_AND_SIZE)
    {
        // Offset and size logic would go here but for now we don't support it
        LogError("From offset and size not implemented yet");
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
        handle_data->size = size;
        
        /*Codes_SRS_CONSTBUFFER_THANDLE_88_038: [ If size is 0 then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall set the pointed to buffer to NULL. ]*/
        if (size > 0 && source != NULL)
        {
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_004: [Otherwise CONSTBUFFER_THANDLE_Create shall return a non-NULL handle.]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_007: [Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall copy the content of buffer.]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_009: [Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall return a non-NULL handle.]*/
            /*Codes_SRS_CONSTBUFFER_THANDLE_88_039: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall set the pointed to a non-NULL value that contains the same bytes as offset...offset+size-1 of handle. ]*/
            (void)memcpy(handle_data->data, source, size);
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
        /* Dummy implementation - just return NULL for now */
        LogError("CONSTBUFFER_THANDLE_CreateFromBuffer not implemented yet");
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWithMoveMemory, unsigned char*, source, uint32_t, size)
{
    /* Dummy implementation - just return NULL for now */
    (void)source;
    (void)size;
    LogError("CONSTBUFFER_THANDLE_CreateWithMoveMemory not implemented yet");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext)
{
    /* Dummy implementation - just return NULL for now */
    (void)source;
    (void)size;
    (void)customFreeFunc;
    (void)customFreeFuncContext;
    LogError("CONSTBUFFER_THANDLE_CreateWithCustomFree not implemented yet");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size)
{
    /* Dummy implementation - just return NULL for now */
    (void)handle;
    (void)offset;
    (void)size;
    LogError("CONSTBUFFER_THANDLE_CreateFromOffsetAndSize not implemented yet");
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size)
{
    /* Dummy implementation - just return NULL for now */
    (void)handle;
    (void)offset;
    (void)size;
    LogError("CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy not implemented yet");
    return NULL;
}

const CONSTBUFFER_THANDLE* CONSTBUFFER_THANDLE_GetContent(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) constbufferHandle)
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
        // For now, return NULL as this needs proper implementation
        LogError("CONSTBUFFER_THANDLE_GetContent not implemented yet");
        result = NULL;
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, bool, CONSTBUFFER_THANDLE_contain_same, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), left, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), right)
{
    /* Dummy implementation - just return false for now */
    (void)left;
    (void)right;
    LogError("CONSTBUFFER_THANDLE_contain_same not implemented yet");
    return false;
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
