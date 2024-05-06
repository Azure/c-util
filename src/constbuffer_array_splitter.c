// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/constbuffer_array.h"

#include "c_util/constbuffer_array_splitter.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

CONSTBUFFER_ARRAY_HANDLE constbuffer_array_splitter_split(CONSTBUFFER_ARRAY_HANDLE buffers, uint32_t max_buffer_size)
{
    CONSTBUFFER_ARRAY_HANDLE result;

    if (
        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_001: [ If buffers is NULL then constbuffer_array_splitter_split shall fail and return NULL. ]*/
        buffers == NULL ||
        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_002: [ If max_buffer_size is 0 then constbuffer_array_splitter_split shall fail and return NULL. ]*/
        max_buffer_size == 0
        )
    {
        LogError("Invalid args : CONSTBUFFER_ARRAY_HANDLE buffers = %p, size_t max_buffer_size = %" PRIu32,
            buffers, max_buffer_size);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
        uint32_t buffer_count;
        (void)constbuffer_array_get_buffer_count(buffers, &buffer_count);

        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_019: [ If the buffer count is 0 then constbuffer_array_splitter_split shall call constbuffer_array_create_empty and return the result. ]*/
        if (buffer_count == 0)
        {
            result = constbuffer_array_create_empty();

            if (result == NULL)
            {
                LogError("constbuffer_array_create_empty failed");
            }
            // return as-is
        }
        else
        {
            /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
            uint32_t remaining_buffer_size;
            // NOTE: the total block size is limited to UINT32_MAX
            // If we need to evict blocks with more than 4GB of data (including all headers) then this needs to be updated to get a 64-bit size
            int temp_result = constbuffer_array_get_all_buffers_size(buffers, &remaining_buffer_size);
            if (temp_result != 0)
            {
                /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
                LogError("constbuffer_array_get_all_buffers_size failed");
                result = NULL;
            }
            else
            {
                /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_020: [ If the remaining_buffer_size is 0 (all buffers are empty) then constbuffer_array_splitter_split shall call constbuffer_array_create_empty and return the result. ]*/
                if (remaining_buffer_size == 0)
                {
                    result = constbuffer_array_create_empty();

                    if (result == NULL)
                    {
                        LogError("constbuffer_array_create_empty failed");
                    }
                    // return as-is
                }
                else
                {
                    /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
                    uint32_t split_buffer_count = (remaining_buffer_size + max_buffer_size - 1) / max_buffer_size;

                    CONSTBUFFER_HANDLE* split_buffers = malloc_2(split_buffer_count, sizeof(CONSTBUFFER_HANDLE));
                    if (split_buffers == NULL)
                    {
                        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
                        LogError("Failed to allocate %" PRIu32 " buffer handle array", split_buffer_count);
                        result = NULL;
                    }
                    else
                    {
                        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
                        uint32_t current_buffer_index = 0;
                        uint32_t current_buffer_offset = 0;
                        uint32_t current_split_buffer_index = 0;

                        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
                        const CONSTBUFFER* current_buffer = constbuffer_array_get_buffer_content(buffers, current_buffer_index);

                        while (current_buffer->size == 0)
                        {
                            ++current_buffer_index;
                            current_buffer = constbuffer_array_get_buffer_content(buffers, current_buffer_index);
                        }

                        /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
                        while (remaining_buffer_size > 0)
                        {
                            bool failed = false;

                            /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
                            uint32_t next_split_buffer_size = MIN(max_buffer_size, remaining_buffer_size);

                            unsigned char* split_buffer_memory = malloc(next_split_buffer_size);
                            if (split_buffer_memory == NULL)
                            {
                                /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
                                LogError("Failed to allocate memory (%" PRIu32 " bytes) for buffer copy", next_split_buffer_size);
                                failed = true;
                            }
                            else
                            {
                                /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
                                for (uint32_t i = 0; i < next_split_buffer_size; ++i)
                                {
                                    split_buffer_memory[i] = current_buffer->buffer[current_buffer_offset];

                                    /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
                                    ++current_buffer_offset;

                                    /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
                                    while (current_buffer != NULL &&
                                            current_buffer_offset >= current_buffer->size)
                                    {
                                        current_buffer_offset = 0;
                                        ++current_buffer_index;

                                        if (remaining_buffer_size - (i + 1) > 0)
                                        {
                                            current_buffer = constbuffer_array_get_buffer_content(buffers, current_buffer_index);
                                        }
                                        else
                                        {
                                            // This should be the end of the copy
                                            current_buffer = NULL;
                                        }
                                    }
                                }

                                /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
                                split_buffers[current_split_buffer_index] = CONSTBUFFER_CreateWithMoveMemory(split_buffer_memory, next_split_buffer_size);

                                if (split_buffers[current_split_buffer_index] == NULL)
                                {
                                    /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
                                    LogError("CONSTBUFFER_CreateWithMoveMemory failed for buffer %" PRIu32, current_split_buffer_index);
                                    failed = true;
                                }
                                else
                                {
                                    split_buffer_memory = NULL;
                                    ++current_split_buffer_index;

                                    /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
                                    remaining_buffer_size -= next_split_buffer_size;
                                }

                                if (split_buffer_memory != NULL)
                                {
                                    free(split_buffer_memory);
                                }
                            }

                            if (failed)
                            {
                                break;
                            }
                        }

                        if (remaining_buffer_size > 0)
                        {
                            result = NULL;
                        }
                        else
                        {
                            /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
                            result = constbuffer_array_create(split_buffers, split_buffer_count);

                            if (result == NULL)
                            {
                                /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
                                LogError("constbuffer_array_create failed");
                                // return as-is
                            }
                            else
                            {
                                /*Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
                            }
                        }

                        for (uint32_t i = 0; i < current_split_buffer_index; ++i)
                        {
                            CONSTBUFFER_DecRef(split_buffers[i]);
                        }
                        free(split_buffers);
                    }
                }
            }
        }
    }

    return result;
}

TARRAY(CONSTBUFFER_ARRAY_HANDLE) constbuffer_array_splitter_split_to_array_of_array(CONSTBUFFER_ARRAY_HANDLE buffers, uint32_t max_buffer_size)
{
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = NULL;

    if (
        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_001: [ If buffers is NULL then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
        buffers == NULL ||
        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_002: [ If max_buffer_size is 0 then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
        max_buffer_size == 0)
    {
        LogError("Invalid args : CONSTBUFFER_ARRAY_HANDLE buffers = %p, size_t max_buffer_size = %" PRIu32 "",
            buffers, max_buffer_size);
    }
    else
    {
        uint32_t buffer_array_count = 0; //used to store total number of constbuffer_array already stored in result -> release when failed
        uint32_t buffer_count;

        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
        (void)constbuffer_array_get_buffer_count(buffers, &buffer_count);

        uint32_t remaining_buffer_size;

        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
        int temp_result = constbuffer_array_get_all_buffers_size(buffers, &remaining_buffer_size);
        if (temp_result != 0)
        {
            /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
            LogError("constbuffer_array_get_all_buffers_size failed");
        }
        else
        {
            /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_024: [ If the total size for all buffers in buffers is 0 or buffer_count is 0: ]*/
            if (buffer_count == 0 || remaining_buffer_size == 0)
            {
                /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_021: [ constbuffer_array_splitter_split_to_array_of_array shall call TARRAY_CREATE_WITH_CAPACITY with size 1. ]*/
                TARRAY(CONSTBUFFER_ARRAY_HANDLE) temp = TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1);

                if (temp == NULL)
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                    LogError("TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(split_array_count=1) failed");
                }
                else
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_007: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_create_empty and store the created const buffer array in the first entry of the TARRAY that was created. ]*/
                    temp->arr[0] = constbuffer_array_create_empty();
                    if (temp->arr[0] == NULL)
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                        LogError("constbuffer_array_create_empty failed");
                        TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&temp, NULL);
                    }
                    else
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
                        TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(&result, &temp);
                    }
                }
            }
            else if (remaining_buffer_size <= max_buffer_size)
            {
                /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_022: [ If remaining_buffers_size is smaller or equal to max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall call TARRAY_CREATE_WITH_CAPACITY with size 1, inc ref the original buffer and return it. ]*/
                TARRAY(CONSTBUFFER_ARRAY_HANDLE) temp = TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1);
                if (temp == NULL)
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                    LogError("TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(split_array_count=1) failed");
                }
                else
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
                    temp->arr[0] = buffers;
                    constbuffer_array_inc_ref(buffers);
                    TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(&result, &temp);
                }
            }
            else
            {
                /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_023: [ constbuffer_array_splitter_split_to_array_of_array shall allocate a TARRAY of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
                uint32_t split_array_count = (remaining_buffer_size + max_buffer_size - 1) / max_buffer_size;
                TARRAY(CONSTBUFFER_ARRAY_HANDLE) temp = TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(split_array_count);

                if (temp == NULL)
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                    LogError("TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(split_array_count=%" PRIu32 ") failed", split_array_count);
                }
                else
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_010: [ constbuffer_array_splitter_split_to_array_of_array shall initialize the start buffer index and offset to 0, current buffer count to 0 and end buffer size to 0. ]*/
                    uint32_t start_buffer_index = 0;
                    uint32_t start_buffer_offset = 0;
                    uint32_t end_buffer_size = 0;
                    uint32_t current_buffer_count = 0; //buffer count in sub-tarray

                    uint32_t current_buffer_size = 0; //sub-tarray size
                    uint32_t index = 0; //index in result tarray

                    bool failed = false;

                    for (uint32_t i = 0; i < buffer_count; i++)
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_011: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer currently checking for the size by calling constbuffer_array_get_buffer. ]*/
                        CONSTBUFFER_HANDLE curr_buffer = constbuffer_array_get_buffer(buffers, i);
                        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_012: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer content by calling CONSTBUFFER_GetContent. ]*/
                        const CONSTBUFFER* buffer = CONSTBUFFER_GetContent(curr_buffer);

                        if(current_buffer_size + buffer->size < max_buffer_size)
                        {
                            /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_016: [ If current buffer size added the current sub - tarray size is smaller than max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall include the current buffer to the current sub - tarray. ]*/
                            current_buffer_count++;
                            current_buffer_size += buffer->size;
                            end_buffer_size = buffer->size;

                            if(i == buffer_count - 1)
                            {
                                /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [ If current buffer is the last buffer in the original constbuffer_array, constbuffer_array_splitter_split_to_array_of_array shall store the sub - tarray with size smaller than max_buffer_size to result. ]*/
                                CONSTBUFFER_ARRAY_HANDLE arr = constbuffer_array_create_from_buffer_offset_and_count(buffers, start_buffer_index, current_buffer_count, start_buffer_offset, end_buffer_size);

                                if(arr == NULL)
                                {
                                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                                    LogError("constbuffer_array_create_from_buffer_offset_and_count failed buffers=%p, uint32_t start_buffer_index=%" PRIu32 ", uint32_t buffer_count=%" PRIu32 ", uint32_t start_buffer_offset=%" PRIu32 ", uint32_t end_buffer_size=%" PRIu32 "",
                                        buffers, start_buffer_index, current_buffer_count, start_buffer_offset, buffer->size);
                                    failed = true;
                                }
                                else
                                {
                                    temp->arr[index] = arr;
                                    buffer_array_count ++;
                                }
                            }
                        }
                        else if(current_buffer_size + buffer->size >= max_buffer_size)
                        {
                            /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_018: [ If current buffer size added the current sub - tarray size is greater than max_buffer_size, then constbuffer_array_splitter_split_to_array_of_array shall get part of the current buffer as end buffer and added a new array into the result until the remaining size for the current buffer is smaller than max_buffer_size. ]*/

                            current_buffer_count++;
                            end_buffer_size = max_buffer_size - current_buffer_size;

                            if (current_buffer_size + buffer->size == max_buffer_size)
                            {

                                while (++i < buffer_count)
                                {
                                    CONSTBUFFER_HANDLE next = constbuffer_array_get_buffer(buffers, i);
                                    const CONSTBUFFER* next_buffer = CONSTBUFFER_GetContent(next);
                                    if (next_buffer->size == 0)
                                    {
                                        current_buffer_count++;
                                        end_buffer_size = 0;
                                        CONSTBUFFER_DecRef(next);
                                    }
                                    else
                                    {
                                        CONSTBUFFER_DecRef(next);
                                        i--;
                                        break;
                                    }
                                }
                            }

                            CONSTBUFFER_ARRAY_HANDLE arr = constbuffer_array_create_from_buffer_offset_and_count(buffers, start_buffer_index, current_buffer_count, start_buffer_offset, end_buffer_size);
                            if(arr == NULL)
                            {
                                /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                                LogError("constbuffer_array_create_from_buffer_offset_and_count failed buffers=%p, uint32_t start_buffer_index=%" PRIu32 ", uint32_t buffer_count=%" PRIu32 ", uint32_t start_buffer_offset=%" PRIu32 ", uint32_t end_buffer_size=%" PRIu32 "",
                                    buffers, start_buffer_index, current_buffer_count, start_buffer_offset, end_buffer_size);
                                failed = true;
                            }
                            else
                            {
                                temp->arr[index] = arr;
                                buffer_array_count ++;
                                index++;
                                start_buffer_index = i;

                                //check if can get any sub-array inside current array
                                uint32_t current_buffer_remaining_size = buffer->size - end_buffer_size;
                                start_buffer_offset = end_buffer_size;
                                while (current_buffer_remaining_size > max_buffer_size)
                                {
                                    CONSTBUFFER_ARRAY_HANDLE arr1 = constbuffer_array_create_from_buffer_offset_and_count(buffers, start_buffer_index, 1, start_buffer_offset, max_buffer_size);
                                    if (arr1 == NULL)
                                    {
                                        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                                        LogError("constbuffer_array_create_from_buffer_offset_and_count failed buffers=%p, uint32_t start_buffer_index=%" PRIu32 ", uint32_t buffer_count=%" PRIu32 ", uint32_t start_buffer_offset=%" PRIu32 ", uint32_t end_buffer_size=%" PRIu32 "",
                                            buffers, start_buffer_index, 1, start_buffer_offset, max_buffer_size);
                                        failed = true;
                                        break;
                                    }
                                    else
                                    {
                                        temp->arr[index] = arr1;
                                        buffer_array_count++;
                                        index++;
                                        current_buffer_remaining_size -= max_buffer_size;
                                        start_buffer_offset += max_buffer_size;
                                    }
                                }
                                //reset for another sub array
                                if (current_buffer_remaining_size == 0)
                                {
                                    start_buffer_index = i + 1;
                                    start_buffer_offset = 0;
                                    current_buffer_count = 0;
                                    current_buffer_size = 0;
                                }
                                else
                                {
                                    /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [ If current buffer is the last buffer in the original constbuffer_array, constbuffer_array_splitter_split_to_array_of_array shall store the sub - tarray with size smaller than max_buffer_size to result. ]*/
                                    if (i == buffer_count - 1)
                                    {
                                        CONSTBUFFER_ARRAY_HANDLE arr2 = constbuffer_array_create_from_buffer_offset_and_count(buffers, start_buffer_index, 1, start_buffer_offset, current_buffer_remaining_size);
                                        if (arr2 == NULL)
                                        {
                                            /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
                                            LogError("constbuffer_array_create_from_buffer_offset_and_count failed buffers=%p, uint32_t start_buffer_index=%" PRIu32 ", uint32_t buffer_count=%" PRIu32 ", uint32_t start_buffer_offset=%" PRIu32 ", uint32_t end_buffer_size=%" PRIu32 "",
                                                buffers, start_buffer_index, 1, start_buffer_offset, max_buffer_size);
                                            failed = true;
                                        }
                                        else
                                        {
                                            temp->arr[index] = arr2;
                                            buffer_array_count++;
                                        }
                                    }
                                    else
                                    {
                                        current_buffer_count = 1;
                                        current_buffer_size = current_buffer_remaining_size;
                                    }
                                }
                            }
                        }
                        CONSTBUFFER_DecRef(curr_buffer);
                        if (failed)
                        {
                            break;
                        }
                    }

                    if (failed)
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_019: [ On any failure, constbuffer_array_splitter_split_to_array_of_array dec ref the sub - tarrays by calling constbuffer_array_dec_ref. ]*/
                        for (uint32_t i = 0; i < buffer_array_count; i++)
                        {
                            constbuffer_array_dec_ref(temp->arr[i]);
                        }
                        TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&temp, NULL);
                    }
                    else
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
                        TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(&result, &temp);
                    }
                }
            }
        }
    }
    return result;
}
