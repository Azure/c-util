// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "windows.h"
#include "mock_interlocked.h"

#undef InterlockedAdd 
#define InterlockedAdd mock_InterlockedAdd
#undef InterlockedAdd64 
#define InterlockedAdd64 mock_InterlockedAdd64
#undef InterlockedAnd 
#define InterlockedAnd mock_InterlockedAnd
#undef InterlockedAnd16 
#define InterlockedAnd16 mock_InterlockedAnd16
#undef InterlockedAnd64 
#define InterlockedAnd64 mock_InterlockedAnd64
#undef InterlockedAnd8 
#define InterlockedAnd8 mock_InterlockedAnd8
#undef InterlockedCompareExchange 
#define InterlockedCompareExchange mock_InterlockedCompareExchange
#undef InterlockedCompareExchange128 
#define InterlockedCompareExchange128 mock_InterlockedCompareExchange128
#undef InterlockedCompareExchange16 
#define InterlockedCompareExchange16 mock_InterlockedCompareExchange16
#undef InterlockedCompareExchange64
#define InterlockedCompareExchange64 mock_InterlockedCompareExchange64
#undef InterlockedCompareExchangePointer 
#define InterlockedCompareExchangePointer mock_InterlockedCompareExchangePointer
#undef InterlockedDecrement 
#define InterlockedDecrement mock_InterlockedDecrement
#undef InterlockedDecrement16 
#define InterlockedDecrement16 mock_InterlockedDecrement16
#undef InterlockedDecrement64 
#define InterlockedDecrement64 mock_InterlockedDecrement64
#undef InterlockedExchange 
#define InterlockedExchange mock_InterlockedExchange
#undef InterlockedExchange16 
#define InterlockedExchange16 mock_InterlockedExchange16
#undef InterlockedExchange64 
#define InterlockedExchange64 mock_InterlockedExchange64
#undef InterlockedExchange8 
#define InterlockedExchange8 mock_InterlockedExchange8
#undef InterlockedExchangeAdd 
#define InterlockedExchangeAdd mock_InterlockedExchangeAdd
#undef InterlockedExchangeAdd64 
#define InterlockedExchangeAdd64 mock_InterlockedExchangeAdd64
#undef InterlockedExchangePointer 
#define InterlockedExchangePointer mock_InterlockedExchangePointer
#undef InterlockedIncrement 
#define InterlockedIncrement mock_InterlockedIncrement
#undef InterlockedIncrement16
#define InterlockedIncrement16 mock_InterlockedIncrement16
#undef InterlockedIncrement64 
#define InterlockedIncrement64 mock_InterlockedIncrement64
#undef InterlockedOr 
#define InterlockedOr mock_InterlockedOr
#undef InterlockedOr16 
#define InterlockedOr16 mock_InterlockedOr16
#undef InterlockedOr64 
#define InterlockedOr64 mock_InterlockedOr64
#undef InterlockedOr8 
#define InterlockedOr8 mock_InterlockedOr8
#undef InterlockedXor 
#define InterlockedXor mock_InterlockedXor
#undef InterlockedXor16
#define InterlockedXor16 mock_InterlockedXor16
#undef InterlockedXor64 
#define InterlockedXor64 mock_InterlockedXor64
#undef InterlockedXor8 
#define InterlockedXor8 mock_InterlockedXor8

#include "../../adapters/interlocked_win32.c"
