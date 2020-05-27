// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "umock_c/umock_c_prod.h"


#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, LONG, mock_InterlockedAdd, volatile LONG*, addend, LONG, value);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedAdd64, volatile LONG64*, addend, LONG64, value);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedAnd, volatile LONG*, destination, LONG, value);
MOCKABLE_FUNCTION(, SHORT, mock_InterlockedAnd16, volatile SHORT*, destination, SHORT, value);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedAnd64, volatile LONG64*, destination, LONG64, value);
MOCKABLE_FUNCTION(, char, mock_InterlockedAnd8, volatile char*, destination, char, value);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedCompareExchange, volatile LONG*, destination, LONG, exchange, LONG, comperand);
#ifdef _WIN64
MOCKABLE_FUNCTION(, BOOLEAN, mock_InterlockedCompareExchange128, volatile LONG64*, destination, LONG64, exchange_high, LONG64, exchange_low, LONG64*, comperand_result);
#endif
MOCKABLE_FUNCTION(, SHORT, mock_InterlockedCompareExchange16, volatile SHORT*, destination, SHORT, exchange, SHORT, comperand);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedCompareExchange64, volatile LONG64*, destination, LONG64, exchange, LONG64, comperand);
MOCKABLE_FUNCTION(, void*, mock_InterlockedCompareExchangePointer, void* volatile*, destination, void*, exchange, void*, comperand);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedDecrement, volatile LONG*, addend);
MOCKABLE_FUNCTION(, SHORT, mock_InterlockedDecrement16, volatile SHORT*, addend);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedDecrement64, volatile LONG64*, addend);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedExchange, volatile LONG*, target, LONG, value);
MOCKABLE_FUNCTION(, SHORT, mock_InterlockedExchange16, volatile SHORT*, target, SHORT, value);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedExchange64, volatile LONG64*, target, LONG64, value);
MOCKABLE_FUNCTION(, char, mock_InterlockedExchange8, volatile char*, target, char, value);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedExchangeAdd, volatile LONG*, addend, LONG, value);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedExchangeAdd64, volatile LONG64*, addend, LONG64, value);
MOCKABLE_FUNCTION(, void*, mock_InterlockedExchangePointer, void* volatile*, target, void*, value);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedIncrement, volatile LONG*, addend);
MOCKABLE_FUNCTION(, SHORT, mock_InterlockedIncrement16, volatile SHORT*, addend);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedIncrement64, volatile LONG64*, addend);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedOr, volatile LONG*, destination, LONG, value);
MOCKABLE_FUNCTION(, SHORT, mock_InterlockedOr16, volatile SHORT*, destination, SHORT, value);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedOr64, volatile LONG64*, destination, LONG64, value);
MOCKABLE_FUNCTION(, char, mock_InterlockedOr8, volatile char*, destination, char, value);
MOCKABLE_FUNCTION(, LONG, mock_InterlockedXor, volatile LONG*, destination, LONG, value);
MOCKABLE_FUNCTION(, SHORT, mock_InterlockedXor16, volatile SHORT*, destination, SHORT, value);
MOCKABLE_FUNCTION(, LONG64, mock_InterlockedXor64, volatile LONG64*, destination, LONG64, value);
MOCKABLE_FUNCTION(, char, mock_InterlockedXor8, volatile char*, destination, char, value);

#ifdef __cplusplus
}
#endif
