// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"

#include "c_util/memory_data.h"
#include "c_util/constbuffer_format.h"
#include "c_util/constbuffer_version.h"
#include "c_util/constbuffer.h"

#define CONSTBUFFER_TYPE_VALUES \
    CONSTBUFFER_TYPE_COPIED, \
    CONSTBUFFER_TYPE_MEMORY_MOVED, \
    CONSTBUFFER_TYPE_WITH_CUSTOM_FREE, \
    CONSTBUFFER_TYPE_FROM_OFFSET_AND_SIZE

MU_DEFINE_ENUM(CONSTBUFFER_TYPE, CONSTBUFFER_TYPE_VALUES)

typedef struct CONSTBUFFER_HANDLE_DATA_TAG
{
    CONSTBUFFER alias;
    volatile_atomic int32_t count;
    CONSTBUFFER_TYPE buffer_type;
    CONSTBUFFER_CUSTOM_FREE_FUNC custom_free_func;
    void* custom_free_func_context;
    CONSTBUFFER_HANDLE originalHandle; /*where the CONSTBUFFER_TYPE_FROM_OFFSET_AND_SIZE was build from*/
    unsigned char storage[]; /*if the memory was copied, this is where the copied memory is. For example in the case of CONSTBUFFER_CreateFromOffsetAndSizeWithCopy. Can have 0 as size.*/
} CONSTBUFFER_HANDLE_DATA;

