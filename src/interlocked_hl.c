// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <limits.h>

#include "c_logging/xlogging.h"

#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_util/interlocked_hl.h"

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES)

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_Add64WithCeiling, int64_t volatile_atomic*, Addend, int64_t, Ceiling, int64_t, Value, int64_t*, originalAddend)
{
    INTERLOCKED_HL_RESULT result;
    /*Codes_SRS_INTERLOCKED_HL_02_001: [ If Addend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
    /*Codes_SRS_INTERLOCKED_HL_02_006: [ If originalAddend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (
        (Addend == NULL) ||
        (originalAddend == NULL)
        )
    {
        LogError("invalid arguments int64_t volatile_atomic* Addend=%p, int64_t Ceiling=%" PRId64 ", int64_t Value=%" PRId64 ", int64_t* originalAddend=%p",
            Addend, Ceiling, Value, originalAddend);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        while(1)
        {
            /*checking if Addend + Value is representable*/
            int64_t addend_copy;
            int64_t expected_operation_result;

            addend_copy = interlocked_add_64(Addend, 0);

            /*Codes_SRS_INTERLOCKED_HL_02_003: [ If Addend + Value would overflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
            if (
                ((addend_copy >= 0) && (Value > INT64_MAX - addend_copy)) ||
                ((addend_copy < 0) && (Value < INT64_MIN - addend_copy))
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
                    if (interlocked_compare_exchange_64(Addend, expected_operation_result, addend_copy) == addend_copy)
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

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)
{
    INTERLOCKED_HL_RESULT result;

    /* Codes_SRS_INTERLOCKED_HL_01_002: [ If address is NULL, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (address == NULL)
    {
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        int32_t current_value;

        do
        {
            /* Codes_SRS_INTERLOCKED_HL_01_007: [ When wait_on_address succeeds, the value at address shall be compared to the target value passed in value by using interlocked_add. ]*/
            current_value = interlocked_add(address, 0);
            if (current_value == value)
            {
                /* Codes_SRS_INTERLOCKED_HL_01_003: [ If the value at address is equal to value, InterlockedHL_WaitForValue shall return INTERLOCKED_HL_OK. ]*/
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_01_008: [ If the value at address does not match, InterlockedHL_WaitForValue shall issue another call to wait_on_address. ]*/

            /* Codes_SRS_INTERLOCKED_HL_01_004: [ If the value at address is not equal to value, InterlockedHL_WaitForValue shall wait until the value at address changes in order to compare it again to value by using wait_on_address. ]*/
            /* Codes_SRS_INTERLOCKED_HL_01_005: [ When waiting for the value at address to change, the milliseconds argument value shall be used as timeout. ]*/
            if (!wait_on_address(address, current_value, milliseconds))
            {
                /* Codes_SRS_INTERLOCKED_HL_01_006: [ If wait_on_address fails, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_WaitForNotValue, int32_t volatile_atomic*, address, int32_t, value, uint32_t, milliseconds)
{
    INTERLOCKED_HL_RESULT result;

    /* Codes_SRS_INTERLOCKED_HL_42_001: [ If address is NULL, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
    if (address == NULL)
    {
        LogError("invalid arguments int32_t volatile_atomic* address=%p, int32_t value=%" PRId32 ", uint32_t milliseconds=%" PRIu32 "",
            address, value, milliseconds);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        int32_t current_value;

        do
        {
            /* Codes_SRS_INTERLOCKED_HL_42_005: [ When wait_on_address succeeds, the value at address shall be compared to the target value passed in value by using interlocked_add. ]*/
            current_value = interlocked_add(address, 0);
            if (current_value != value)
            {
                /* Codes_SRS_INTERLOCKED_HL_42_002: [ If the value at address is not equal to value, InterlockedHL_WaitForNotValue shall return INTERLOCKED_HL_OK. ]*/
                result = INTERLOCKED_HL_OK;
                break;
            }

            /* Codes_SRS_INTERLOCKED_HL_42_006: [ If the value at address matches, InterlockedHL_WaitForNotValue shall issue another call to wait_on_address. ]*/

            /* Codes_SRS_INTERLOCKED_HL_42_003: [ If the value at address is equal to value, InterlockedHL_WaitForNotValue shall wait until the value at address changes in order to compare it again to value by using wait_on_address. ]*/
            /* Codes_SRS_INTERLOCKED_HL_42_004: [ When waiting for the value at address to change, the milliseconds argument value shall be used as timeout. ]*/
            if (!wait_on_address(address, current_value, milliseconds))
            {
                LogError("failure in wait_on_address(address=%p, &current_value=%p, milliseconds=%" PRIu32 ")",
                    address, &current_value, milliseconds);
                /* Codes_SRS_INTERLOCKED_HL_42_007: [ If wait_on_address fails, InterlockedHL_WaitForNotValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
                result = INTERLOCKED_HL_ERROR;
                break;
            }
        } while (1);
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_CompareExchange64If, int64_t volatile_atomic*, target, int64_t, exchange, INTERLOCKED_COMPARE_EXCHANGE_64_IF, compare, int64_t*, original_target)
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
        LogError("invalid arguments int64_t volatile_atomic* target=%p, int64_t exchange=%" PRId64 ", INTERLOCKED_COMPARE_EXCHANGE_IF compare=%p original_target=%p",
            target, exchange, compare, original_target);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ]*/
        int64_t copyOfTarget = interlocked_add_64(target, 0);

        /*Codes_SRS_INTERLOCKED_HL_02_012: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchange64If shall exchange target with exchange. ]*/
        if (compare(copyOfTarget, exchange))
        {
            /*Codes_SRS_INTERLOCKED_HL_02_013: [ If target changed meanwhile then InterlockedHL_CompareExchange64If shall return return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ]*/
            /*Codes_SRS_INTERLOCKED_HL_02_014: [ If target did not change meanwhile then InterlockedHL_CompareExchange64If shall return return INTERLOCKED_HL_OK and shall peform the exchange of values. ]*/
            if (interlocked_compare_exchange_64(target, exchange, copyOfTarget) == copyOfTarget)
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

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWake, int32_t volatile_atomic*, address, int32_t, value)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /*Codes_SRS_INTERLOCKED_HL_02_020: [ If address is NULL then InterlockedHL_SetAndWake shall fail and return INTERLOCKED_HL_ERROR. ]*/
        LogError("invalid arguments int32_t volatile_atomic* address=%p, int32_t value=%" PRId32 "",
            address, value);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_017: [ InterlockedHL_SetAndWake shall set address to value. ]*/
        (void)interlocked_exchange(address, value);

        /*Codes_SRS_INTERLOCKED_HL_02_018: [ InterlockedHL_SetAndWake shall call wake_by_address_single. ]*/
        wake_by_address_single(address);

        /*Codes_SRS_INTERLOCKED_HL_02_019: [ InterlockedHL_SetAndWake shall succeed and return INTERLOCKED_HL_OK. ]*/
        result = INTERLOCKED_HL_OK;
    }
    return result;

}

IMPLEMENT_MOCKABLE_FUNCTION(, INTERLOCKED_HL_RESULT, InterlockedHL_SetAndWakeAll, int32_t volatile_atomic*, address, int32_t, value)
{
    INTERLOCKED_HL_RESULT result;
    if (address == NULL)
    {
        /*Codes_SRS_INTERLOCKED_HL_02_028: [ If address is NULL then InterlockedHL_SetAndWakeAll shall fail and return INTERLOCKED_HL_ERROR. ]*/
        LogError("invalid arguments int32_t volatile_atomic* address=%p, int32_t value=%" PRId32 "",
            address, value);
        result = INTERLOCKED_HL_ERROR;
    }
    else
    {
        /*Codes_SRS_INTERLOCKED_HL_02_029: [ InterlockedHL_SetAndWakeAll shall set address to value. ]*/
        (void)interlocked_exchange(address, value);

        /*Codes_SRS_INTERLOCKED_HL_02_030: [ InterlockedHL_SetAndWakeAll shall call wake_by_address_all. ]*/
        wake_by_address_all(address);

        /*Codes_SRS_INTERLOCKED_HL_02_031: [ InterlockedHL_SetAndWakeAll shall succeed and return INTERLOCKED_HL_OK. ]*/
        result = INTERLOCKED_HL_OK;
    }
    return result;

}
