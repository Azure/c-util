# ConstBuffer Requirements

## Overview

ConstBuffer is a module that implements a read-only buffer of bytes (unsigned char). 
Once created, the buffer can no longer be changed. The buffer is ref counted so further API calls result in zero copy.

The content of a `CONSTBUFFER_HANDLE` can be serialized and deserialized to a buffer. The serialization format is as follows:

| Byte offset |   0     | 1-4    | 5...        |
|-------------|---------|--------|-------------|
| Content     | version | size   | content     |

## References

[refcount](../inc/refcount.h)

[buffer](buffer_requirements.md)

## Exposed API

```c
/*this is the handle*/
typedef struct CONSTBUFFER_HANDLE_DATA_TAG* CONSTBUFFER_HANDLE;

/*this is what is returned when the content of the buffer needs access*/
typedef struct CONSTBUFFER_TAG
{
    const unsigned char* buffer;
    uint32_t size;
} CONSTBUFFER;

typedef void(*CONSTBUFFER_CUSTOM_FREE_FUNC)(void* context);

/*what function should CONSTBUFFER_HANDLE_to_buffer use to allocate the returned serialized form. NULL means malloc from gballoc_hl_malloc_redirect.h*/
typedef void*(*CONSTBUFFER_to_buffer_alloc)(uint32_t size);

#define CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_VALUES \
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK, \
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER, \
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG

MU_DEFINE_ENUM(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)

#define CONSTBUFFER_FROM_BUFFER_RESULT_VALUES \
    CONSTBUFFER_FROM_BUFFER_RESULT_OK, \
    CONSTBUFFER_FROM_BUFFER_RESULT_ERROR, \
    CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG, \
    CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA


MOCKABLE_INTERFACE(constbuffer,
    /*this creates a new constbuffer from a memory area*/
    FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, uint32_t, size),

    /*this creates a new constbuffer from an existing BUFFER_HANDLE*/
    FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromBuffer, BUFFER_HANDLE, buffer),

    FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithMoveMemory, unsigned char*, source, uint32_t, size),

    FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext),

    FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSize, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size),

    FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSizeWithCopy, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size),

    FUNCTION(, void, CONSTBUFFER_IncRef, CONSTBUFFER_HANDLE, constbufferHandle),

    FUNCTION(, void, CONSTBUFFER_DecRef, CONSTBUFFER_HANDLE, constbufferHandle),

    FUNCTION(, const CONSTBUFFER*, CONSTBUFFER_GetContent, CONSTBUFFER_HANDLE, constbufferHandle),

    FUNCTION(, bool, CONSTBUFFER_HANDLE_contain_same, CONSTBUFFER_HANDLE, left, CONSTBUFFER_HANDLE, right),

    FUNCTION(, uint32_t, CONSTBUFFER_get_serialization_size, CONSTBUFFER_HANDLE, source),

    FUNCTION(, unsigned char*, CONSTBUFFER_to_buffer, CONSTBUFFER_HANDLE, source, CONSTBUFFER_to_buffer_alloc, alloc, void*, alloc_context, uint32_t*, size),

    FUNCTION(, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_to_fixed_size_buffer, CONSTBUFFER_HANDLE, source, unsigned char*, destination, uint32_t, destination_size, uint32_t*, serialized_size),

    FUNCTION(, CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_from_buffer, const unsigned char*, source, uint32_t, size, uint32_t*, consumed, CONSTBUFFER_HANDLE*, destination),

    FUNCTION(, CONSTBUFFER_WRITABLE_HANDLE, CONSTBUFFER_create_writable_handle, uint32_t, size),

    FUNCTION(, unsigned char*, CONSTBUFFER_get_writable_buffer, CONSTBUFFER_WRITABLE_HANDLE, constbufferWritableHandle),

    FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_seal_writable_handle, CONSTBUFFER_WRITABLE_HANDLE, constbufferWritableHandle)
)
```

