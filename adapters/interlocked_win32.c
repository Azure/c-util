// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <stdint.h>
#include "windows.h"
#include "azure_c_util/interlocked.h"

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile int32_t*, addend, int32_t, value) 
{
    /*Codes_SRS_INTERLOCKED_43_001 [ interlocked_add shall atomically add *addend with value and store the result in *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_032: [ interlocked_add shall return the result of the addition.]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_001: [ interlocked_add shall call InterlockedAdd from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_002: [ interlocked_add shall return the result of the addition. ]*/

    return InterlockedAdd((volatile LONG*)addend, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_add_64, volatile int64_t*, addend, int64_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_065: [ interlocked_add_64 shall atomically add 64-bit integers *addend and value and store the result in *addend. ]*/
    /*Codes_SRS_INTERLOCKED_43_066: [ interlocked_add_64 shall return the result of the addition. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_064: [ interlocked_add_64 shall call InterlockedAdd64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_065: [ interlocked_add_64 shall return the result of the addition. ]*/

    return InterlockedAdd64((volatile LONG64*)addend, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile int32_t*, destination, int32_t, value) 
{
    /*Codes_SRS_INTERLOCKED_43_002 [ interlocked_and shall perform an atomic bitwise AND operation on the 32-bit integer values *destination and value and store the result in *destination.]*/
    /*Codes_SRS_INTERLOCKED_43_033: [ interlocked_and shall return the initial value of *destination.]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_003: [ interlocked_and shall call InterlockedAnd from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_004: [ interlocked_and shall return the initial value of *destination. ]*/

    return InterlockedAnd((volatile LONG*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_003 [ interlocked_and_16 shall perform an atomic bitwise AND operation on the 16-bit integer values *destination and value and store the result in *destination.]*/
    /*Codes_SRS_INTERLOCKED_43_034: [ interlocked_and_16 shall return the initial value of *destination.]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_005: [ interlocked_and_16 shall call InterlockedAnd16 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_006: [ interlocked_and_16 shall return the initial value of *destination. ]*/

    return InterlockedAnd16((volatile SHORT*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile int64_t*, destination, int64_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_004 [ interlocked_and_64 shall perform an atomic bitwise AND operation on the 64-bit integer values *destination and value and store the result in *destination.]*/
    /*Codes_SRS_INTERLOCKED_43_035: [ interlocked_and_64 shall return the initial value of *destination.]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_007: [ interlocked_and_64 shall call InterlockedAnd64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_008: [ interlocked_and_64 shall return the initial value of *destination. ]*/

    return InterlockedAnd64((volatile LONG64*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile int8_t*, destination, int8_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_005 [ interlocked_and_8 shall perform an atomic bitwise AND operation on the 8-bit integer values *destination and value and store the result in *destination]*/
    /*Codes_SRS_INTERLOCKED_43_036: [ interlocked_and_8 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_009: [ interlocked_and_8 shall call InterlockedAnd8 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_010: [ interlocked_and_8 shall return the initial value of *destination. ]*/

    return InterlockedAnd8((volatile char*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile int32_t*, destination, int32_t, exchange, int32_t, comperand)
{
    /*Codes_SRS_INTERLOCKED_43_006 [ interlocked_compare_exchange shall compare the 32-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.*]*/
    /*Codes_SRS_INTERLOCKED_43_037: [ interlocked_compare_exchange shall return the initial value of *destination.]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_011: [ interlocked_compare_exchange shall call InterlockedCompareExchange from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_012: [ interlocked_compare_exchange shall return the initial value of *destination. ]*/

    return InterlockedCompareExchange((volatile LONG*)destination, exchange, comperand);
}

#ifdef _WIN64
IMPLEMENT_MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile int64_t*, destination, int64_t, exchange_high, int64_t, exchange_low, int64_t*, comperand_result)
{
    /*Codes_SRS_INTERLOCKED_43_007 [ interlocked_compare_exchange_128 shall compare *destination with *comperand_result. If they are equal, destination[0] is set to exchange_low and destination[1] is set to exchange_high. These operations are performed atomically.]*/
    /*Codes_SRS_INTERLOCKED_43_039: [ interlocked_compare_exchange_128 shall store the initial value of *destination in *comperand_result regardless of the result of the comparison.` ]*/
    /*Codes_SRS_INTERLOCKED_43_038: [ interlocked_compare_exchange_128 shall return true if *comperand_result equals the original value of *destination.]*/
    /*Codes_SRS_INTERLOCKED_43_063: [ interlocked_compare_exchange_128 shall return false if *comperand_result does not equal the original value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_013: [ interlocked_compare_exchange_128 shall call InterlockedCompareExchange128 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_014: [ interlocked_compare_exchange_128 shall return true if *comperand_result equals the original value of *destination.]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_063: [ interlocked_compare_exchange_128 shall return false if *comperand_result does not equal the original value of *destination. ]*/

    return InterlockedCompareExchange128((volatile LONG64*)destination, exchange_high, exchange_low, (LONG64*)comperand_result);
}
#endif

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile int16_t*, destination, int16_t, exchange, int16_t, comperand)
{
    /*Codes_SRS_INTERLOCKED_43_009 [interlocked_compare_exchange_16 shall compare the 16-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
    /*Codes_SRS_INTERLOCKED_43_040: [ interlocked_compare_exchange_16 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_015: [ interlocked_compare_exchange_16 shall call InterlockedCompareExchange16 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_016: [ interlocked_compare_exchange_16 shall return the initial value of *destination. ]*/

    return InterlockedCompareExchange16((volatile SHORT*)destination, exchange, comperand);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile int64_t*, destination, int64_t, exchange, int64_t, comperand)
{
    /*Codes_SRS_INTERLOCKED_43_008 [interlocked_compare_exchange_64 shall compare the 64-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
    /*Codes_SRS_INTERLOCKED_43_041: [ interlocked_compare_exchange_64 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_017: [ interlocked_compare_exchange_64 shall call InterlockedCompareExchange64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_018: [ interlocked_compare_exchange_64 shall return the initial value of *destination. ]*/

    return InterlockedCompareExchange64((volatile LONG64*)destination, exchange, comperand);
}

IMPLEMENT_MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, void* volatile*, destination, void*, exchange, void*, comperand)
{
    /*Codes_SRS_INTERLOCKED_43_010 [interlocked_compare_exchange_pointer shall compare the pointers destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
    /*Codes_SRS_INTERLOCKED_43_042: [ interlocked_compare_exchange_pointer shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_019: [ interlocked_compare_exchange_pointer shall call InterlockedCompareExchangePointer from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_020: [ interlocked_compare_exchange_pointer shall return the initial value of *destination. ]*/

    return InterlockedCompareExchangePointer((PVOID volatile *)destination, exchange, comperand);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile int32_t*, addend)
{
    /*Codes_SRS_INTERLOCKED_43_011[ interlocked_decrement shall atomically decrement (decrease by one) the 32-bit variable *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_043: [ interlocked_decrement shall return the decremented value. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_021: [ interlocked_decrement shall call InterlockedDecrement from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_022: [ interlocked_decrement shall return the resulting 32-bit integer value. ]*/

    return InterlockedDecrement((volatile LONG*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile int16_t*, addend)
{
    /*Codes_SRS_INTERLOCKED_43_012[ interlocked_decrement_16 shall atomically decrement (decrease by one) the 16-bit variable *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_044: [ interlocked_decrement_16 shall return the decremented value. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_023: [ interlocked_decrement_16 shall call InterlockedDecrement16 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_024: [ interlocked_decrement_16 shall return the resulting 16-bit integer value. ]*/

    return InterlockedDecrement16((volatile SHORT*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile int64_t*, addend)
{
    /*Codes_SRS_INTERLOCKED_43_013[ interlocked_decrement_64 shall atomically decrement (decrease by one) the 64-bit variable *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_045: [ interlocked_decrement_64 shall return the decremented value. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_025: [ interlocked_decrement_64 shall call InterlockedDecrement64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_026: [ interlocked_decrement_64 shall return the resulting 64-bit integer value. ]*/

    return InterlockedDecrement64((volatile LONG64*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile int32_t*, target, int32_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_014 [ interlocked_exchange shall set the 32-bit variable pointed to by target to value as an atomic operation.]*/
    /*Codes_SRS_INTERLOCKED_43_046: [ interlocked_exchange shall return the initial value pointed to by target. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_027: [ interlocked_exchange shall call InterlockedExchange from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_028: [ interlocked_exchange shall return the initial value pointed to by target. ]*/

    return InterlockedExchange((volatile LONG*)target, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile int16_t*, target, int16_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_015 [ interlocked_exchange_16 shall set the 16-bit variable pointed to by target to value as an atomic operation.]*/
    /*Codes_SRS_INTERLOCKED_43_047: [ interlocked_exchange_16 shall return the initial value pointed to by target. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_029: [ interlocked_exchange_16 shall call InterlockedExchange16 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_030: [ interlocked_exchange_16 shall return the initial value pointed to by target. ]*/

    return InterlockedExchange16((volatile SHORT*)target, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile int64_t*, target, int64_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_016 [ interlocked_exchange_64 shall set the 64-bit variable pointed to by target to value as an atomic operation.]*/
    /*Codes_SRS_INTERLOCKED_43_048: [ interlocked_exchange_64 shall return the initial value pointed to by target. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_031: [ interlocked_exchange_64 shall call InterlockedExchange64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_032: [ interlocked_exchange_64 shall return the initial value pointed to by target. ]*/

    return InterlockedExchange64((volatile LONG64*)target, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile int8_t*, target, int8_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_017 [ interlocked_exchange_8 shall set the 8-bit variable pointed to by target to value as an atomic operation.]*/
    /*Codes_SRS_INTERLOCKED_43_049: [ interlocked_exchange_8 shall return the initial value pointed to by target. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_033: [ interlocked_exchange_8 shall call InterlockedExchange8 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_034: [ interlocked_exchange_8 shall return the initial value pointed to by target. ]*/

    return InterlockedExchange8((volatile CHAR*)target, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile int32_t*, addend, int32_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_018 [ interlocked_exchange_add shall perform an atomic addition of the 32-bit values *addend and value and store the result in *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_050: [ interlocked_exchange_add shall return the initial value of *addend. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_035: [ interlocked_exchange_add shall call InterlockedExchangeAdd from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_036: [ interlocked_exchange_add shall return the initial value of *addend. ]*/

    return InterlockedExchangeAdd((volatile LONG*)addend, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile int64_t*, addend, int64_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_019 [ interlocked_exchange_add_64 shall perform an atomic addition of the 64-bit values *addend and value and store the result in *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_064: [ interlocked_exchange_add_64 shall return the initial value of *addend. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_037: [ interlocked_exchange_add_64 shall call InterlockedExchangeAdd64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_038: [ interlocked_exchange_add_64 shall return the initial value of *addend. ]*/

    return InterlockedExchangeAdd64((volatile LONG64*)addend, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, void* volatile*, target, void*, value)
{
    /*Codes_SRS_INTERLOCKED_43_020 [ interlocked_exchange_pointer shall atomically set *target to value]*/
    /*Codes_SRS_INTERLOCKED_43_051: [ interlocked_exchange_pointer shall return the initial value of *target. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_039: [ interlocked_exchange_pointer shall call InterlockedExchangePointer from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_040: [interlocked_exchange_pointer shall return the initial address pointed to by the target parameter ]*/

    return InterlockedExchangePointer((PVOID volatile*)target, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile int32_t*, addend)
{
    /*Codes_SRS_INTERLOCKED_43_021 [ interlocked_increment shall atomically increment (increase by one) the 32-bit variable *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_052: [ interlocked_increment shall return the incremented value. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_041: [ interlocked_increment shall call InterlockedIncrement from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_042: [ interlocked_increment shall return the incremented 32-bit integer. ]*/

    return InterlockedIncrement((volatile LONG*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile int16_t*, addend)
{
    /*Codes_SRS_INTERLOCKED_43_022 [ interlocked_increment_16 shall atomically increment (increase by one) the 16-bit variable *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_053: [ interlocked_increment_16 shall return the incremented value. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_043: [ interlocked_increment_16 shall call InterlockedIncrement16 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_044: [ interlocked_increment_16 shall return the incremented 16-bit integer. ]*/

    return InterlockedIncrement16((volatile SHORT*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile int64_t*, addend)
{
    /*Codes_SRS_INTERLOCKED_43_023 [ interlocked_increment_64 shall atomically increment (increase by one) the 64-bit variable *addend.]*/
    /*Codes_SRS_INTERLOCKED_43_054: [ interlocked_increment_64 shall return the incremented value. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_045: [ interlocked_increment_64 shall call InterlockedIncrement64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_046: [ interlocked_increment_64 shall return the incremented 64-bit integer. ]*/

    return InterlockedIncrement64((volatile LONG64*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile int32_t*, destination, int32_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_024 [ interlocked_or shall perform an atomic bitwise OR operation on the 32-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_055: [ interlocked_or shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_047: [ interlocked_or shall call InterlockedOr from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_048: [ interlocked_or shall return the initial value of *destination. ]*/

    return InterlockedOr((volatile LONG*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile int16_t*, destination, int16_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_025 [ interlocked_or_16 shall perform an atomic bitwise OR operation on the 16-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_056: [ interlocked_or_16 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_049: [ interlocked_or_16 shall call InterlockedOr16 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_050: [ interlocked_or_16 shall return the initial value of *destination. ]*/

    return InterlockedOr16((volatile SHORT*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile int64_t*, destination, int64_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_026 [ interlocked_or_64 shall perform an atomic bitwise OR operation on the 64-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_057: [ interlocked_or_64 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_051: [ interlocked_or_64 shall call InterlockedOr64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_052: [ interlocked_or_64 shall return the initial value of *destination. ]*/

    return InterlockedOr64((volatile LONG64*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile int8_t*, destination, int8_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_027 [ interlocked_or_8 shall perform an atomic bitwise OR operation on the 8-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_058: [ interlocked_or_8 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_053: [ interlocked_or_8 shall call InterlockedOr8 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_054: [ interlocked_or_8 shall return the initial value of *destination. ]*/

    return InterlockedOr8((volatile char*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile int32_t*, destination, int32_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_028 [ interlocked_xor shall perform an atomic bitwise XOR operation on the 32-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_059: [ interlocked_xor shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_055: [ interlocked_xor shall call InterlockedXor from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_056: [ interlocked_xor shall return the initial value of *destination. ]*/

    return InterlockedXor((volatile LONG*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile int16_t*, destination, int16_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_029 [ interlocked_xor_16 shall perform an atomic bitwise XOR operation on the 16-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_060: [ interlocked_xor_16 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_057: [ interlocked_xor_16 shall call InterlockedXor16 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_058: [ interlocked_xor_16 shall return the initial value of *destination. ]*/

    return InterlockedXor16((volatile SHORT*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile int64_t*, destination, int64_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_030 [ interlocked_xor_64 shall perform an atomic bitwise XOR operation on the 64-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_061: [ interlocked_xor_64 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_059: [ interlocked_xor_64 shall call InterlockedXor64 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_060: [ interlocked_xor_64 shall return the initial value of *destination. ]*/

    return InterlockedXor64((volatile LONG64*)destination, value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile int8_t*, destination, int8_t, value)
{
    /*Codes_SRS_INTERLOCKED_43_031 [ interlocked_xor_8 shall perform an atomic bitwise XOR operation on the 8-bit integers *destination and value and store the result in destination.]*/
    /*Codes_SRS_INTERLOCKED_43_062: [ interlocked_xor_8 shall return the initial value of *destination. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_061: [ interlocked_xor_8 shall call InterlockedXor8 from windows.h. ]*/
    /*Codes_SRS_INTERLOCKED_WIN32_43_062: [ interlocked_xor_8 shall return the initial value of *destination. ]*/

    return InterlockedXor8((volatile char*)destination, value);
}
