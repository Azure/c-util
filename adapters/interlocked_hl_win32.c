// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <limits.h>

#include "windows.h"

#include "azure_c_util/xlogging.h"

#include "azure_c_util/interlocked_hl.h"

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_Add64WithCeiling, LONG64 volatile*, Addend, LONG64, Ceiling, LONG64, Value, LONG64*, originalAddend)
{
    INTERLOCKED_HL_RESULT result;
    /*Codes_SRS_INTERLOCKED_HL_02_001: [ If Addend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
    /*Codes_SRS_INTERLOCKED_HL_02_006: [ If originalAddend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (
        (Addend == NULL) ||
        (originalAddend == NULL)
        )
    {
        LogError("invalid arguments LONGLONG volatile * Addend=%p, LONGLONG Ceiling=%I64d, LONGLONG Value=%I64d, LONGLONG* originalAddend=%p",
            Addend, Ceiling, Value, originalAddend);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        while(1)
        {
            /*checking if Addend + Value is representable*/
            LONGLONG addend_copy;
            LONGLONG expected_operation_result;

            addend_copy = InterlockedAdd64(Addend, 0);

            /*Codes_SRS_INTERLOCKED_HL_02_003: [ If Addend + Value would overflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
            if (
                ((addend_copy >= 0) && (Value > LLONG_MAX - addend_copy)) ||
                ((addend_copy < 0) && (Value < LLONG_MIN - addend_copy))
                )
            {
                /*Codes_SRS_INTERLOCKED_HL_02_002: [ If Addend + Value would underflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
                /*would overflow*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
            else
            {
                expected_operation_result = addend_copy + Value;
                /*Codes_SRS_INTERLOCKED_HL_02_004: [ If Addend + Value would be greater than Ceiling then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
                if (expected_operation_result > Ceiling)
                {
                    result = INTERLOCKED_HL_ERROR;
                    break;
                }
                else
                {
                    /*Codes_SRS_INTERLOCKED_HL_02_005: [ Otherwise, InterlockedHL_Add64WithCeiling shall atomically write in Addend the sum of Addend and Value, succeed and return INTERLOCKED_HL_OK. ]*/
                    if (InterlockedCompareExchange64(Addend, expected_operation_result, addend_copy) == addend_copy)
                    {
                        /*Codes_SRS_INTERLOCKED_HL_02_007: [ In all failure cases InterlockedHL_Add64WithCeiling shall not modify Addend or originalAddend*/
                        *originalAddend = addend_copy;
                        result = INTERLOCKED_HL_OK;
                        break;
                    }
                    else
                    {
                        /*go back to while(1)*/
                    }
                }
            }
        }
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue, LONG volatile*, address, LONG, value, DWORD, milliseconds)
{
    INTERLOCKED_HL_RESULT result;

    /* Codes_SRS_INTERLOCKED_HL_01_002: [ If address is NULL, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (address == NULL)
    {
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        LONG current_value;

        do
        {
            /* Codes_SRS_INTERLOCKED_HL_01_007: [ When WaitOnAddress succeeds, the value at address shall be compared to the target value passed in value by using InterlockedAdd. ]*/
            current_value = InterlockedAdd(address, 0);
            if (current_value == value)
            {
                /* Codes_SRS_INTERLOCKED_HL_01_003: [ If the value at address is equal to value, InterlockedHL_WaitForValue shall return INTERLOCKED_HL_OK. ]*/
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_01_008: [ If the value at address does not match, InterlockedHL_WaitForValue shall issue another call to WaitOnAddress. ]*/

            /* Codes_SRS_INTERLOCKED_HL_01_004: [ If the value at address is not equal to value, InterlockedHL_WaitForValue shall wait until the value at address changes in order to compare it again to value by using WaitOnAddress. ]*/
            /* Codes_SRS_INTERLOCKED_HL_01_005: [ When waiting for the value at address to change, the milliseconds argument value shall be used as timeout. ]*/
            if (!WaitOnAddress(address, &current_value, sizeof(current_value), milliseconds))
            {
                /* Codes_SRS_INTERLOCKED_HL_01_006: [ If WaitOnAddress fails, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue64, LONG64 volatile*, address, LONG64, value, DWORD, milliseconds)
{
    INTERLOCKED_HL_RESULT result;

    /*Codes_SRS_INTERLOCKED_HL_02_021: [ If address is NULL then InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (address == NULL)
    {
        LogError("invalid arguments LONG64 volatile*, address=%p, LONG64, value=%" PRId64 ", DWORD, milliseconds=%" PRIu32 "",
            address, value, milliseconds);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        LONG64 current_value;

        do
        {
            /* Codes_SRS_INTERLOCKED_HL_02_025: [ When WaitOnAddress succeeds, the value at address shall be compared to the target value passed in value by using InterlockedAdd64. ]*/
            current_value = InterlockedAdd64(address, 0);
            if (current_value == value)
            {
                /* Codes_SRS_INTERLOCKED_HL_02_022: [ If the value at address is equal to value then InterlockedHL_WaitForValue64 shall return INTERLOCKED_HL_OK. ]*/
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_02_026: [ If the value at address does not match, InterlockedHL_WaitForValue64 shall issue another call to WaitOnAddress. ]*/

            /* Codes_SRS_INTERLOCKED_HL_02_023: [ If the value at address is not equal to value, InterlockedHL_WaitForValue64 shall wait until the value at address changes in order to compare it again to value by using WaitOnAddress. ]*/
            /* Codes_SRS_INTERLOCKED_HL_02_024: [ When waiting for the value at address to change, the milliseconds argument value shall be used as timeout. ]*/
            if (!WaitOnAddress(address, &current_value, sizeof(current_value), milliseconds))
            {
                LogLastError("failure in WaitOnAddress(address=%p, &current_value=%p, sizeof(current_value)=%zu, milliseconds=%" PRIu32 ")",
                    address, current_value, sizeof(current_value), milliseconds);
                /* Codes_SRS_INTERLOCKED_HL_02_027: [ If WaitOnAddress fails, InterlockedHL_WaitForValue64 shall fail and return INTERLOCKED_HL_ERROR. ]*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }

    return result;
}


IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue, LONG volatile*, address, LONG, value, DWORD, milliseconds)
{
    INTERLOCKED_HL_RESULT result;

    /* Codes_SRS_INTERLOCKED_HL_42_001: [ If address is NULL, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (address == NULL)
    {
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        LONG current_value;

        do
        {
            /* Codes_SRS_INTERLOCKED_HL_42_005: [ When WaitOnAddress succeeds, the value at address shall be compared to the target value passed in value by using InterlockedAdd. ]*/
            current_value = InterlockedAdd(address, 0);
            if (current_value != value)
            {
                /* Codes_SRS_INTERLOCKED_HL_42_002: [ If the value at address is not equal to value, InterlockedHL_WaitForNotValue shall return INTERLOCKED_HL_OK. ]*/
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_42_006: [ If the value at address matches, InterlockedHL_WaitForNotValue shall issue another call to WaitOnAddress. ]*/

            /* Codes_SRS_INTERLOCKED_HL_42_003: [ If the value at address is equal to value, InterlockedHL_WaitForNotValue shall wait until the value at address changes in order to compare it again to value by using WaitOnAddress. ]*/
            /* Codes_SRS_INTERLOCKED_HL_42_004: [ When waiting for the value at address to change, the milliseconds argument value shall be used as timeout. ]*/
            if (!WaitOnAddress(address, &current_value, sizeof(current_value), milliseconds))
            {
                LogLastError("failure in WaitOnAddress(address=%p, &current_value=%p, sizeof(current_value)=%zu, milliseconds=%" PRIu32 ")",
                    address, &current_value, sizeof(current_value), milliseconds);
                /* Codes_SRS_INTERLOCKED_HL_42_007: [ If WaitOnAddress fails, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchange64If, LONG64 volatile*, target, LONG64, exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF, compare, LONG64*, original_target)
{
    INTERLOCKED_HL_RESULT result;
    if (
        /*Codes_SRS_INTERLOCKED_HL_02_008: [ If target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (target == NULL) ||
        /*Codes_SRS_INTERLOCKED_HL_02_009: [ If compare is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (compare == NULL) ||
        /*Codes_SRS_INTERLOCKED_HL_02_010: [ If original_target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
        (original_target == NULL)
        )
    {
        LogError("invalid arguments LONG64 volatile* target=%p, LONG64 exchange=%" PRId64 ", INTERLOCKED_COMPARE_EXCHANGE_IF compare=%p original_target=%p",
            target, exchange, compare, original_target);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ]*/
        LONG64 copyOfTarget = InterlockedAdd64(target, 0);

        /*Codes_SRS_INTERLOCKED_HL_02_012: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchange64If shall exchange target with exchange. ]*/
        if (compare(copyOfTarget, exchange))
        {
            /*Codes_SRS_INTERLOCKED_HL_02_013: [ If target changed meanwhile then InterlockedHL_CompareExchange64If shall return return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ]*/
            /*Codes_SRS_INTERLOCKED_HL_02_014: [ If target did not change meanwhile then InterlockedHL_CompareExchange64If shall return return INTERLOCKED_HL_OK and shall peform the exchange of values. ]*/
            if (InterlockedCompareExchange64(target, exchange, copyOfTarget) == copyOfTarget)
            {
                result = INTERLOCKED_HL_OK;
            }
            else
            {
                result = INTERLOCKED_HL_CHANGED;
            }
        }
        else
        {
            /*Codes_SRS_INTERLOCKED_HL_02_015: [ If compare returns false then InterlockedHL_CompareExchange64If shall not perform any exchanges and return INTERLOCKED_HL_OK. ]*/
            result = INTERLOCKED_HL_OK;
        }
        /*Codes_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ]*/
        *original_target = copyOfTarget;
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake, LONG volatile*, address, LONG, value)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /*Codes_SRS_INTERLOCKED_HL_02_020: [ If address is NULL then InterlockedHL_SetAndWake shall fail and return INTERLOCKED_HL_ERROR. ]*/
        LogError("invalid arguments LONG volatile* address=%p, LONG value=%" PRId32 "",
            address, value);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_017: [ InterlockedHL_SetAndWake shall set address to value. ]*/
        (void)InterlockedExchange(address, value);

        /*Codes_SRS_INTERLOCKED_HL_02_018: [ InterlockedHL_SetAndWake shall call WakeByAddressSingle. ]*/
        WakeByAddressSingle((void*)address);

        /*Codes_SRS_INTERLOCKED_HL_02_019: [ InterlockedHL_SetAndWake shall succeed and return INTERLOCKED_HL_OK. ]*/
        result = INTERLOCKED_HL_OK;
    }
    return result;

}