static CONSTBUFFER_HANDLE CONSTBUFFER_Create_Internal(const unsigned char* source, uint32_t size)
{
    CONSTBUFFER_HANDLE result;
    /*Codes_SRS_CONSTBUFFER_02_005: [The non-NULL handle returned by CONSTBUFFER_Create shall have its ref count set to "1".]*/
    /*Codes_SRS_CONSTBUFFER_02_010: [The non-NULL handle returned by CONSTBUFFER_CreateFromBuffer shall have its ref count set to "1".]*/
    /*Codes_SRS_CONSTBUFFER_02_037: [ CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall allocate enough memory to hold CONSTBUFFER_HANDLE and size bytes. ]*/
    result = (CONSTBUFFER_HANDLE)malloc(sizeof(CONSTBUFFER_HANDLE_DATA) + size * sizeof(unsigned char));
    if (result == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_02_003: [If creating the copy fails then CONSTBUFFER_Create shall return NULL.]*/
        /*Codes_SRS_CONSTBUFFER_02_008: [If copying the content fails, then CONSTBUFFER_CreateFromBuffer shall fail and return NULL.] */
        /*Codes_SRS_CONSTBUFFER_02_040: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        LogError("failure in malloc(sizeof(CONSTBUFFER_HANDLE_DATA)=%zu + size=%" PRIu32 " * sizeof(unsigned char)=%zu)",
            sizeof(CONSTBUFFER_HANDLE_DATA) , size , sizeof(unsigned char));
        /*return as is*/
    }
    else
    {
        (void)interlocked_exchange(&result->count, 1);

        /*Codes_SRS_CONSTBUFFER_02_002: [Otherwise, CONSTBUFFER_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
        result->alias.size = size;
        /*Codes_SRS_CONSTBUFFER_02_038: [ If size is 0 then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall set the pointed to buffer to NULL. ]*/
        if (size == 0)
        {
            result->alias.buffer = NULL;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_02_004: [Otherwise CONSTBUFFER_Create shall return a non-NULL handle.]*/
            /*Codes_SRS_CONSTBUFFER_02_007: [Otherwise, CONSTBUFFER_CreateFromBuffer shall copy the content of buffer.]*/
            /*Codes_SRS_CONSTBUFFER_02_009: [Otherwise, CONSTBUFFER_CreateFromBuffer shall return a non-NULL handle.]*/
            /*Codes_SRS_CONSTBUFFER_02_039: [ CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall set the pointed to a non-NULL value that contains the same bytes as offset...offset+size-1 of handle. ]*/
            (void)memcpy(result->storage, source, size);
            result->alias.buffer = result->storage;
        }

        result->buffer_type = CONSTBUFFER_TYPE_COPIED;
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, uint32_t, size)
{
    CONSTBUFFER_HANDLE result;
    /*Codes_SRS_CONSTBUFFER_02_001: [If source is NULL and size is different than 0 then CONSTBUFFER_Create shall fail and return NULL.]*/
    if (
        (source == NULL) &&
        (size != 0)
        )
    {
        LogError("invalid arguments passes to CONSTBUFFER_Create: %p, size: %" PRIu32 "", source, size);
        result = NULL;
    }
    else
    {
        result = CONSTBUFFER_Create_Internal(source, size);
    }
    return result;
}

/*this creates a new constbuffer from an existing BUFFER_HANDLE*/
IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromBuffer, BUFFER_HANDLE, buffer)
{
    CONSTBUFFER_HANDLE result;
    /*Codes_SRS_CONSTBUFFER_02_006: [If buffer is NULL then CONSTBUFFER_CreateFromBuffer shall fail and return NULL.]*/
    if (buffer == NULL)
    {
        LogError("invalid arg passed to CONSTBUFFER_CreateFromBuffer buffer: NULL");
        result = NULL;
    }
    else
    {
        uint32_t length = (uint32_t)BUFFER_length(buffer);
        unsigned char* rawBuffer = BUFFER_u_char(buffer);
        result = CONSTBUFFER_Create_Internal(rawBuffer, length);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithMoveMemory, unsigned char*, source, uint32_t, size)
{
    CONSTBUFFER_HANDLE result;

    /* Codes_SRS_CONSTBUFFER_01_001: [ If source is NULL and size is different than 0 then CONSTBUFFER_Create shall fail and return NULL. ]*/
    if ((source == NULL) && (size > 0))
    {
        LogError("Invalid arguments: unsigned char* source=%p, uint32_t size=%" PRIu32 "", source, size);
        result = NULL;
    }
    else
    {
        result = (CONSTBUFFER_HANDLE)malloc(sizeof(CONSTBUFFER_HANDLE_DATA));
        if (result == NULL)
        {
            /* Codes_SRS_CONSTBUFFER_01_005: [ If any error occurs, CONSTBUFFER_CreateWithMoveMemory shall fail and return NULL. ]*/
            LogError("Allocation of CONSTBUFFER_HANDLE_DATA object failed");
        }
        else
        {
            /* Codes_SRS_CONSTBUFFER_01_004: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. ]*/
            /* Codes_SRS_CONSTBUFFER_01_002: [ CONSTBUFFER_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
            result->alias.buffer = source;
            result->alias.size = size;
            result->buffer_type = CONSTBUFFER_TYPE_MEMORY_MOVED;

            /* Codes_SRS_CONSTBUFFER_01_003: [ The non-NULL handle returned by CONSTBUFFER_CreateWithMoveMemory shall have its ref count set to "1". ]*/
            (void)interlocked_exchange(&result->count, 1);
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext)
{
    CONSTBUFFER_HANDLE result;

    /* Codes_SRS_CONSTBUFFER_01_014: [ customFreeFuncContext shall be allowed to be NULL. ]*/

    if (
        /* Codes_SRS_CONSTBUFFER_01_006: [ If source is NULL and size is different than 0 then CONSTBUFFER_CreateWithCustomFree shall fail and return NULL. ]*/
        ((source == NULL) && (size > 0)) ||
        /* Codes_SRS_CONSTBUFFER_01_013: [ If customFreeFunc is NULL, CONSTBUFFER_CreateWithCustomFree shall fail and return NULL. ]*/
        (customFreeFunc == NULL)
        )
    {
        LogError("Invalid arguments: unsigned char* source=%p, uint32_t size=%" PRIu32 ", customFreeFunc=%p, customFreeFuncContext=%p",
            source, size, customFreeFunc, customFreeFuncContext);
        result = NULL;
    }
    else
    {
        result = (CONSTBUFFER_HANDLE)malloc(sizeof(CONSTBUFFER_HANDLE_DATA));
        if (result == NULL)
        {
            /* Codes_SRS_CONSTBUFFER_01_011: [ If any error occurs, CONSTBUFFER_CreateWithMoveMemory shall fail and return NULL. ]*/
            LogError("Allocation of CONSTBUFFER_HANDLE_DATA object failed");
        }
        else
        {
            /* Codes_SRS_CONSTBUFFER_01_007: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. ]*/
            /* Codes_SRS_CONSTBUFFER_01_008: [ CONSTBUFFER_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
            result->alias.buffer = source;
            result->alias.size = size;
            result->buffer_type = CONSTBUFFER_TYPE_WITH_CUSTOM_FREE;

            /* Codes_SRS_CONSTBUFFER_01_009: [ CONSTBUFFER_CreateWithCustomFree shall store customFreeFunc and customFreeFuncContext in order to use them to free the memory when the CONST buffer resources are freed. ]*/
            result->custom_free_func = customFreeFunc;
            result->custom_free_func_context = customFreeFuncContext;

            /* Codes_SRS_CONSTBUFFER_01_010: [ The non-NULL handle returned by CONSTBUFFER_CreateWithCustomFree shall have its ref count set to 1. ]*/
            (void)interlocked_exchange(&result->count, 1);
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSize, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size)
{
    CONSTBUFFER_HANDLE result;

    if (
        /*Codes_SRS_CONSTBUFFER_02_025: [ If handle is NULL then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
        (handle == NULL) ||
        /* Codes_SRS_CONSTBUFFER_02_033: [ If offset is greater than handles's size then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
        (offset > handle->alias.size) ||
        /*Codes_SRS_CONSTBUFFER_02_032: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
        (offset > SIZE_MAX - size) ||
        /*Codes_SRS_CONSTBUFFER_02_027: [ If offset + size exceed handles's size then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
        (offset + size > handle->alias.size)
        )
    {
        LogError("invalid arguments CONSTBUFFER_HANDLE handle=%p, uint32_t offset=%" PRIu32 ", uint32_t size=%" PRIu32 "",
            handle, offset, size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_02_028: [ CONSTBUFFER_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER_HANDLE's content. ]*/
        result = (CONSTBUFFER_HANDLE)malloc(sizeof(CONSTBUFFER_HANDLE_DATA));
        if (result == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_02_032: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
            LogError("failure in malloc(sizeof(CONSTBUFFER_HANDLE_DATA)=%zu)", sizeof(CONSTBUFFER_HANDLE_DATA));
            /*return as is*/
        }
        else
        {
            result->buffer_type = CONSTBUFFER_TYPE_FROM_OFFSET_AND_SIZE;
            result->alias.buffer = handle->alias.buffer+offset;
            result->alias.size = size;

            /*Codes_SRS_CONSTBUFFER_02_030: [ CONSTBUFFER_CreateFromOffsetAndSize shall increment the reference count of handle. ]*/
            (void)interlocked_increment(&handle->count);
            result->originalHandle = handle;

            /*Codes_SRS_CONSTBUFFER_02_029: [ CONSTBUFFER_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER_HANDLE to the initial value. ]*/
            (void)interlocked_exchange(&result->count, 1);

            /*Codes_SRS_CONSTBUFFER_02_031: [ CONSTBUFFER_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSizeWithCopy, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size)
{
    CONSTBUFFER_HANDLE result;

    if (
        /*Codes_SRS_CONSTBUFFER_02_034: [ If handle is NULL then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        (handle == NULL) ||
        /*Codes_SRS_CONSTBUFFER_02_035: [ If offset exceeds the capacity of handle then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        (offset > handle->alias.size) ||
        /*Codes_SRS_CONSTBUFFER_02_040: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        (offset > SIZE_MAX - size) ||
        /*Codes_SRS_CONSTBUFFER_02_036: [ If offset + size exceed the capacity of handle then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
        (offset + size > handle->alias.size)
        )
    {
        LogError("Invalid arguments CONSTBUFFER_HANDLE handle=%p, uint32_t offset=%" PRIu32 ", uint32_t size=%" PRIu32 "",
            handle, offset, size);
        result = NULL;
    }
    else
    {
        result = CONSTBUFFER_Create_Internal(handle->alias.buffer + offset, size);
        if (result == NULL)
        {
            LogError("failure in CONSTBUFFER_Create_Internal(handle->alias.buffer=%p + offset=%" PRIu32 ", size=%" PRIu32 ")",
                handle->alias.buffer, offset, size);
            /*return as is*/
        }
        else
        {
            /*return as is*/
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, CONSTBUFFER_IncRef, CONSTBUFFER_HANDLE, constbufferHandle)
{
    if (constbufferHandle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_02_013: [If constbufferHandle is NULL then CONSTBUFFER_IncRef shall return.]*/
        LogError("Invalid arguments: CONSTBUFFER_HANDLE constbufferHandle: %p", constbufferHandle);
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_02_014: [Otherwise, CONSTBUFFER_IncRef shall increment the reference count.]*/
        (void)interlocked_increment(&constbufferHandle->count);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, const CONSTBUFFER*, CONSTBUFFER_GetContent, CONSTBUFFER_HANDLE, constbufferHandle)
{
    const CONSTBUFFER* result;
    if (constbufferHandle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_02_011: [If constbufferHandle is NULL then CONSTBUFFER_GetContent shall return NULL.]*/
        result = NULL;
        LogError("Invalid argument constbufferHandle: NULL");
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_02_012: [Otherwise, CONSTBUFFER_GetContent shall return a const CONSTBUFFER* that matches byte by byte the original bytes used to created the const buffer and has the same length.]*/
        result = &(constbufferHandle->alias);
    }
    return result;
}

static void CONSTBUFFER_DecRef_internal(CONSTBUFFER_HANDLE constbufferHandle)
{
    /*Codes_SRS_CONSTBUFFER_02_016: [Otherwise, CONSTBUFFER_DecRef shall decrement the refcount on the constbufferHandle handle.]*/
    if (interlocked_decrement(&constbufferHandle->count) == 0)
    {
        if (constbufferHandle->buffer_type == CONSTBUFFER_TYPE_MEMORY_MOVED)
        {
            free((void*)constbufferHandle->alias.buffer);
        }
        else if (constbufferHandle->buffer_type == CONSTBUFFER_TYPE_WITH_CUSTOM_FREE)
        {
            /* Codes_SRS_CONSTBUFFER_01_012: [ If the buffer was created by calling CONSTBUFFER_CreateWithCustomFree, the customFreeFunc function shall be called to free the memory, while passed customFreeFuncContext as argument. ]*/
            constbufferHandle->custom_free_func(constbufferHandle->custom_free_func_context);
        }
        /*Codes_SRS_CONSTBUFFER_02_024: [ If the constbufferHandle was created by calling CONSTBUFFER_CreateFromOffsetAndSize then CONSTBUFFER_DecRef shall decrement the ref count of the original handle passed to CONSTBUFFER_CreateFromOffsetAndSize. ]*/
        else if (constbufferHandle->buffer_type == CONSTBUFFER_TYPE_FROM_OFFSET_AND_SIZE)
        {
            CONSTBUFFER_DecRef_internal(constbufferHandle->originalHandle);
        }

        /*Codes_SRS_CONSTBUFFER_02_017: [If the refcount reaches zero, then CONSTBUFFER_DecRef shall deallocate all resources used by the CONSTBUFFER_HANDLE.]*/
        free(constbufferHandle);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, CONSTBUFFER_DecRef, CONSTBUFFER_HANDLE, constbufferHandle)
{
    if (constbufferHandle == NULL)
    {
        /*Codes_SRS_CONSTBUFFER_02_015: [If constbufferHandle is NULL then CONSTBUFFER_DecRef shall do nothing.]*/
        LogError("Invalid arguments: CONSTBUFFER_HANDLE constbufferHandle=%p", constbufferHandle);
    }
    else
    {
        CONSTBUFFER_DecRef_internal(constbufferHandle);
    }
}


IMPLEMENT_MOCKABLE_FUNCTION(, bool, CONSTBUFFER_HANDLE_contain_same, CONSTBUFFER_HANDLE, left, CONSTBUFFER_HANDLE, right)
{
    bool result;
    if (left == NULL)
    {
        if (right == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_02_018: [ If left is NULL and right is NULL then CONSTBUFFER_HANDLE_contain_same shall return true. ]*/
            result = true;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_02_019: [ If left is NULL and right is not NULL then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
            result = false;
        }
    }
    else
    {
        if (right == NULL)
        {
            /*Codes_SRS_CONSTBUFFER_02_020: [ If left is not NULL and right is NULL then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
            result = false;
        }
        else
        {
            if (left->alias.size != right->alias.size)
            {
                /*Codes_SRS_CONSTBUFFER_02_021: [ If left's size is different than right's size then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
                result = false;
            }
            else
            {
                if (memcmp(left->alias.buffer, right->alias.buffer, left->alias.size) != 0)
                {
                    /*Codes_SRS_CONSTBUFFER_02_022: [ If left's buffer is contains different bytes than rights's buffer then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
                    result = false;
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_02_023: [ CONSTBUFFER_HANDLE_contain_same shall return true. ]*/
                    result = true;
                }
            }
        }
    }
    return result;
}

uint32_t CONSTBUFFER_get_serialization_size(CONSTBUFFER_HANDLE source)
{
    uint32_t result;
    /*Codes_SRS_CONSTBUFFER_02_041: [ If source is NULL then CONSTBUFFER_get_serialization_size shall fail and return 0. ]*/
    if (source == NULL)
    {
        LogError("invalid argument CONSTBUFFER_HANDLE source=%p", source);
        result = 0;
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_02_042: [ If sizeof(uint8_t) + sizeof(uint32_t) + source's size exceed UINT32_MAX then CONSTBUFFER_get_serialization_size shall fail and return 0. ]*/
        if (source->alias.size > UINT32_MAX - CONSTBUFFER_VERSION_SIZE - CONSTBUFFER_SIZE_SIZE)
        {
            LogError("serialization size exceeds UINT32_MAX=%" PRIu32 ". It is the sum of sizeof(uint8_t)=%zu + sizeof(uint32_t)=%zu + source->alias.size=%" PRIu32 "",
                UINT32_MAX, CONSTBUFFER_VERSION_SIZE, CONSTBUFFER_SIZE_SIZE, source->alias.size);
            result = 0;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_02_043: [ Otherwise CONSTBUFFER_get_serialization_size shall succeed and return sizeof(uint8_t) + sizeof(uint32_t) + source's size. ]*/
            result = CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + (uint32_t)source->alias.size;
        }
    }
    return result;
}

#define THE_LESSER UINT32_MAX
#define PRI_THE_LESSER PRIu32
typedef uint32_t THE_LESSER_T;

static void* calls_malloc(uint32_t size, void* context)
{
    (void)context;
    return malloc(size);
}

unsigned char* CONSTBUFFER_to_buffer(CONSTBUFFER_HANDLE source, CONSTBUFFER_to_buffer_alloc alloc, void* alloc_context, uint32_t* serialized_size)
{
    unsigned char* result;
    if (
        /*Codes_SRS_CONSTBUFFER_02_044: [ If source is NULL then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
        (source == NULL) ||
        /*Codes_SRS_CONSTBUFFER_02_045: [ If serialized_size is NULL then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
        (serialized_size == NULL)
        )
    {
        LogError("invalid arguments CONSTBUFFER_HANDLE source=%p, CONSTBUFFER_to_buffer_alloc alloc=%p, uint32_t* serialized_size=%p",
            source, alloc, serialized_size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_02_046: [ If alloc is NULL then CONSTBUFFER_to_buffer shall use malloc as provided by gballoc_hl_redirect.h. ]*/
        if (alloc == NULL)
        {
            alloc = &calls_malloc;
        }

        /*Codes_SRS_CONSTBUFFER_02_054: [ If there are any failures then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
        if (THE_LESSER - CONSTBUFFER_VERSION_SIZE - CONSTBUFFER_SIZE_SIZE < source->alias.size)
        {
            /*overflow*/
            LogError("serialization size exceeds the lesser of (UINT32_MAX, SIZE_MAX)=%" PRI_THE_LESSER ". Serialization size is the sum of sizeof(uint8_t)=%zu + sizeof(uint32_t)=%zu + source->alias.size=%" PRIu32 "",
                THE_LESSER, CONSTBUFFER_VERSION_SIZE, CONSTBUFFER_SIZE_SIZE, source->alias.size);
            result = NULL;
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_02_049: [ CONSTBUFFER_to_buffer shall allocate memory using alloc for holding the complete serialization. ]*/
            result = alloc(CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + source->alias.size, alloc_context);
            if (result == NULL)
            {
                /*Codes_SRS_CONSTBUFFER_02_054: [ If there are any failures then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
                LogError("failure in alloc=%p, (sizeof(uint8_t)=%zu + sizeof(uint32_t)=%zu + source->alias.size=%" PRIu32 " alloc_context=%p);",
                    alloc, CONSTBUFFER_VERSION_SIZE, CONSTBUFFER_SIZE_SIZE, source->alias.size, alloc_context);
                /*return as is*/
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_02_050: [ CONSTBUFFER_to_buffer shall write at offset 0 of the allocated memory the version of the serialization (currently 1). ]*/
                write_uint8_t(result + CONSTBUFFER_VERSION_OFFSET, CONSTBUFFER_VERSION_V1);

                /*Codes_SRS_CONSTBUFFER_02_051: [ CONSTBUFFER_to_buffer shall write at offsets 1-4 of the allocated memory the value of source->alias.size in network byte order. ]*/
                write_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, (uint32_t)source->alias.size);
                
                /*Codes_SRS_CONSTBUFFER_02_052: [ CONSTBUFFER_to_buffer shall write starting at offset 5 of the allocated memory the bytes of source->alias.buffer. ]*/
                (void)memcpy(result + CONSTBUFFER_CONTENT_OFFSET, source->alias.buffer, source->alias.size);

                /*Codes_SRS_CONSTBUFFER_02_053: [ CONSTBUFFER_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory. ]*/
                *serialized_size = CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + (THE_LESSER_T)source->alias.size;
                /*return as is*/
            }
        }
    }
    return result;
}

CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT CONSTBUFFER_to_fixed_size_buffer(CONSTBUFFER_HANDLE source, unsigned char* destination, uint32_t destination_size, uint32_t* serialized_size)
{
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;
    
    if (
        /*Codes_SRS_CONSTBUFFER_02_055: [ If source is NULL then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
        (source == NULL) ||
        /*Codes_SRS_CONSTBUFFER_02_056: [ If destination is NULL then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
        (destination == NULL) ||
        /*Codes_SRS_CONSTBUFFER_02_057: [ If serialized_size is NULL then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
        (serialized_size == NULL)
        )
    {
        LogError("invalid arguments CONSTBUFFER_HANDLE source=%p, unsigned char* destination=%p, uint32_t destination_size=%" PRIu32 ", uint32_t* serialized_size=%p",
            source, destination, destination_size, serialized_size);
        result = CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG;
    }
    else
    {
        if (UINT32_MAX - (CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE) < source->alias.size)
        {
            /*when this is true, an output for "serialized_size" cannot be provided as the complete serialization would exceed UINT32_MAX*/
            LogError("overflow in computation of output parameter serialized_size");

            /*Codes_SRS_CONSTBUFFER_02_074: [ If there are any failures then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_ERROR. ]*/
            result = CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_ERROR;
        }
        else
        {
            *serialized_size = CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + (uint32_t)source->alias.size;
            /*Codes_SRS_CONSTBUFFER_02_058: [ If the size of serialization exceeds destination_size then CONSTBUFFER_to_fixed_size_buffer shall fail, write in serialized_size how much it would need and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER. ]*/
            if (destination_size < *serialized_size)
            {
                LogError("destination=%p does not contain enough bytes for the complete serialization. It only has %" PRIu32 " bytes and there are needed %" PRIu32 " bytes", destination, destination_size, *serialized_size);
                result = CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER;
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_02_059: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 0 of destination the version of serialization (currently 1). ]*/
                write_uint8_t(destination + CONSTBUFFER_VERSION_OFFSET, CONSTBUFFER_VERSION_V1);

                /*Codes_SRS_CONSTBUFFER_02_060: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 1 of destination the value of source->alias.size in network byte order. ]*/
                write_uint32_t(destination + CONSTBUFFER_SIZE_OFFSET, (uint32_t)source->alias.size);

                /*Codes_SRS_CONSTBUFFER_02_061: [ CONSTBUFFER_to_fixed_size_buffer shall copy all the bytes of source->alias.buffer in destination starting at offset 5. ]*/
                (void)memcpy(destination + CONSTBUFFER_CONTENT_OFFSET, source->alias.buffer, source->alias.size);

                /*Codes_SRS_CONSTBUFFER_02_062: [ CONSTBUFFER_to_fixed_size_buffer shall succeed, write in serialized_size how much it used and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK. ]*/
                result = CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK;
            }
        }
    }
    return result;
}

CONSTBUFFER_FROM_BUFFER_RESULT CONSTBUFFER_from_buffer(const unsigned char* source, uint32_t size, uint32_t* consumed, CONSTBUFFER_HANDLE* destination)
{
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    if (
        /*Codes_SRS_CONSTBUFFER_02_063: [ If source is NULL then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
        (source == NULL) ||
        /*Codes_SRS_CONSTBUFFER_02_064: [ If consumed is NULL then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
        (consumed == NULL) ||
        /*Codes_SRS_CONSTBUFFER_02_065: [ If destination is NULL then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
        (destination == NULL)
        )
    {
        LogError("invalid arguments const unsigned char* source=%p, uint32_t size=%" PRIu32 ", uint32_t* consumed=%p, CONSTBUFFER_HANDLE* destination=%p", 
            source, size, consumed, destination);
        result = CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG;
    }
    else
    {
        if (size == 0)
        {
            /*Codes_SRS_CONSTBUFFER_02_066: [ If size is 0 then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
            LogError("cannot deserialize from size=%" PRIu32 "", size);
            result = CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG;
        }
        else
        {
            uint8_t version;
            read_uint8_t(source + CONSTBUFFER_VERSION_OFFSET, &version);
            if (version != CONSTBUFFER_VERSION_V1)
            {
                /*Codes_SRS_CONSTBUFFER_02_067: [ If source byte at offset 0 is not 1 (current version) then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
                LogError("different version (%" PRIu8 ") detected. This module only knows about version %" PRIu8 "", version, CONSTBUFFER_VERSION_V1);
                result = CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA;
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_02_068: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
                if (size < CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE)
                {
                    LogError("cannot deserialize when the numbe of serialized bytes cannot be determined. size=%" PRIu32 "", size);
                    result = CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA;
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_02_069: [ CONSTBUFFER_from_buffer shall read the number of serialized content bytes from offset 1 of source. ]*/
                    uint32_t content_size;
                    read_uint32_t(source + CONSTBUFFER_SIZE_OFFSET, &content_size);
                    /*Codes_SRS_CONSTBUFFER_02_070: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) + number of content bytes then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
                    if (size - (CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE) < content_size)
                    {
                        LogError("in the buffer at source=%p of size=%" PRIu32 " there are not enough bytes remaining after version and size to construct content from. Serialized content size was computed as %" PRIu32 " but there are only %" PRIu32 " bytes available",
                            source, size, content_size, (uint32_t)(size - CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE));
                        result = CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA;
                    }
                    else
                    {
                        /*Codes_SRS_CONSTBUFFER_02_071: [ CONSTBUFFER_from_buffer shall create a CONSTBUFFER_HANDLE from the bytes at offset 5 of source. ]*/
                        *destination = CONSTBUFFER_Create_Internal(source + CONSTBUFFER_CONTENT_OFFSET, content_size);
                        if (*destination == NULL)
                        {
                            /*Codes_SRS_CONSTBUFFER_02_073: [ If there are any failures then shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_ERROR. ]*/
                            LogError("failure in CONSTBUFFER_Create(source=%p + CONSTBUFFER_CONTENT_OFFSET=%zu, content_size=%" PRIu32 ")",
                                source, CONSTBUFFER_CONTENT_OFFSET, content_size);
                            result = CONSTBUFFER_FROM_BUFFER_RESULT_ERROR;
                        }
                        else
                        {
                            /*Codes_SRS_CONSTBUFFER_02_072: [ CONSTBUFFER_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed CONSTBUFFER_HANDLE and return CONSTBUFFER_FROM_BUFFER_RESULT_OK. ]*/
                            *consumed = CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + content_size;
                            result = CONSTBUFFER_FROM_BUFFER_RESULT_OK;
                        }
                    }
                }
            }
        }
    }
    return result;
}
