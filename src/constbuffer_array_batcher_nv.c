// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/memory_data.h"

#include "c_util/constbuffer_array_batcher_nv.h"

CONSTBUFFER_ARRAY_HANDLE constbuffer_array_batcher_nv_batch(CONSTBUFFER_ARRAY_HANDLE* payloads, uint32_t count)
{
    CONSTBUFFER_ARRAY_HANDLE result;

    if (
        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_001: [ If payloads is NULL, constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
        (payloads == NULL) ||
        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_002: [ If count is 0, constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
        (count == 0) ||
        /*Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_02_001: [ If count is greater than UINT32_MAX / sizeof(uint32_t) - 1 then constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
        (count > (UINT32_MAX / sizeof(uint32_t)) - 1) /*this is needed because we might end up multiplying (count+1) by sizeof(uint32_t) in CONSTBUFFER_CreateWithMoveMemory*/
        )
    {
        LogError("CONSTBUFFER_ARRAY_HANDLE* payloads=%p, uint32_t count=%" PRIu32,
            payloads, count);
    }
    else
    {
        uint32_t i;
        uint32_t total_buffer_count = 0;
        uint32_t* header_memory;

        for (i = 0; i < count; i++)
        {
            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_023: [ If any of the payload const buffer arrays is NULL, constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
            if (payloads[i] == NULL)
            {
                break;
            }
        }

        if (i < count)
        {
            LogError("Payload %" PRIu32 " is NULL", i);
        }
        else
        {
            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_003: [ Otherwise constbuffer_array_batcher_nv_batch shall obtain the number of buffers used by each CONSTBUFFER_ARRAY. ]*/

            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_004: [ constbuffer_array_batcher_nv_batch shall allocate memory for the header buffer (enough to hold the entire batch header namingly (count + 1) uint32_t values). ]*/
            header_memory = malloc_2(count + 1, sizeof(uint32_t));
            if (header_memory == NULL)
            {
                /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_010: [ If any error occurrs, constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
                LogError("failure in malloc_2(count=%" PRIu32 " + 1, sizeof(uint32_t)=%zu)",
                    count, sizeof(uint32_t));
            }
            else
            {
                CONSTBUFFER_HANDLE* all_buffers;

                /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_005: [ count shall be written as the first uint32_t in the header memory. ]*/
                write_uint32_t((void*)&header_memory[0], count);

                for (i = 0; i < count; i++)
                {
                    uint32_t buffer_count;

                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_003: [ Otherwise constbuffer_array_batcher_nv_batch shall obtain the number of buffers used by each CONSTBUFFER_ARRAY. ]*/
                    (void)constbuffer_array_get_buffer_count(payloads[i], &buffer_count);

                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_006: [ The count of buffers for each array in payloads shall also be written in the header. ]*/
                    write_uint32_t((void*)&header_memory[i + 1], buffer_count);

                    total_buffer_count += buffer_count;

                    if ((total_buffer_count < buffer_count) || (total_buffer_count == UINT32_MAX))
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_010: [ If any error occurrs, constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
                        LogError("exceeding total_buffer_count=%" PRIu32 " buffer_count=%" PRIu32 "",
                            total_buffer_count, buffer_count);
                        break;
                    }
                }

                if (i != count)
                {
                    /*error in the loop above, exit*/
                }
                else
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_007: [ constbuffer_array_batcher_nv_batch shall allocate enough memory for all the buffer handles in all the arrays + one extra header buffer handle. ]*/
                    all_buffers = malloc_2(total_buffer_count + 1, sizeof(CONSTBUFFER_HANDLE)); /*previous code ensures that total_buffer_count + 1 doesn't overflow*/
                    if (all_buffers == NULL)
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_010: [ If any error occurrs, constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
                        LogError("failure in malloc_2(total_buffer_count=%" PRIu32 " + 1, sizeof(CONSTBUFFER_HANDLE)=%zu);",
                            total_buffer_count, sizeof(CONSTBUFFER_HANDLE));
                    }
                    else
                    {
                        uint32_t current_index = 0;

                        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_008: [ constbuffer_array_batcher_nv_batch shall populate the first handle in the newly allocated handles array with the header buffer handle. ]*/
                        all_buffers[current_index] = CONSTBUFFER_CreateWithMoveMemory((void*)header_memory, sizeof(uint32_t) * (count + 1)); /*previous code ensures that this multiplication is always possible*/
                        if (all_buffers[current_index] == NULL)
                        {
                            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_010: [ If any error occurrs, constbuffer_array_batcher_nv_batch shall fail and return NULL. ]*/
                            LogError("CONSTBUFFER_CreateWithMoveMemory failed");
                        }
                        else
                        {
                            current_index++;

                            header_memory = NULL;

                            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_009: [ constbuffer_array_batcher_nv_batch shall populate the rest of the handles in the newly allocated handles array with the const buffer handles obtained from the arrays in payloads. ]*/
                            for (i = 0; i < count; i++)
                            {
                                uint32_t buffer_count;
                                uint32_t j;

                                (void)constbuffer_array_get_buffer_count(payloads[i], &buffer_count);

                                for (j = 0; j < buffer_count; j++)
                                {
                                    all_buffers[current_index++] = constbuffer_array_get_buffer(payloads[i], j);
                                }
                            }

                            result = constbuffer_array_create(all_buffers, total_buffer_count + 1);
                            for (i = 0; i < current_index; i++)
                            {
                                CONSTBUFFER_DecRef(all_buffers[i]);
                            }

                            if (result == NULL)
                            {
                                LogError("constbuffer_array_create failed");
                            }
                            else
                            {
                                free(all_buffers);

                                goto all_ok;
                            }
                        }

                        free(all_buffers);
                    }
                }

                if (header_memory != NULL)
                {
                    free(header_memory);
                }
            }
        }
    }

    result = NULL;

all_ok:
    return result;
}

CONSTBUFFER_ARRAY_HANDLE* constbuffer_array_batcher_nv_unbatch(CONSTBUFFER_ARRAY_HANDLE batch, uint32_t* payload_count)
{
    CONSTBUFFER_ARRAY_HANDLE* result;

    if (
        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_011: [ If batch is NULL, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
        (batch == NULL) ||
        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_012: [ If payload_count is NULL, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
        (payload_count == NULL)
        )
    {
        LogError("Invalid arguments: CONSTBUFFER_ARRAY_HANDLE batch=%p, uint32_t* payload_count=%p",
            batch, payload_count);
    }
    else
    {
        uint32_t batch_buffer_count;
        uint32_t batch_payload_count;
        const CONSTBUFFER* header_buffer_content;
        uint32_t* header_buffer_memory;

        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_013: [ Otherwise, constbuffer_array_batcher_nv_unbatch shall obtain the number of buffers in batch. ]*/
        (void)constbuffer_array_get_buffer_count(batch, &batch_buffer_count);

        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_021: [ If there are not enough buffers in batch to properly create all the payloads, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
        if (batch_buffer_count == 0)
        {
            LogError("Insufficient buffers in batch");
        }
        else
        {
            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_014: [ constbuffer_array_batcher_nv_unbatch shall obtain the content of first (header) buffer in batch. ]*/
            header_buffer_content = constbuffer_array_get_buffer_content(batch, 0);

            if ((header_buffer_content->size < sizeof(uint32_t)) ||
                (header_buffer_content->size % sizeof(uint32_t) != 0))
            {
                /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_024: [ If the size of the first buffer is less than uint32_t or not a multiple of uint32_t, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
                LogError("Invalid header buffer size: %" PRIu32, (uint32_t)header_buffer_content->size);
            }
            else
            {
                header_buffer_memory = (void*)header_buffer_content->buffer;

                /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_015: [ constbuffer_array_batcher_nv_unbatch shall extract the number of buffer arrays batched by reading the first uint32_t. ]*/
                read_uint32_t((void*)&header_buffer_memory[0], &batch_payload_count);

                if (batch_payload_count == 0)
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_026: [ If the number of buffer arrays in the batch is 0, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
                    LogError("Batch with 0 payloads");
                }
                else if (((header_buffer_content->size / sizeof(uint32_t)) - 1) != batch_payload_count)
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_025: [ If the number of buffer arrays does not match the size of the first buffer, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
                    LogError("Header buffer size not matching number of payloads: payload count=%" PRIu32 ", header buffer size=%" PRIu32,
                        batch_payload_count, (uint32_t)header_buffer_content->size);
                }
                else
                {
                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_017: [ constbuffer_array_batcher_nv_unbatch shall allocate enough memory to hold the handles for buffer arrays that will be unbatched. ]*/
                    result = malloc_2(batch_payload_count, sizeof(CONSTBUFFER_ARRAY_HANDLE));
                    if (result == NULL)
                    {
                        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_022: [ If any error occurs, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
                        LogError("failure in malloc_2(batch_payload_count=%" PRIu32 ", sizeof(CONSTBUFFER_ARRAY_HANDLE)=%zu);",
                            batch_payload_count, sizeof(CONSTBUFFER_ARRAY_HANDLE));
                    }
                    else
                    {
                        uint32_t i;
                        uint32_t buffer_index = 1;

                        for (i = 0; i < batch_payload_count; i++)
                        {
                            uint32_t buffer_count;
                            uint32_t j;

                            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_016: [ constbuffer_array_batcher_nv_unbatch shall extract the number of buffers in each of the batched payloads reading the uint32_t values encoded in the rest of the first (header) buffer. ]*/
                            read_uint32_t((void*)&header_buffer_memory[i + 1], &buffer_count);

                            if (buffer_index + buffer_count > batch_buffer_count)
                            {
                                /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_021: [ If there are not enough buffers in batch to properly create all the payloads, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
                                LogError("Not enough buffers in batch");
                                break;
                            }
                            else
                            {
                                if (buffer_count == 0)
                                {
                                    result[i] = constbuffer_array_create_empty();
                                }
                                else
                                {
                                    CONSTBUFFER_HANDLE* payload_buffers = malloc_2(buffer_count, sizeof(CONSTBUFFER_HANDLE));
                                    if (payload_buffers == NULL)
                                    {
                                        /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_022: [ If any error occurs, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
                                        LogError("failure in malloc_2(buffer_count=%" PRIu32 ", sizeof(CONSTBUFFER_HANDLE)=%zu);",
                                            buffer_count, sizeof(CONSTBUFFER_HANDLE));
                                        break;
                                    }

                                    for (j = 0; j < buffer_count; j++)
                                    {
                                        payload_buffers[j] = constbuffer_array_get_buffer(batch, buffer_index++);
                                    }

                                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_018: [ constbuffer_array_batcher_nv_unbatch shall create a const buffer array for each of the payloads in the batch. ]*/
                                    result[i] = constbuffer_array_create(payload_buffers, buffer_count);
                                    for (j = 0; j < buffer_count; j++)
                                    {
                                        CONSTBUFFER_DecRef(payload_buffers[j]);
                                    }
                                    free(payload_buffers);
                                }

                                if (result[i] == NULL)
                                {
                                    /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_022: [ If any error occurs, constbuffer_array_batcher_nv_unbatch shall fail and return NULL. ]*/
                                    LogError("creating the buffer array for payload %" PRIu32 " failed", i);
                                    break;
                                }
                            }
                        }

                        if (i < batch_payload_count)
                        {
                            uint32_t j;

                            for (j = 0; j < i; j++)
                            {
                                constbuffer_array_dec_ref(result[j]);
                            }
                        }
                        else
                        {
                            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_019: [ On success constbuffer_array_batcher_nv_unbatch shall return the array of const buffer array handles that constitute the batch. ]*/
                            /* Codes_SRS_CONSTBUFFER_ARRAY_BATCHER_NV_01_020: [ On success constbuffer_array_batcher_nv_unbatch shall write in payload_count the number of const buffer arrays that are in the batch. ]*/
                            *payload_count = batch_payload_count;

                            goto all_ok;
                        }

                        free(result);
                    }
                }
            }
        }
    }

    result = NULL;

all_ok:
    return result;
}