### CONSTBUFFER_Create

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_Create, const unsigned char*, source, uint32_t, size);
```

**SRS_CONSTBUFFER_02_001: [** If `source` is NULL and `size` is different than 0 then CONSTBUFFER_Create shall fail and return NULL. **]**

**SRS_CONSTBUFFER_02_002: [** Otherwise, `CONSTBUFFER_Create` shall create a copy of the memory area pointed to by `source` having `size` bytes. **]**

**SRS_CONSTBUFFER_02_003: [** If creating the copy fails then `CONSTBUFFER_Create` shall return NULL. **]**

**SRS_CONSTBUFFER_02_004: [** Otherwise `CONSTBUFFER_Create` shall return a non-NULL handle. **]**

**SRS_CONSTBUFFER_02_005: [** The non-NULL handle returned by `CONSTBUFFER_Create` shall have its ref count set to "1". **]** 

### CONSTBUFFER_CreateFromBuffer

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromBuffer, BUFFER_HANDLE, buffer);
```

**SRS_CONSTBUFFER_02_006: [** If `buffer` is NULL then `CONSTBUFFER_CreateFromBuffer` shall fail and return NULL. **]**

**SRS_CONSTBUFFER_02_007: [** Otherwise, `CONSTBUFFER_CreateFromBuffer` shall copy the content of `buffer`. **]**

**SRS_CONSTBUFFER_02_008: [** If copying the content fails, then `CONSTBUFFER_CreateFromBuffer` shall fail and return NULL. **]**

**SRS_CONSTBUFFER_02_009: [** Otherwise, `CONSTBUFFER_CreateFromBuffer` shall return a non-NULL handle. **]**

**SRS_CONSTBUFFER_02_010: [** The non-NULL handle returned by `CONSTBUFFER_CreateFromBuffer` shall have its ref count set to "1". **]** 

### CONSTBUFFER_CreateWithMoveMemory

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithMoveMemory, unsigned char*, source, uint32_t, size);
```

`CONSTBUFFER_CreateWithMoveMemory` creates a CONST buffer with move semantics for the memory given as argument (if succesfull, the const buffer owns the memory from that point on).
The memory is assumed to be freeable by a call to `free`.

**SRS_CONSTBUFFER_01_001: [** If `source` is NULL and `size` is different than 0 then `CONSTBUFFER_CreateWithMoveMemory` shall fail and return NULL. **]**

**SRS_CONSTBUFFER_01_004: [** If `source` is non-NULL and `size` is 0, the `source` pointer shall be owned (and freed) by the newly created instance of const buffer. **]**

**SRS_CONSTBUFFER_01_002: [** `CONSTBUFFER_CreateWithMoveMemory` shall store the `source` and `size` and return a non-NULL handle to the newly created const buffer. **]**

**SRS_CONSTBUFFER_01_003: [** The non-NULL handle returned by `CONSTBUFFER_CreateWithMoveMemory` shall have its ref count set to "1". **]**

**SRS_CONSTBUFFER_01_005: [** If any error occurs, `CONSTBUFFER_CreateWithMoveMemory` shall fail and return NULL. **]**

### CONSTBUFFER_CreateWithCustomFree

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateWithCustomFree, const unsigned char*, source, uint32_t, size, CONSTBUFFER_CUSTOM_FREE_FUNC, customFreeFunc, void*, customFreeFuncContext);
```

`CONSTBUFFER_CreateWithCustomFree` creates a CONST buffer with move semantics for the memory given as argument (if succesfull, the const buffer owns the memory from that point on).
The memory has to be free by calling the custom free function passed as argument.

**SRS_CONSTBUFFER_01_006: [** If `source` is NULL and `size` is different than 0 then `CONSTBUFFER_CreateWithCustomFree` shall fail and return NULL. **]**

**SRS_CONSTBUFFER_01_013: [** If `customFreeFunc` is NULL, `CONSTBUFFER_CreateWithCustomFree` shall fail and return NULL. **]**

**SRS_CONSTBUFFER_01_014: [** `customFreeFuncContext` shall be allowed to be NULL. **]**

**SRS_CONSTBUFFER_01_007: [** If `source` is non-NULL and `size` is 0, the `source` pointer shall be owned (and freed) by the newly created instance of const buffer. **]**

**SRS_CONSTBUFFER_01_008: [** `CONSTBUFFER_CreateWithCustomFree` shall store the `source` and `size` and return a non-NULL handle to the newly created const buffer. **]**

**SRS_CONSTBUFFER_01_009: [** `CONSTBUFFER_CreateWithCustomFree` shall store `customFreeFunc` and `customFreeFuncContext` in order to use them to free the memory when the CONST buffer resources are freed. **]**

