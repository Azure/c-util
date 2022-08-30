// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_CONSTBUFFER_H
#define REAL_CONSTBUFFER_H


#include <stdint.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CONSTBUFFER_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        CONSTBUFFER_Create, \
        CONSTBUFFER_CreateFromBuffer, \
        CONSTBUFFER_CreateWithMoveMemory, \
        CONSTBUFFER_CreateWithCustomFree, \
        CONSTBUFFER_CreateFromOffsetAndSizeWithCopy, \
        CONSTBUFFER_IncRef, \
        CONSTBUFFER_GetContent, \
        CONSTBUFFER_DecRef, \
        CONSTBUFFER_HANDLE_contain_same, \
        CONSTBUFFER_CreateFromOffsetAndSize, \
        CONSTBUFFER_get_serialization_size, \
        CONSTBUFFER_to_buffer, \
        CONSTBUFFER_to_fixed_size_buffer, \
        CONSTBUFFER_from_buffer, \
        CONSTBUFFER_CreateWritableHandle, \
        CONSTBUFFER_GetWritableBuffer, \
        CONSTBUFFER_SealWritableHandle, \
        CONSTBUFFER_WritableHandleIncRef, \
        CONSTBUFFER_WritableHandleDecRef, \
        CONSTBUFFER_GetWritableBufferSize \
)


#include <stddef.h>
#include <stdbool.h>


#include "c_util/constbuffer.h"



CONSTBUFFER_HANDLE real_CONSTBUFFER_Create(const unsigned char* source, uint32_t size);

CONSTBUFFER_HANDLE real_CONSTBUFFER_CreateFromBuffer(BUFFER_HANDLE buffer);

CONSTBUFFER_HANDLE real_CONSTBUFFER_CreateWithMoveMemory(unsigned char* source, uint32_t size);

CONSTBUFFER_HANDLE real_CONSTBUFFER_CreateWithCustomFree(const unsigned char* source, uint32_t size, CONSTBUFFER_CUSTOM_FREE_FUNC custom_free_func, void* custom_free_func_context);

CONSTBUFFER_HANDLE real_CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(CONSTBUFFER_HANDLE handle, uint32_t offset, uint32_t size);

void real_CONSTBUFFER_IncRef(CONSTBUFFER_HANDLE constbufferHandle);

const CONSTBUFFER* real_CONSTBUFFER_GetContent(CONSTBUFFER_HANDLE constbufferHandle);

void real_CONSTBUFFER_DecRef(CONSTBUFFER_HANDLE constbufferHandle);

bool real_CONSTBUFFER_HANDLE_contain_same(CONSTBUFFER_HANDLE left, CONSTBUFFER_HANDLE right);

CONSTBUFFER_HANDLE real_CONSTBUFFER_CreateFromOffsetAndSize(CONSTBUFFER_HANDLE handle, uint32_t offset, uint32_t size);

uint32_t real_CONSTBUFFER_get_serialization_size(CONSTBUFFER_HANDLE source);

unsigned char* real_CONSTBUFFER_to_buffer(CONSTBUFFER_HANDLE source, CONSTBUFFER_to_buffer_alloc alloc, void* alloc_context, uint32_t* serialized_size);

CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT real_CONSTBUFFER_to_fixed_size_buffer(CONSTBUFFER_HANDLE source, unsigned char* destination, uint32_t destination_size, uint32_t* serialized_size);

CONSTBUFFER_FROM_BUFFER_RESULT real_CONSTBUFFER_from_buffer(const unsigned char* source, uint32_t size, uint32_t* consumed, CONSTBUFFER_HANDLE* destination);

CONSTBUFFER_WRITABLE_HANDLE real_CONSTBUFFER_CreateWritableHandle(uint32_t size);

unsigned char * real_CONSTBUFFER_GetWritableBuffer(CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle);

CONSTBUFFER_HANDLE real_CONSTBUFFER_SealWritableHandle(CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle);

void real_CONSTBUFFER_WritableHandleIncRef(CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle);

void real_CONSTBUFFER_WritableHandleDecRef(CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle);

uint32_t real_CONSTBUFFER_GetWritableBufferSize(CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle);

#endif //REAL_CONSTBUFFER_H
