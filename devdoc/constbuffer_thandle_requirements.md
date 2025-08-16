# ConstBuffer THANDLE Requirements

## Overview

ConstBuffer THANDLE is a module that implements a read-only buffer of bytes (unsigned char) using the THANDLE reference counting system. 
Once created, the buffer can no longer be changed. The buffer uses THANDLE for thread-safe reference counting.

## References

[THANDLE](../deps/c-pal/common/inc/c_pal/thandle.h)

[constbuffer](constbuffer_requirements.md)

## Exposed API

```c
/*this is what is returned when the content of the buffer needs access*/
typedef struct CONSTBUFFER_THANDLE_TAG
{
    const unsigned char* buffer;
    uint32_t size;
} CONSTBUFFER_THANDLE;

/*forward declaration for the THANDLE type*/
typedef struct CONSTBUFFER_THANDLE_HANDLE_DATA_TAG CONSTBUFFER_THANDLE_HANDLE_DATA;

/*declare the THANDLE type*/
THANDLE_TYPE_DECLARE(CONSTBUFFER_THANDLE_HANDLE_DATA);

/*this is the writable handle*/
typedef struct CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA_TAG CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA;

/*declare the THANDLE type*/
THANDLE_TYPE_DECLARE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA);

typedef void(*CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC)(void* context);

typedef void*(*CONSTBUFFER_THANDLE_to_buffer_alloc)(size_t size, void* context);

#define CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_VALUES \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK, \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_ERROR, \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER, \
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG

MU_DEFINE_ENUM(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)

#define CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK, \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR, \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG, \
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA

MU_DEFINE_ENUM(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES)

MOCKABLE_INTERFACE(constbuffer_thandle,
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_Create, const unsigned char*, source, uint32_t, size),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromBuffer, BUFFER_HANDLE, buffer),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWithMoveMemory, unsigned char*, source, uint32_t, size),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size),
    FUNCTION(, const CONSTBUFFER_THANDLE*, CONSTBUFFER_THANDLE_GetContent, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), constbufferHandle),
    FUNCTION(, bool, CONSTBUFFER_THANDLE_HANDLE_contain_same, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), left, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), right),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_Clone, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), constbufferHandle),
    FUNCTION(, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_to_fixed_size_buffer, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), constbufferHandle, unsigned char*, destination, uint32_t, destination_size, uint32_t*, serialized_size),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWritableHandle, uint32_t, size),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_SealWritableHandle, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle),
    FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_GetWritableBuffer, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle),
    FUNCTION(, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_CreateFromBuffer, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA)*, constbufferHandle, const unsigned char*, source, uint32_t, size),
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_to_buffer, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), constbufferHandle, CONSTBUFFER_THANDLE_to_buffer_alloc, alloc, void*, alloc_context)
)
```

## CONSTBUFFER_THANDLE_Create

```c
THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) CONSTBUFFER_THANDLE_Create(const unsigned char* source, uint32_t size)
```

`CONSTBUFFER_THANDLE_Create` creates a const buffer from a memory area.

**SRS_CONSTBUFFER_THANDLE_88_001: [** If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_Create shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_002: [** Otherwise, CONSTBUFFER_THANDLE_Create shall create a copy of the memory area pointed to by source having size bytes. **]**

**SRS_CONSTBUFFER_THANDLE_88_003: [** If creating the copy fails then CONSTBUFFER_THANDLE_Create shall return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_004: [** Otherwise CONSTBUFFER_THANDLE_Create shall return a non-NULL handle. **]**

**SRS_CONSTBUFFER_THANDLE_88_005: [** The non-NULL handle returned by CONSTBUFFER_THANDLE_Create shall have its ref count set to "1". **]**

## CONSTBUFFER_THANDLE_GetContent

```c
const CONSTBUFFER_THANDLE* CONSTBUFFER_THANDLE_GetContent(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) constbufferHandle)
```

`CONSTBUFFER_THANDLE_GetContent` returns a pointer to a CONSTBUFFER_THANDLE structure that can be used to access the stored bytes and the size of the const buffer.

**SRS_CONSTBUFFER_THANDLE_88_011: [** If constbufferHandle is NULL then CONSTBUFFER_THANDLE_GetContent shall return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_012: [** Otherwise, CONSTBUFFER_THANDLE_GetContent shall return a pointer to a CONSTBUFFER_THANDLE structure. **]**