**SRS_CONSTBUFFER_01_010: [** The non-NULL handle returned by `CONSTBUFFER_CreateWithCustomFree` shall have its ref count set to 1. **]**

**SRS_CONSTBUFFER_01_011: [** If any error occurs, `CONSTBUFFER_CreateWithMoveMemory` shall fail and return NULL. **]**

### CONSTBUFFER_CreateFromOffsetAndSize

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSize, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size)
```

Given an existing `handle` `CONSTBUFFER_CreateFromOffsetAndSize` creates another `CONSTBUFFER_HANDLE` from `size` bytes  `handle` starting at `offset`.

**SRS_CONSTBUFFER_02_025: [** If `handle` is `NULL` then `CONSTBUFFER_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_033: [** If `offset` is greater than `handles`'s size then `CONSTBUFFER_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_027: [** If `offset` + `size` exceed `handles`'s size then `CONSTBUFFER_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_028: [** `CONSTBUFFER_CreateFromOffsetAndSize` shall allocate memory for a new `CONSTBUFFER_HANDLE`'s content. **]**

**SRS_CONSTBUFFER_02_029: [** `CONSTBUFFER_CreateFromOffsetAndSize` shall set the ref count of the newly created `CONSTBUFFER_HANDLE` to the initial value. **]**

**SRS_CONSTBUFFER_02_030: [** `CONSTBUFFER_CreateFromOffsetAndSize` shall increment the reference count of `handle`. **]**

**SRS_CONSTBUFFER_02_031: [** `CONSTBUFFER_CreateFromOffsetAndSize` shall succeed and return a non-`NULL` value. **]**

**SRS_CONSTBUFFER_02_032: [** If there are any failures then `CONSTBUFFER_CreateFromOffsetAndSize` shall fail and return `NULL`. **]**

### CONSTBUFFER_CreateFromOffsetAndSizeWithCopy

```c
FUNCTION(, CONSTBUFFER_HANDLE, CONSTBUFFER_CreateFromOffsetAndSizeWithCopy, CONSTBUFFER_HANDLE, handle, uint32_t, offset, uint32_t, size)
```

`CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` creates a new CONSTBUFFER starting with the memory at `offset` in `handle` and having `size` bytes by copying (`memcpy`) those bytes. This creates a new `CONSTBUFFER_HANDLE` with its ref count set to 1.

**SRS_CONSTBUFFER_02_034: [** If `handle` is `NULL` then `CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_035: [** If `offset` exceeds the capacity of `handle` then `CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_036: [** If `offset` + `size` exceed the capacity of `handle` then `CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_037: [** `CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` shall allocate enough memory to hold `CONSTBUFFER_HANDLE` and `size` bytes. **]**

**SRS_CONSTBUFFER_02_038: [** If `size` is 0 then `CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` shall set the pointed to buffer to `NULL`. **]**

**SRS_CONSTBUFFER_02_039: [** `CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` shall set the pointed to a non-`NULL` value that contains the same bytes as `offset`...`offset`+`size`-1 of `handle`. **]**

**SRS_CONSTBUFFER_02_040: [** If there are any failures then `CONSTBUFFER_CreateFromOffsetAndSizeWithCopy` shall fail and return `NULL`. **]**

### CONSTBUFFER_IncRef

```c
MOCKABLE_FUNCTION(, void, CONSTBUFFER_IncRef, CONSTBUFFER_HANDLE, constbufferHandle);
```

**SRS_CONSTBUFFER_02_013: [** If `constbufferHandle` is NULL then `CONSTBUFFER_IncRef` shall return. **]**

**SRS_CONSTBUFFER_02_014: [** Otherwise, `CONSTBUFFER_IncRef` shall increment the reference count. **]**

### CONSTBUFFER_DecRef

```c
MOCKABLE_FUNCTION(, void, CONSTBUFFER_DecRef, CONSTBUFFER_HANDLE, constbufferHandle);
```

**SRS_CONSTBUFFER_02_015: [** If `constbufferHandle` is NULL then `CONSTBUFFER_DecRef` shall do nothing. **]**

**SRS_CONSTBUFFER_02_016: [** Otherwise, `CONSTBUFFER_DecRef` shall decrement the refcount on the `constbufferHandle` handle. **]**

**SRS_CONSTBUFFER_02_017: [** If the refcount reaches zero, then `CONSTBUFFER_DecRef` shall deallocate all resources used by the CONSTBUFFER_HANDLE. **]**

**SRS_CONSTBUFFER_01_012: [** If the buffer was created by calling `CONSTBUFFER_CreateWithCustomFree`, the `customFreeFunc` function shall be called to free the memory, while passed `customFreeFuncContext` as argument. **]**

**SRS_CONSTBUFFER_02_024: [** If the `constbufferHandle` was created by calling `CONSTBUFFER_CreateFromOffsetAndSize` then `CONSTBUFFER_DecRef` shall decrement the ref count of the original `handle` passed to `CONSTBUFFER_CreateFromOffsetAndSize`. **]**

### CONSTBUFFER_GetContent

```c
MOCKABLE_FUNCTION(, const CONSTBUFFER*, CONSTBUFFER_GetContent, CONSTBUFFER_HANDLE, constbufferHandle);
```

**SRS_CONSTBUFFER_02_011: [** If `constbufferHandle` is NULL then CONSTBUFFER_GetContent shall return NULL. **]**

**SRS_CONSTBUFFER_02_012: [** Otherwise, `CONSTBUFFER_GetContent` shall return a `const CONSTBUFFER*` that matches byte by byte the original bytes used to created the const buffer and has the same length. **]**

### CONSTBUFFER_HANDLE_contain_same

```c
MOCKABLE_FUNCTION(, bool, CONSTBUFFER_HANDLE_contain_same, CONSTBUFFER_HANDLE, left, CONSTBUFFER_HANDLE, right);
```

`CONSTBUFFER_HANDLE_contain_same` returns `true` if `left` and `right` have the same content.

**SRS_CONSTBUFFER_02_018: [** If `left` is `NULL` and `right` is `NULL` then `CONSTBUFFER_HANDLE_contain_same` shall return `true`. **]**

**SRS_CONSTBUFFER_02_019: [** If `left` is `NULL` and `right` is not `NULL` then `CONSTBUFFER_HANDLE_contain_same` shall return `false`. **]**

**SRS_CONSTBUFFER_02_020: [** If `left` is not `NULL` and `right` is `NULL` then `CONSTBUFFER_HANDLE_contain_same` shall return `false`. **]**

**SRS_CONSTBUFFER_02_021: [** If `left`'s size is different than `right`'s size then `CONSTBUFFER_HANDLE_contain_same` shall return `false`. **]**

**SRS_CONSTBUFFER_02_022: [** If `left`'s buffer is contains different bytes than `rights`'s buffer then `CONSTBUFFER_HANDLE_contain_same` shall return `false`. **]**

**SRS_CONSTBUFFER_02_023: [** `CONSTBUFFER_HANDLE_contain_same` shall return `true`. **]**

### CONSTBUFFER_get_serialization_size

```c
MOCKABLE_FUNCTION(, uint32_t, CONSTBUFFER_get_serialization_size, CONSTBUFFER_HANDLE, source)
```

`CONSTBUFFER_get_serialization_size` returns the needed size in bytes for serialization of `source`.

**SRS_CONSTBUFFER_02_041: [** If `source` is `NULL` then `CONSTBUFFER_get_serialization_size` shall fail and return 0. **]**

**SRS_CONSTBUFFER_02_042: [** If `sizeof(uint8_t)` + `sizeof(uint32_t)` + `source`'s `size` exceed `UINT32_MAX` then `CONSTBUFFER_get_serialization_size` shall fail and return 0. **]**

**SRS_CONSTBUFFER_02_043: [** Otherwise `CONSTBUFFER_get_serialization_size` shall succeed and return `sizeof(uint8_t)` + `sizeof(uint32_t)` + `source`'s `size`. **]**

### CONSTBUFFER_to_buffer

```c
MOCKABLE_FUNCTION(, unsigned char*, CONSTBUFFER_to_buffer, CONSTBUFFER_HANDLE, source, CONSTBUFFER_to_buffer_alloc, alloc, void*, alloc_context, uint32_t*, size),
```

`CONSTBUFFER_to_buffer` returns the serialized form of `source` using memory allocated with `alloc`. It writes in `size` the number of bytes of the returned buffer.

**SRS_CONSTBUFFER_02_044: [** If `source` is `NULL` then `CONSTBUFFER_to_buffer` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_045: [** If `size` is `NULL` then `CONSTBUFFER_to_buffer` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_02_046: [** If `alloc` is `NULL` then `CONSTBUFFER_to_buffer` shall use `malloc` as provided by `gballoc_hl_redirect.h`. **]**

**SRS_CONSTBUFFER_02_049: [** `CONSTBUFFER_to_buffer` shall allocate memory using `alloc` for holding the complete serialization. **]**

**SRS_CONSTBUFFER_02_050: [** `CONSTBUFFER_to_buffer` shall write at offset 0 of the allocated memory the version of the serialization (currently 1). **]**

**SRS_CONSTBUFFER_02_051: [** `CONSTBUFFER_to_buffer` shall write at offsets 1-4 of the allocated memory the value of `source->alias.size` in network byte order. **]**

**SRS_CONSTBUFFER_02_052: [** `CONSTBUFFER_to_buffer` shall write starting at offset 5 of the allocated memory the bytes of `source->alias.buffer`. **]**

**SRS_CONSTBUFFER_02_053: [** `CONSTBUFFER_to_buffer` shall succeed, write in `size` the size of the serialization and return the allocated memory. **]**

**SRS_CONSTBUFFER_02_054: [** If there are any failures then `CONSTBUFFER_to_buffer` shall fail and return `NULL`. **]**

### CONSTBUFFER_to_fixed_size_buffer

```c
CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT CONSTBUFFER_to_fixed_size_buffer(CONSTBUFFER_HANDLE source, unsigned char* destination, uint32_t destination_size, uint32_t* serialized_size)
```

`CONSTBUFFER_to_fixed_size_buffer` write the serialization of `source` into the buffer `destination` having `destination_size` size and returns in `serialized_size` the number of bytes written.

**SRS_CONSTBUFFER_02_055: [** If `source` is `NULL` then `CONSTBUFFER_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_02_056: [** If `destination` is `NULL` then `CONSTBUFFER_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_02_057: [** If `serialized_size` is `NULL` then `CONSTBUFFER_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_02_058: [** If the size of serialization exceeds `destination_size` then `CONSTBUFFER_to_fixed_size_buffer` shall fail, write in `serialized_size` how much it would need and return `CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER`. **]**

**SRS_CONSTBUFFER_02_059: [** `CONSTBUFFER_to_fixed_size_buffer` shall write at offset 0 of `destination` the version of serialization (currently 1). **]**

**SRS_CONSTBUFFER_02_060: [** `CONSTBUFFER_to_fixed_size_buffer` shall write at offset 1 of `destination` the value of `source->alias.size` in network byte order. **]**

**SRS_CONSTBUFFER_02_061: [** `CONSTBUFFER_to_fixed_size_buffer` shall copy all the bytes of `source->alias.buffer` in `destination` starting at offset 5. **]**

**SRS_CONSTBUFFER_02_062: [** `CONSTBUFFER_to_fixed_size_buffer` shall succeed, write in `serialized_size` how much it used and return `CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK`. **]**

**SRS_CONSTBUFFER_02_074: [** If there are any failures then `CONSTBUFFER_to_fixed_size_buffer` shall fail and return `CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_ERROR`. **]**

### CONSTBUFFER_from_buffer

```c
CONSTBUFFER_FROM_BUFFER_RESULT CONSTBUFFER_from_buffer(const unsigned char* source, uint32_t size, uint32_t* consumed, CONSTBUFFER_HANDLE* destination)
```

`CONSTBUFFER_from_buffer` construct a new `CONSTBUFFER_HANDLE` from the bytes at `source` having size `size`. `CONSTBUFFER_from_buffer` writes in `consumed` the number of bytes consumed from `source` to build the `CONSTBUFFER_HANDLE`.

**SRS_CONSTBUFFER_02_063: [** If `source` is `NULL` then `CONSTBUFFER_from_buffer` shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_02_064: [** If `consumed` is `NULL` then `CONSTBUFFER_from_buffer` shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_02_065: [** If `destination` is `NULL` then `CONSTBUFFER_from_buffer` shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_02_066: [** If size is 0 then `CONSTBUFFER_from_buffer` shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG`. **]**

**SRS_CONSTBUFFER_02_067: [** If `source` byte at offset 0 is not 1 (current version) then `CONSTBUFFER_from_buffer` shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA`. **]**

**SRS_CONSTBUFFER_02_068: [** If `source`'s size is less than sizeof(uint8_t) + sizeof(uint32_t) then `CONSTBUFFER_from_buffer` shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA`. **]**

**SRS_CONSTBUFFER_02_069: [** `CONSTBUFFER_from_buffer` shall read the number of serialized content bytes from offset 1 of `source`. **]**

**SRS_CONSTBUFFER_02_070: [** If `source`'s `size` is less than `sizeof(uint8_t)` + `sizeof(uint32_t)` + number of content bytes then `CONSTBUFFER_from_buffer` shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA`. **]**

**SRS_CONSTBUFFER_02_071: [** `CONSTBUFFER_from_buffer` shall create a `CONSTBUFFER_HANDLE` from the bytes at offset 5 of `source`. **]**

**SRS_CONSTBUFFER_02_072: [** `CONSTBUFFER_from_buffer` shall succeed, write in `consumed` the total number of consumed bytes from `source`, write in `destination` the constructed `CONSTBUFFER_HANDLE` and return `CONSTBUFFER_FROM_BUFFER_RESULT_OK`. **]**

**SRS_CONSTBUFFER_02_073: [** If there are any failures then shall fail and return `CONSTBUFFER_FROM_BUFFER_RESULT_ERROR`. **]**


### CONSTBUFFER_create_writiable_handle

```c
 CONSTBUFFER_WRITABLE_HANDLE CONSTBUFFER_create_writable_handle (uint32_t size)
```

`CONSTBUFFER_create_writiable_handle` construct a new `CONSTBUFFER_WRITABLE_HANDLE` of size `size`. Buffers of `CONSTBUFFER_WRITABLE_HANDLE` can be overridden (the same buffer can be reused to fill).

**S_RS_CONSTBUFFER_51_001: [** If `size` is 0, then CONSTBUFFER_create_writiable_handle shall fail and return NULL. **]**

**S_RS_CONSTBUFFER_51_002: [** `CONSTBUFFER_create_writiable_handle` shall allocate memory for the `CONSTBUFFER_WRITABLE_HANDLE` **]**

**S_RS_CONSTBUFFER_51_003: [** If any error occurs, `CONSTBUFFER_create_writiable_handle` shall fail and return NULL. **]**

**S_RS_CONSTBUFFER_51_004: [** `CONSTBUFFER_create_writiable_handle` shall set the ref count of the newly created `CONSTBUFFER_WRITABLE_HANDLE` to 1. **]**

**S_RS_CONSTBUFFER_51_005: [** `CONSTBUFFER_create_writiable_handle` shall succeed and return a non-`NULL` `CONSTBUFFER_WRITABLE_HANDLE`. **]**


### CONSTBUFFER_get_writable_buffer

```c
 unsigned char* CONSTBUFFER_get_writable_buffer(CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle)
```

`CONSTBUFFER_get_writable_buffer` return the handle of writable buffer.

**S_RS_CONSTBUFFER_51_006: [** If `constbufferHandle` is `NULL`, then `CONSTBUFFER_get_writable_buffer` shall fail and return `NULL`. **]**

**S_RS_CONSTBUFFER_51_007: [** `CONSTBUFFER_get_writable_buffer` shall succeed and return a handle to the non-CONST buffer of `constbufferWritableHandle`. **]**



### CONSTBUFFER_seal_writable_handle

```c
 CONSTBUFFER_HANDLE CONSTBUFFER_seal_writable_handle(CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle)
```

`CONSTBUFFER_seal_writable_handle` shall return `CONSTBUFFER_HANDLE` from the `CONSTBUFFER_WRITABLE_HANDLE`.

**S_RS_CONSTBUFFER_51_008: [** If `constbufferWritableHandle` is `NULL` then `CONSTBUFFER_seal_writable_handle` shall fail and return `NULL`. **]**

**S_RS_CONSTBUFFER_51_009: [** `CONSTBUFFER_seal_writable_handle` shall succeed and return a non-`NULL` `CONSTBUFFER_HANDLE`. **]**