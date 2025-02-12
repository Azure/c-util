// Copyright (c) Microsoft. All rights reserved.
#include <stdint.h>

#include "c_pal/sync.h"
#include "c_pal/interlocked.h"
#include "c_pal/log_critical_and_terminate.h"

#include "c_util/critical_section.h"

void enter_crit_section(volatile_atomic int32_t* access_value)
{
    if (access_value == NULL)
    {
        /*Codes_SRS_CRITICAL_SECTION_18_001: [ If access_value is NULL, enter_crit_section shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid args: volatile_atomic int32_t* access_value=%p", access_value);
    }
    else
    {
        do
        {
            /*Codes_SRS_CRITICAL_SECTION_18_002: [ enter_crit_section shall call interlocked_compare_exchange to set access_value to 1 if it was previously 0. ]*/
            int32_t current_val = interlocked_compare_exchange(access_value, 1, 0);
            /*Codes_SRS_CRITICAL_SECTION_18_003: [ If interlocked_compare_exchange indicates that access_value was changed from 0 to 1, enter_crit_section returns. ]*/
            if (current_val == 0)
            {
                break;
            }
            else
            {
                // Do Nothing wait for address
            }
            /*Codes_SRS_CRITICAL_SECTION_18_004: [ Otherwise, enter_crit_section shall call wait_on_address passing access_value. ]*/
            (void)wait_on_address(access_value, current_val, UINT32_MAX);
            /*Codes_SRS_CRITICAL_SECTION_18_005: [ After wait_on_address returns, enter_crit_section shall loop around to the beginning of the function to call interlocked_compare_exchange again. ]*/
        } while (true);
    }
}

void leave_crit_section(volatile_atomic int32_t* access_value)
{
    if (access_value == NULL)
    {
        /*Codes_SRS_CRITICAL_SECTION_18_006: [ If access_value is NULL, leave_crit_section shall terminate the process. ]*/
        LogCriticalAndTerminate("Invalid args: volatile_atomic int32_t* access_value=%p", access_value);
    }
    else
    {
        /*Codes_SRS_CRITICAL_SECTION_18_007: [ leave_crit_section shall call interlocked_exchange to set access_value to 0. ]*/
        (void)interlocked_exchange(access_value, 0);
        /*Codes_SRS_CRITICAL_SECTION_18_008: [ leave_crit_section shall call wake_by_address_single to wake any threads that may be waiting for access_value to change. ]*/
        wake_by_address_single(access_value);
    }
}

