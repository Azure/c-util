// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_util/interlocked.h"
#include "windows.h"

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile int32_t*, addend, int32_t, value) 
{
	return (int32_t)InterlockedAdd((volatile LONG*)addend, (LONG)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile int32_t*, destination, int32_t, value) 
{
	return (int32_t)InterlockedAnd((volatile LONG*)destination, (LONG)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value)
{
	return (int16_t)InterlockedAnd16((volatile SHORT*)destination, (SHORT)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile int64_t*, destination, int64_t, value)
{
	return (int64_t)InterlockedAnd64((volatile LONG64*)destination, (LONG64)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile int8_t*, destination, int8_t, value)
{
	return (int8_t)InterlockedAnd8((volatile char*)destination, (char)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile int32_t*, destination, int32_t, exchange, int32_t, comperand)
{
	return (int32_t)InterlockedCompareExchange((volatile LONG*)destination, (LONG)exchange, (LONG)comperand);
}

IMPLEMENT_MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile int64_t*, destination, int64_t, exchange_high, int64_t, exchange_low, int64_t*, comperand_result)
{
	return (bool)InterlockedCompareExchange128((volatile LONG64*)destination, (LONG64)exchange_high, (LONG64)exchange_low, (LONG64*)comperand_result);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile int16_t*, destination, int16_t, exchange, int16_t, comperand)
{
	return (int16_t)InterlockedCompareExchange16((volatile SHORT*)destination, (SHORT)exchange, (SHORT)comperand);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile int64_t*, destination, int64_t, exchange, int64_t, comperand)
{
	return (int64_t)InterlockedCompareExchange64((volatile LONG64*)destination, (LONG64)exchange, (LONG64)comperand);
}

IMPLEMENT_MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, volatile void**, destination, void*, exchange, void*, comperand)
{
	return (void*)InterlockedCompareExchange64((volatile PVOID*)destination, (PVOID)exchange, (PVOID)comperand);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile int32_t*, addend)
{
	return (int32_t)InterlockedDecrement((volatile LONG*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile int16_t*, addend)
{
	return (int16_t)InterlockedDecrement16((volatile SHORT*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile int64_t*, addend)
{
	return (int64_t)InterlockedDecrement64((volatile LONG64*)addend);
}
IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile int32_t*, target, int32_t, value)
{
	return (int32_t)InterlockedExchange((volatile LONG*)target, (LONG)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile int16_t*, target, int16_t, value)
{
	return (int16_t)InterlockedExchange16((volatile SHORT*)target, (SHORT)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile int64_t*, target, int64_t, value)
{
	return (int64_t)InterlockedExchange64((volatile LONG64*)target, (LONG64)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile int8_t*, target, int8_t, value)
{
	return (int8_t)InterlockedExchange8((volatile char*)target, (char)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile int32_t*, addend, int32_t, value)
{
	return (int32_t)InterlockedExchangeAdd((volatile LONG*)addend, (LONG)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile int64_t*, addend, int64_t, value)
{
	return (int64_t)InterlockedExchangeAdd64((volatile LONG64*)addend, (LONG64)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, volatile void**, target, void*, value)
{
	return (void*)InterlockedExchangeAdd((volatile PVOID*)target, (PVOID)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile int32_t*, addend)
{
	return (int32_t)InterlockedIncrement((volatile LONG*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile int16_t*, addend)
{
	return (int16_t)InterlockedIncrement16((volatile SHORT*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile int64_t*, addend)
{
	return (int64_t)InterlockedIncrement64((volatile LONG64*)addend);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile int32_t*, destination, int32_t, value)
{
	return (int32_t)InterlockedOr((volatile LONG*)destination, (LONG)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile int16_t*, destination, int16_t, value)
{
	return (int16_t)InterlockedOr16((volatile SHORT*)destination, (SHORT)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile int64_t*, destination, int64_t, value)
{
	return (int64_t)InterlockedOr64((volatile LONG64*)destination, (LONG64)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile int8_t*, destination, int8_t, value)
{
	return (int8_t)InterlockedOr8((volatile char*)destination, (char)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile int32_t*, destination, int32_t, value)
{
	return (int32_t)InterlockedXor((volatile LONG*)destination, (LONG)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile int16_t*, destination, int16_t, value)
{
	return (int16_t)InterlockedXor16((volatile SHORT*)destination, (SHORT)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile int64_t*, destination, int64_t, value)
{
	return (int64_t)InterlockedXor64((volatile LONG64*)destination, (LONG64)value);
}

IMPLEMENT_MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile int8_t*, destination, int8_t, value)
{
	return (int8_t)InterlockedXor8((volatile char*)destination, (char)value);
}
