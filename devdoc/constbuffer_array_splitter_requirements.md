`constbuffer_array_splitter` requirements
================

## Overview

`constbuffer_array_splitter` is a helper module that can take a const buffer array and transform it into another const buffer array where each const buffer is of size `max_buffer_size` (except the last one which may be smaller).

For example, it is used to upload large payloads to Blob store, split across large blocks of a maximum size.

## Exposed API

```c
MOCKABLE_FUNCTION(, CONSTBUFFER_ARRAY_HANDLE, constbuffer_array_splitter_split, CONSTBUFFER_ARRAY_HANDLE, buffers, uint32_t, max_buffer_size);
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
