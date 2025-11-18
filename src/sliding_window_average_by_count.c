// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"

#include "c_util/sliding_window_average_by_count.h"

typedef struct SLIDING_WINDOW_AVERAGE_TAG
{
    SRW_LOCK_HANDLE lock;
    int32_t window_count;
    int64_t current_sum;
    int64_t current_count;
    int32_t next_available_slot;
    volatile double current_average;
    int64_t counts[];
} SLIDING_WINDOW_AVERAGE;

THANDLE_TYPE_DEFINE(SLIDING_WINDOW_AVERAGE);

static void sliding_window_average_by_count_dispose(SLIDING_WINDOW_AVERAGE* content)
{
    if (content == NULL)
    {
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_031: [sliding_window_average_by_count_dispose shall return if content is NULL.]
        LogError("SLIDING_WINDOW_AVERAGE* content is NULL");
    }
    else
    {
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_033 : [sliding_window_average_by_count_dispose shall call srw_lock_destroy on the lock.]
        srw_lock_destroy(content->lock);
    }
}

THANDLE(SLIDING_WINDOW_AVERAGE) sliding_window_average_by_count_create(int32_t window_count)
{
    THANDLE(SLIDING_WINDOW_AVERAGE) handle = NULL;
    if (window_count < 1)
    {
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_001: [ sliding_window_average_by_count_create shall return NULL is window_count is not >= 1. ]
        LogError("window_count is too small window_count=%" PRId32 "", window_count);
    }
    else
    {
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_002: [ sliding_window_average_by_count_create shall call THANDLE_MALLOC_FLEX to allocate SLIDING_WINDOW_AVERAGE struct and an array of int64_t the size of window_count. ]
        THANDLE(SLIDING_WINDOW_AVERAGE) temp_handle = THANDLE_MALLOC_FLEX(SLIDING_WINDOW_AVERAGE)(sliding_window_average_by_count_dispose, window_count, sizeof(int64_t));
        if (temp_handle == NULL)
        {
            // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_003: [ sliding_window_average_by_count_create shall return NULL if there are any errors. ]
            LogError("Failed to allocate SLIDING_WINDOW_AVERAGE struct with window_count= %" PRId32 "", window_count);
        }
        else
        {
            SLIDING_WINDOW_AVERAGE* internal_handle = THANDLE_GET_T(SLIDING_WINDOW_AVERAGE)(temp_handle);
            // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_026: [ sliding_window_average_by_count_create shall call srw_lock_create to create a lock for the SLIDING_WINDOW_AVERAGE struct. ]
            internal_handle->lock = srw_lock_create(false, "sliding_window_average");
            if (internal_handle->lock == NULL)
            {
                LogError("Failed to create SRW_LOCK_HANDLE");
                THANDLE_FREE(SLIDING_WINDOW_AVERAGE)((void*)temp_handle);
            }
            else
            {
                internal_handle->window_count = window_count;
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_004: [ sliding_window_average_by_count_create shall initialize all counts in window to zero. ]
                internal_handle->current_sum = 0;
                internal_handle->current_count = 0;
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_006: [ sliding_window_average_by_count_create shall initialize the next_available_slot to 0. ]
                internal_handle->next_available_slot = 0;
                internal_handle->current_average = 0.0;
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_005: [ sliding_window_average_by_count_create shall initialize the current sum, the current count, and the current average to zero. ]
                for (int i = 0; i < window_count; i++)
                {
                    internal_handle->counts[i] = 0;
                }
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_025: [ sliding_window_average_by_count_create shall return a non-null THANDLE(SLIDING_WINDOW_AVERAGE) on success. ]
                THANDLE_MOVE(SLIDING_WINDOW_AVERAGE)(&handle, &temp_handle);
            }
        }
    }
    return handle;
}

int sliding_window_average_by_count_add(THANDLE(SLIDING_WINDOW_AVERAGE) handle, int64_t next_count)
{
    int result;
    if (handle == NULL)
    {
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_008: [ sliding_window_average_by_count_add shall return a non-zero value if handle is NULL. ]
        LogError("Invalid arguments: THANDLE(SLIDING_WINDOW_AVERAGE) handle=%p, int64_t next_count=%" PRId64 "", handle, next_count);
        result = MU_FAILURE;
    }
    else
    {
        SLIDING_WINDOW_AVERAGE* internal_handle = THANDLE_GET_T(SLIDING_WINDOW_AVERAGE)(handle);
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_027: [ sliding_window_average_by_count_add shall call srw_lock_acquire_exclusive on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
        srw_lock_acquire_exclusive(internal_handle->lock);
        {
            int window_location = handle->next_available_slot % handle->window_count;

            // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_010: [ If next_available_slot is >= window_count ]
            if (internal_handle->next_available_slot >= handle->window_count)
            {
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_011: [ sliding_window_average_by_count_add shall subtract the value at next_available_slot%window_count from the current sum. ]
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_012: [ sliding_window_average_by_count_add shall add the next_count value to the current sum. ]
                internal_handle->current_sum += next_count - handle->counts[window_location];
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_013: [ sliding_window_average_by_count_add shall assign the next_count value at next_available_slot%window_count index. ]
                internal_handle->counts[window_location] = next_count;
            }
            // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_014: [ If the next_available_slot < window_count ]
            else
            {
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_015: [ sliding_window_average_by_count_add shall add the next_count value to the current sum ]
                internal_handle->current_sum += next_count;
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_016: [ sliding_window_average_by_count_add shall assign the next_slot value at next_available_slot index ]
                internal_handle->counts[window_location] = next_count;
                // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_017: [ sliding_window_average_by_count_add shall increment the current count. ]
                internal_handle->current_count++;
            }
            // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_009: [ sliding_window_average_by_count_add shall increment the next_available_slot. ]
            ++(internal_handle->next_available_slot);
            // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_018: [ sliding_window_average_by_count_add shall assign the current average to the current sum/current count. ]
            internal_handle->current_average = (double)handle->current_sum / handle->current_count;
        }
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_028: [ sliding_window_average_by_count_add shall call srw_lock_release_exclusive on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
        srw_lock_release_exclusive(internal_handle->lock);
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_019: [ sliding_window_average_by_count_add shall return zero on success. ]
        result = 0;
    }
    return result;
}

int sliding_window_average_by_count_get(THANDLE(SLIDING_WINDOW_AVERAGE) handle, double* average)
{
    int result;
    if (
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_021: [ sliding_window_average_by_count_get shall return a non-zero value if handle is NULL. ]
        handle == NULL ||
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_022: [ sliding_window_average_by_count_get shall return a non-zero value if average is NULL. ]
        average == NULL)
    {
        LogError("Invalid arguments: THANDLE(SLIDING_WINDOW_AVERAGE) handle=%p, double* average=%p", handle, average);
        result = MU_FAILURE;
    }
    else
    {
        SLIDING_WINDOW_AVERAGE* internal_handle = THANDLE_GET_T(SLIDING_WINDOW_AVERAGE)(handle);
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_029: [ sliding_window_average_by_count_get shall call srw_lock_acquire_shared on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
        srw_lock_acquire_shared(internal_handle->lock);
        {
            // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_023: [ sliding_window_average_by_count_get shall copy the current average into average. ]
            *average = handle->current_average;
        }
        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_030: [ sliding_window_average_by_count_get shall call srw_lock_release_shared on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
        srw_lock_release_shared(internal_handle->lock);

        // Codes_SRS_SLIDING_AVERAGE_WINDOW_45_024: [ sliding_window_average_by_count_get shall return zero on success. ]
        result = 0;
    }
    return result;
}
