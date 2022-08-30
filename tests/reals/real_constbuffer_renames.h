// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_CONSTBUFFER_RENAMES_H
#define REAL_CONSTBUFFER_RENAMES_H

#define CONSTBUFFER_Create real_CONSTBUFFER_Create
#define CONSTBUFFER_CreateFromBuffer real_CONSTBUFFER_CreateFromBuffer
#define CONSTBUFFER_CreateWithMoveMemory real_CONSTBUFFER_CreateWithMoveMemory
#define CONSTBUFFER_CreateWithCustomFree real_CONSTBUFFER_CreateWithCustomFree
#define CONSTBUFFER_CreateFromOffsetAndSizeWithCopy real_CONSTBUFFER_CreateFromOffsetAndSizeWithCopy
#define CONSTBUFFER_IncRef real_CONSTBUFFER_IncRef
#define CONSTBUFFER_GetContent real_CONSTBUFFER_GetContent
#define CONSTBUFFER_DecRef real_CONSTBUFFER_DecRef
#define CONSTBUFFER_HANDLE_contain_same real_CONSTBUFFER_HANDLE_contain_same
#define CONSTBUFFER_CreateFromOffsetAndSize real_CONSTBUFFER_CreateFromOffsetAndSize
#define CONSTBUFFER_get_serialization_size real_CONSTBUFFER_get_serialization_size
#define CONSTBUFFER_to_buffer real_CONSTBUFFER_to_buffer
#define CONSTBUFFER_to_fixed_size_buffer real_CONSTBUFFER_to_fixed_size_buffer
#define CONSTBUFFER_from_buffer real_CONSTBUFFER_from_buffer

#define CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT real_CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT
#define CONSTBUFFER_FROM_BUFFER_RESULT real_CONSTBUFFER_FROM_BUFFER_RESULT

#define CONSTBUFFER_createWritableHandle real_CONSTBUFFER_createWritableHandle
#define CONSTBUFFER_getWritableBuffer real_CONSTBUFFER_getWritableBuffer
#define CONSTBUFFER_sealWritableHandle real_CONSTBUFFER_sealWritableHandle
#define CONSTBUFFER_WritableHandleIncRef real_CONSTBUFFER_WritableHandleIncRef
#define CONSTBUFFER_WritableHandleDecRef real_CONSTBUFFER_WritableHandleDecRef
#define CONSTBUFFER_getWritableBufferSize real_CONSTBUFFER_getWritableBufferSize

#endif // REAL_CONSTBUFFER_RENAMES_H
