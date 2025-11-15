// Copyright (c) Microsoft. All rights reserved.
#include <stdint.h>

#include "c_logging/logger.h"
#include "c_pal/sync.h"
#include "c_pal/interlocked.h"

#include "c_util/critical_section.h"

int critical_section_enter(volatile_atomic int32_t* access_value)
{
    int result;

    if (access_value == NULL)
    {
        /*Codes_SRS_CRITICAL_SECTION_18_001: [ If access_value is NULL, critical_section_enter shall fail and return a non-zero value. ]*/
        LogError("Invalid args: volatile_atomic int32_t* access_value=%p", access_value);
        result = MU_FAILURE;
    }
    else
    {
        do
        {
            /*Codes_SRS_CRITICAL_SECTION_18_002: [ critical_section_enter shall call interlocked_compare_exchange to set access_value to 1 if it was previously 0. ]*/
            int32_t current_val = interlocked_compare_exchange(access_value, 1, 0);
            /*Codes_SRS_CRITICAL_SECTION_18_003: [ If interlocked_compare_exchange indicates that access_value was changed from 0 to 1, critical_section_enter shall succeed and return 0. ]*/
            if (current_val == 0)
            {
                result = 0;
                goto all_ok;
            }
            else
            {
                /*Codes_SRS_CRITICAL_SECTION_18_004: [ Otherwise, critical_section_enter shall call wait_on_address passing access_value. ]*/
                (void)wait_on_address(access_value, current_val, UINT32_MAX);
                /*Codes_SRS_CRITICAL_SECTION_18_005: [ After wait_on_address returns, critical_section_enter shall loop around to the beginning of the function to call interlocked_compare_exchange again. ]*/
            }
        } while (true);
    }

all_ok:
    return result;
}

int critical_section_leave(volatile_atomic int32_t* access_value)
{
    int result;

    if (access_value == NULL)
    {
        /*Codes_SRS_CRITICAL_SECTION_18_006: [ If access_value is NULL, critical_section_leave shall fail and return a non-zero value. ]*/
        LogError("Invalid args: volatile_atomic int32_t* access_value=%p", access_value);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_CRITICAL_SECTION_18_007: [ critical_section_leave shall call interlocked_exchange to set access_value to 0. ]*/
        (void)interlocked_exchange(access_value, 0);
        /*Codes_SRS_CRITICAL_SECTION_18_008: [ critical_section_leave shall call wake_by_address_single to wake any threads that may be waiting for access_value to change. ]*/
        wake_by_address_single(access_value);
        /*Codes_SRS_CRITICAL_SECTION_18_009: [ critical_section_leave shall succeed and return 0.]*/
        result = 0;
    }

    return result;
}

