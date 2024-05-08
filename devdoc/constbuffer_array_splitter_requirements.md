`constbuffer_array_splitter` requirements
================

## Overview

`constbuffer_array_splitter` is a helper module that can take a const buffer array and transform it into another const buffer array where each const buffer is of size `max_buffer_size` (except the last one which may be smaller).

For example, it is used to upload large payloads to Blob store, split across large blocks of a maximum size.

## Exposed API

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_ARRAY_HANDLE, constbuffer_array_splitter_split, CONSTBUFFER_ARRAY_HANDLE, buffers, uint32_t, max_buffer_size);

MOCKABLE_FUNCTION(, TARRAY(CONSTBUFFER_ARRAY_HANDLE), constbuffer_array_splitter_split_to_array_of_array, CONSTBUFFER_ARRAY_HANDLE, buffers, uint32_t, max_buffer_size, uint32_t*, split_buffer_arrays_count);
```

### constbuffer_array_splitter_split

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_ARRAY_HANDLE, constbuffer_array_splitter_split, CONSTBUFFER_ARRAY_HANDLE, buffers, uint32_t, max_buffer_size);
```

This takes the `buffers` from a `CONSTBUFFER_ARRAY_HANDLE` and splits them into a new `CONSTBUFFER_ARRAY_HANDLE` where each const buffer is of size `max_buffer_size` (or smaller for the final remaining buffer). The caller is responsible for cleaning up the returned `CONSTBUFFER_ARRAY_HANDLE`.

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_001: [** If `buffers` is `NULL` then `constbuffer_array_splitter_split` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_002: [** If `max_buffer_size` is `0` then `constbuffer_array_splitter_split` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [** `constbuffer_array_splitter_split` shall call `constbuffer_array_get_buffer_count`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_019: [** If the buffer count is 0 then `constbuffer_array_splitter_split` shall call `constbuffer_array_create_empty` and return the result. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [** `constbuffer_array_splitter_split` shall call `constbuffer_array_get_all_buffers_size` for `buffers` and store the result as `remaining_buffer_size`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_020: [** If the `remaining_buffer_size` is `0` (all buffers are empty) then `constbuffer_array_splitter_split` shall call `constbuffer_array_create_empty` and return the result. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [** `constbuffer_array_splitter_split` shall allocate an array of `CONSTBUFFER_HANDLE` of size `remaining_buffer_size / max_buffer_size` (rounded up). **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [** `constbuffer_array_splitter_split` shall initialize the current buffer index to `0` and the current buffer offset to `0`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [** `constbuffer_array_splitter_split` shall get the first buffer in `buffers` that is not empty. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [** While the `remaining_buffer_size` is greater than 0: **]**

  - **SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [** `constbuffer_array_splitter_split` shall allocate memory of size `min(max_buffer_size, remaining_buffer_size)`. **]**

  - **SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [** `constbuffer_array_splitter_split` shall copy data from the `buffers` starting at the current buffer index and buffer offset until it has filled the allocated memory. **]**

  - **SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [** `constbuffer_array_splitter_split` shall update the current buffer offset as it copies the memory. **]**

  - **SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [** If the end of the current buffer in `buffers` is reached then `constbuffer_array_splitter_split` shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). **]**

  - **SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [** `constbuffer_array_splitter_split` shall decrement the `remaining_buffer_size` by the amount of data copied. **]**

  - **SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [** `constbuffer_array_splitter_split` shall call `CONSTBUFFER_CreateWithMoveMemory` for the allocated memory and store it in the allocated array. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [** `constbuffer_array_splitter_split` shall call `constbuffer_array_create` with the allocated array of buffer handles as `split_buffers`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [** `constbuffer_array_splitter_split` shall succeed and return the `split_buffers`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [** If there are any other failures then `constbuffer_array_splitter_split` shall fail and return `NULL`. **]**

### constbuffer_array_splitter_split_to_array_of_array

```c
MOCKABLE_FUNCTION(, TARRAY(CONSTBUFFER_ARRAY_HANDLE), constbuffer_array_splitter_split_to_array_of_array, CONSTBUFFER_ARRAY_HANDLE, buffers, uint32_t, max_buffer_size, uint32_t*, split_buffer_arrays_count);
```

This takes the `buffers` from a `CONSTBUFFER_ARRAY_HANDLE` and splits them into a new `TARRAY(CONSTBUFFER_ARRAY_HANDLE)` where each `CONSTBUFFER_ARRAY_HANDLE` is of size `max_buffer_size` (or smaller for the final remaining buffer). The caller is responsible for cleaning up the returned `TARRAY(CONSTBUFFER_ARRAY_HANDLE)`.

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_001: [** If `buffers` is `NULL` then `constbuffer_array_splitter_split_to_array_of_array` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_002: [** If `max_buffer_size` is `0` then `constbuffer_array_splitter_split_to_array_of_array` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_026: [** If `split_buffer_arrays_count` is `NULL` then `constbuffer_array_splitter_split_to_array_of_array` shall fail and return `NULL`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [** `constbuffer_array_splitter_split_to_array_of_array` shall call `constbuffer_array_get_buffer_count` to get the total number of buffers. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [** `constbuffer_array_splitter_split_to_array_of_array` shall call `constbuffer_array_get_all_buffers_size` for `buffers` to obtain the total size of all buffers in `buffers`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_024: [** If the total size for all buffers in `buffers` is `0` or `buffer_count` is `0`: **]**

- **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_021: [** `constbuffer_array_splitter_split_to_array_of_array` shall call `TARRAY_CREATE_WITH_CAPACITY` with size 1. **]**

- **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_007: [** `constbuffer_array_splitter_split_to_array_of_array` shall call `constbuffer_array_create_empty` and store the created const buffer array in the first entry of the `TARRAY` that was created. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_022: [** If `remaining_buffers_size` is smaller or equal to `max_buffer_size`, `constbuffer_array_splitter_split_to_array_of_array` shall call `TARRAY_CREATE_WITH_CAPACITY` with size 1, inc ref the original buffer and return it.  **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_023: [** `constbuffer_array_splitter_split_to_array_of_array` shall allocate a TARRAY of `CONSTBUFFER_HANDLE` of size total size of all buffers in `buffers` divided by `max_buffer_size` (rounded up). **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_010: [** `constbuffer_array_splitter_split_to_array_of_array` shall initialize the start buffer index and offset to `0`, current buffer count to `0` and end buffer size to `0`. **]**

For every buffer in the original buffer:

- **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_011: [** `constbuffer_array_splitter_split_to_array_of_array` shall get the buffer currently checking for the size by calling `constbuffer_array_get_buffer`. **]**

- **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_012: [** `constbuffer_array_splitter_split_to_array_of_array` shall get the buffer content by calling `CONSTBUFFER_GetContent`. **]**

- **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_016: [** If current buffer size added the current sub-tarray size is smaller than `max_buffer_size`, `constbuffer_array_splitter_split_to_array_of_array` shall include the current buffer to the current sub-tarray. **]**

- **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [** If current buffer is the last buffer in the original `constbuffer_array`, `constbuffer_array_splitter_split_to_array_of_array` shall store the sub-tarray with size smaller than `max_buffer_size` to result.  **]**

- **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_018: [** If current buffer size added the current sub-tarray size is greater than `max_buffer_size`, then `constbuffer_array_splitter_split_to_array_of_array` shall get part of the current buffer as end buffer and added a new array into the result until the remaining size for the current buffer is smaller than `max_buffer_size`. **]**

 - **SRS_CONSTBUFFER_ARRAY_SPLITTER_07_025: [** If current buffer size added the current sub-tarray size is equal to `max_buffers_size`, then `constbuffer_array_splitter_split_to_array_of_array` shall include any consecutive empty buffers right after the current buffer to the new array which will be added to the result. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_019: [** On any failure, `constbuffer_array_splitter_split_to_array_of_array` dec ref the sub-tarrays by calling `constbuffer_array_dec_ref`. **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [** `constbuffer_array_splitter_split_to_array_of_array` shall succeed and return the new `TARRAY(CONSTBUFFER_ARRAY_HANDLE)` and write the count of used constbuffer array in `split_buffer_arrays_count`.  **]**

**SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [** If there are any other failures then `constbuffer_array_splitter_split_to_array_of_array` shall fail and return `NULL`. **]**