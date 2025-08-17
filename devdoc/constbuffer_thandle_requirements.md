# ConstBuffer THANDLE Requirements

## Overview

ConstBuffer THANDLE is a module that implements a read-only buffer of bytes (unsigned char) using the THANDLE reference counting system. Once created, the buffer can no longer be changed. The buffer uses THANDLE for thread-safe reference counting.

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
    FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size),
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

## CONSTBUFFER_THANDLE_CreateFromBuffer

```c
THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) CONSTBUFFER_THANDLE_CreateFromBuffer(BUFFER_HANDLE buffer)
```

`CONSTBUFFER_THANDLE_CreateFromBuffer` creates a const buffer from an existing BUFFER_HANDLE by copying its content.

**SRS_CONSTBUFFER_THANDLE_88_006: [** If buffer is NULL then CONSTBUFFER_THANDLE_CreateFromBuffer shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_007: [** Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall copy the content of buffer. **]**

**SRS_CONSTBUFFER_THANDLE_88_008: [** If copying the content fails, then CONSTBUFFER_THANDLE_CreateFromBuffer shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_009: [** Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall return a non-NULL handle. **]**

**SRS_CONSTBUFFER_THANDLE_88_010: [** The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateFromBuffer shall have its ref count set to "1". **]**

## CONSTBUFFER_THANDLE_GetContent

```c
const CONSTBUFFER_THANDLE* CONSTBUFFER_THANDLE_GetContent(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) constbufferHandle)
```

`CONSTBUFFER_THANDLE_GetContent` returns a pointer to a CONSTBUFFER_THANDLE structure that can be used to access the stored bytes and the size of the const buffer.

**SRS_CONSTBUFFER_THANDLE_88_011: [** If constbufferHandle is NULL then CONSTBUFFER_THANDLE_GetContent shall return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_012: [** Otherwise, CONSTBUFFER_THANDLE_GetContent shall return a pointer to a CONSTBUFFER_THANDLE structure. **]**

## CONSTBUFFER_THANDLE_CreateWithMoveMemory

```c
THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) CONSTBUFFER_THANDLE_CreateWithMoveMemory(unsigned char* source, uint32_t size)
```

`CONSTBUFFER_THANDLE_CreateWithMoveMemory` creates a const buffer by moving the memory ownership from the caller to the const buffer.

**SRS_CONSTBUFFER_THANDLE_88_015: [** If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_016: [** CONSTBUFFER_THANDLE_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer. **]**

**SRS_CONSTBUFFER_THANDLE_88_017: [** If any error occurs, CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_018: [** If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. **]**

**SRS_CONSTBUFFER_THANDLE_88_019: [** The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithMoveMemory shall have its ref count set to "1". **]**

## Disposal

**SRS_CONSTBUFFER_THANDLE_88_013: [** CONSTBUFFER_THANDLE_HANDLE_DATA_dispose shall free the memory used by the const buffer. **]**

**SRS_CONSTBUFFER_THANDLE_88_014: [** If the buffer was created by calling CONSTBUFFER_THANDLE_CreateWithMoveMemory, the memory pointed to by the buffer pointer shall be freed. **]**

## CONSTBUFFER_THANDLE_contain_same

```c
bool CONSTBUFFER_THANDLE_contain_same(THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) left, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) right)
```

`CONSTBUFFER_THANDLE_contain_same` compares two const buffer handles to determine if they contain the same data.

**SRS_CONSTBUFFER_THANDLE_88_020: [** If left is NULL and right is NULL then CONSTBUFFER_THANDLE_contain_same shall return true. **]**

**SRS_CONSTBUFFER_THANDLE_88_021: [** If left is NULL and right is not NULL then CONSTBUFFER_THANDLE_contain_same shall return false. **]**

**SRS_CONSTBUFFER_THANDLE_88_022: [** If left is not NULL and right is NULL then CONSTBUFFER_THANDLE_contain_same shall return false. **]**

**SRS_CONSTBUFFER_THANDLE_88_023: [** If left's size is different than right's size then CONSTBUFFER_THANDLE_contain_same shall return false. **]**

**SRS_CONSTBUFFER_THANDLE_88_024: [** If left's buffer contains different bytes than right's buffer then CONSTBUFFER_THANDLE_contain_same shall return false. **]**

**SRS_CONSTBUFFER_THANDLE_88_025: [** CONSTBUFFER_THANDLE_contain_same shall return true. **]**

## CONSTBUFFER_THANDLE_CreateWithCustomFree

```c
THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) CONSTBUFFER_THANDLE_CreateWithCustomFree(const unsigned char* source, uint32_t size, CONSTBUFFER_THANDLE_CUSTOM_FREE_FUNC customFreeFunc, void* customFreeFuncContext)
```

`CONSTBUFFER_THANDLE_CreateWithCustomFree` creates a const buffer by taking ownership of existing memory and using a custom function to free it.

**SRS_CONSTBUFFER_THANDLE_88_026: [** If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_027: [** If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. **]**

**SRS_CONSTBUFFER_THANDLE_88_028: [** CONSTBUFFER_THANDLE_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer. **]**

**SRS_CONSTBUFFER_THANDLE_88_029: [** If customFreeFunc is NULL, CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_030: [** The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithCustomFree shall have its ref count set to "1". **]**

**SRS_CONSTBUFFER_THANDLE_88_031: [** If the buffer was created by calling CONSTBUFFER_THANDLE_CreateWithCustomFree, the customFreeFunc function shall be called to free the memory, while passed customFreeFuncContext as argument. **]**

**SRS_CONSTBUFFER_THANDLE_88_032: [** customFreeFuncContext shall be allowed to be NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_033: [** If any error occurs, CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_034: [** CONSTBUFFER_THANDLE_CreateWithCustomFree shall store customFreeFunc and customFreeFuncContext in order to use them to free the memory when the const buffer resources are freed. **]**

## CONSTBUFFER_THANDLE_CreateFromOffsetAndSize

```c
MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size)
```

`CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` creates a new `CONSTBUFFER_THANDLE_HANDLE_DATA` from a region of an existing one.

**SRS_CONSTBUFFER_THANDLE_88_035: [** If `handle` is `NULL` then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_037: [** If `offset` is greater than `handle`'s size then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_038: [** If `offset + size` would overflow then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_039: [** If `offset + size` exceed `handle`'s size then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_040: [** If `offset` is 0 and `size` is equal to `handle`'s size then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall increment the reference count of `handle` and return `handle`. **]**

**SRS_CONSTBUFFER_THANDLE_88_041: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall allocate memory for a new `CONSTBUFFER_THANDLE_HANDLE_DATA`. **]**

**SRS_CONSTBUFFER_THANDLE_88_042: [** If there are any failures then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_043: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall set the buffer pointer to point to `handle`'s buffer + `offset`. **]**

**SRS_CONSTBUFFER_THANDLE_88_044: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall set the size to the provided `size`. **]**

**SRS_CONSTBUFFER_THANDLE_88_045: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall increment the reference count of `handle` and store it. **]**

**SRS_CONSTBUFFER_THANDLE_88_046: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall set the ref count of the newly created `CONSTBUFFER_THANDLE_HANDLE_DATA` to 1. **]**

**SRS_CONSTBUFFER_THANDLE_88_047: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize` shall succeed and return a non-NULL value. **]**

**SRS_CONSTBUFFER_THANDLE_88_048: [** If the buffer was created by calling `CONSTBUFFER_THANDLE_CreateFromOffsetAndSize`, the original handle shall be decremented. **]**

## CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy

```c
MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), handle, uint32_t, offset, uint32_t, size);
```

`CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` creates a new `CONSTBUFFER_THANDLE_HANDLE_DATA` by copying data from an existing handle starting at a given offset and with a given size.

**SRS_CONSTBUFFER_THANDLE_88_049: [** If `handle` is `NULL` then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_051: [** If `offset` is greater than `handle`'s size then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_052: [** If `offset + size` would overflow then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_053: [** If `offset + size` exceed `handle`'s size then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_054: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` shall create a new const buffer by copying data from `handle`'s buffer starting at `offset` and with the given `size`. **]**

**SRS_CONSTBUFFER_THANDLE_88_055: [** If there are any failures then `CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_056: [** `CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy` shall succeed and return a non-NULL value. **]**

## CONSTBUFFER_THANDLE_get_serialization_size

```c
MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_get_serialization_size, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), source)
```

`CONSTBUFFER_THANDLE_get_serialization_size` returns the size required to serialize the const buffer.

**SRS_CONSTBUFFER_THANDLE_88_057: [** If `source` is `NULL` then `CONSTBUFFER_THANDLE_get_serialization_size` shall fail and return 0. **]**

**SRS_CONSTBUFFER_THANDLE_88_058: [** If `sizeof(uint8_t) + sizeof(uint32_t) + source's size` exceed `UINT32_MAX` then `CONSTBUFFER_THANDLE_get_serialization_size` shall fail and return 0. **]**

**SRS_CONSTBUFFER_THANDLE_88_059: [** Otherwise `CONSTBUFFER_THANDLE_get_serialization_size` shall succeed and return `sizeof(uint8_t) + sizeof(uint32_t) + source's size`. **]**

## CONSTBUFFER_THANDLE_to_buffer

```c
MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_to_buffer, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), source, CONSTBUFFER_THANDLE_to_buffer_alloc, alloc, void*, alloc_context, uint32_t*, serialized_size)
```

`CONSTBUFFER_THANDLE_to_buffer` serializes the const buffer to a binary representation.

**SRS_CONSTBUFFER_THANDLE_88_060: [** If `source` is `NULL` then `CONSTBUFFER_THANDLE_to_buffer` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_061: [** If `serialized_size` is `NULL` then `CONSTBUFFER_THANDLE_to_buffer` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_062: [** If `alloc` is `NULL` then `CONSTBUFFER_THANDLE_to_buffer` shall use `malloc` as provided by `gballoc_hl_redirect.h`. **]**

**SRS_CONSTBUFFER_THANDLE_88_063: [** `CONSTBUFFER_THANDLE_to_buffer` shall allocate memory using `alloc` for holding the complete serialization. **]**

**SRS_CONSTBUFFER_THANDLE_88_064: [** `CONSTBUFFER_THANDLE_to_buffer` shall write at offset 0 of the allocated memory the version of the serialization (currently 1). **]**

**SRS_CONSTBUFFER_THANDLE_88_065: [** `CONSTBUFFER_THANDLE_to_buffer` shall write at offsets 1-4 of the allocated memory the value of `source`'s size in network byte order. **]**

**SRS_CONSTBUFFER_THANDLE_88_066: [** `CONSTBUFFER_THANDLE_to_buffer` shall write starting at offset 5 of the allocated memory the bytes of `source`'s buffer. **]**

**SRS_CONSTBUFFER_THANDLE_88_067: [** `CONSTBUFFER_THANDLE_to_buffer` shall succeed, write in `serialized_size` the size of the serialization and return the allocated memory. **]**

**SRS_CONSTBUFFER_THANDLE_88_068: [** If there are any failures then `CONSTBUFFER_THANDLE_to_buffer` shall fail and return `NULL`. **]**

## CONSTBUFFER_THANDLE_to_fixed_size_buffer

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_to_fixed_size_buffer, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), source, unsigned char*, destination, uint32_t, destination_size, uint32_t*, serialized_size)
```

`CONSTBUFFER_THANDLE_to_fixed_size_buffer` serializes the const buffer to a fixed-size destination buffer.

**SRS_CONSTBUFFER_THANDLE_88_069: [** If `source` is `NULL` then `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_THANDLE_88_070: [** If `destination` is `NULL` then `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_THANDLE_88_071: [** If `serialized_size` is `NULL` then `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_THANDLE_88_072: [** If the size of serialization exceeds `destination_size` then `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall fail, write in `serialized_size` how much it would need and return `CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER`. **]**

**SRS_CONSTBUFFER_THANDLE_88_073: [** `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall write at offset 0 of `destination` the version of serialization (currently 1). **]**

**SRS_CONSTBUFFER_THANDLE_88_074: [** `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall write at offset 1 of `destination` the value of `source`'s size in network byte order. **]**

**SRS_CONSTBUFFER_THANDLE_88_075: [** `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall copy all the bytes of `source`'s buffer in `destination` starting at offset 5. **]**

**SRS_CONSTBUFFER_THANDLE_88_076: [** `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall succeed, write in `serialized_size` how much it used and return `CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK`. **]**

**SRS_CONSTBUFFER_THANDLE_88_077: [** If there are any failures then `CONSTBUFFER_THANDLE_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_ERROR`. **]**

## CONSTBUFFER_THANDLE_from_buffer

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_from_buffer, const unsigned char*, source, uint32_t, size, uint32_t*, consumed, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA)*, destination)
```

`CONSTBUFFER_THANDLE_from_buffer` deserializes a const buffer from a serialized buffer.

**SRS_CONSTBUFFER_THANDLE_88_078: [** If `source` is `NULL` then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_THANDLE_88_079: [** If `consumed` is `NULL` then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_THANDLE_88_080: [** If `destination` is `NULL` then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_THANDLE_88_081: [** If `size` is 0 then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_THANDLE_88_082: [** If source byte at offset 0 is not 1 (current version) then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA`. **]**

**SRS_CONSTBUFFER_THANDLE_88_083: [** If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA`. **]**

**SRS_CONSTBUFFER_THANDLE_88_084: [** `CONSTBUFFER_THANDLE_from_buffer` shall read the number of serialized content bytes from offset 1 of source. **]**

**SRS_CONSTBUFFER_THANDLE_88_085: [** If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) + number of content bytes then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA`. **]**

**SRS_CONSTBUFFER_THANDLE_88_086: [** `CONSTBUFFER_THANDLE_from_buffer` shall create a THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) from the bytes at offset 5 of source. **]**

**SRS_CONSTBUFFER_THANDLE_88_087: [** `CONSTBUFFER_THANDLE_from_buffer` shall succeed, write in `consumed` the total number of consumed bytes from source, write in `destination` the constructed THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA) and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK`. **]**

**SRS_CONSTBUFFER_THANDLE_88_088: [** If there are any failures then `CONSTBUFFER_THANDLE_from_buffer` shall fail and return `CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR`. **]**

## CONSTBUFFER_THANDLE_CreateWritableHandle

```c
MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), CONSTBUFFER_THANDLE_CreateWritableHandle, uint32_t, size)
```

`CONSTBUFFER_THANDLE_CreateWritableHandle` constructs a new `THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)` of size `size`. Buffers of `THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)` are writeable (the same buffer can be reused to fill).

**SRS_CONSTBUFFER_THANDLE_88_089: [** If `size` is 0, then `CONSTBUFFER_THANDLE_CreateWritableHandle` shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_090: [** `CONSTBUFFER_THANDLE_CreateWritableHandle` shall allocate memory for the `THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)`. **]**

**SRS_CONSTBUFFER_THANDLE_88_091: [** If any error occurs, `CONSTBUFFER_THANDLE_CreateWritableHandle` shall fail and return NULL. **]**

**SRS_CONSTBUFFER_THANDLE_88_092: [** `CONSTBUFFER_THANDLE_CreateWritableHandle` shall set the ref count of the newly created `THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)` to 1. **]**

**SRS_CONSTBUFFER_THANDLE_88_093: [** `CONSTBUFFER_THANDLE_CreateWritableHandle` shall succeed and return a non-`NULL` `THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)`. **]**

## CONSTBUFFER_THANDLE_GetWritableBuffer

```c
MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_THANDLE_GetWritableBuffer, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
```

`CONSTBUFFER_THANDLE_GetWritableBuffer` returns a pointer to the writable buffer of `constbufferWritableHandle`.

**SRS_CONSTBUFFER_THANDLE_88_094: [** If `constbufferWritableHandle` is `NULL`, then `CONSTBUFFER_THANDLE_GetWritableBuffer` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_095: [** `CONSTBUFFER_THANDLE_GetWritableBuffer` shall succeed and return a pointer to the non-CONST buffer of `constbufferWritableHandle`. **]**

## CONSTBUFFER_THANDLE_SealWritableHandle

```c
MOCKABLE_FUNCTION(, THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA), CONSTBUFFER_THANDLE_SealWritableHandle, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
```

`CONSTBUFFER_THANDLE_SealWritableHandle` seals a writable handle, making it immutable.

**SRS_CONSTBUFFER_THANDLE_88_096: [** If `constbufferWritableHandle` is `NULL` then `CONSTBUFFER_THANDLE_SealWritableHandle` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_097: [** `CONSTBUFFER_THANDLE_SealWritableHandle` shall create a new `THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA)` from the contents of `constbufferWritableHandle`. **]**

**SRS_CONSTBUFFER_THANDLE_88_098: [** If there are any failures then `CONSTBUFFER_THANDLE_SealWritableHandle` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_THANDLE_88_099: [** `CONSTBUFFER_THANDLE_SealWritableHandle` shall succeed and return a non-`NULL` `THANDLE(CONSTBUFFER_THANDLE_HANDLE_DATA)`. **]**

## CONSTBUFFER_THANDLE_GetWritableBufferSize

```c
MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_THANDLE_GetWritableBufferSize, THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA), constbufferWritableHandle)
```

`CONSTBUFFER_THANDLE_GetWritableBufferSize` returns the size of the writable buffer.

**SRS_CONSTBUFFER_THANDLE_88_100: [** If `constbufferWritableHandle` is `NULL`, then `CONSTBUFFER_THANDLE_GetWritableBufferSize` shall return 0. **]**

**SRS_CONSTBUFFER_THANDLE_88_101: [** `CONSTBUFFER_THANDLE_GetWritableBufferSize` shall succeed and return the size of the writable buffer of `constbufferWritableHandle`. **]**
