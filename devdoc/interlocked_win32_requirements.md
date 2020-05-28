interlocked Win32
================

## Overview

`interlocked Win32` is the Windows-specific implementation of the `interlocked` module. Each function calls the corresponding [`Interlocked` function from `windows.h`](https://docs.microsoft.com/en-us/windows/win32/sync/interlocked-variable-access). The `interlocked` module passes the arguments to the Windows Interlocked API in the same order it receives them.


## Exposed API

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile_atomic int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_add_64, volatile_atomic int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile_atomic int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile_atomic int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile_atomic int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile_atomic int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile_atomic int32_t*, destination, int32_t, exchange, int32_t, comperand);
MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile_atomic int64_t*, destination, int64_t, exchange_high, int64_t, exchange_low, int64_t*, comperand_result);
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile_atomic int16_t*, destination, int16_t, exchange, int16_t, comperand);
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile_atomic int64_t*, destination, int64_t, exchange, int64_t, comperand);
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, void* volatile_atomic*, destination, void*, exchange, void*, comperand);
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile_atomic int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile_atomic int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile_atomic int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile_atomic int32_t*, target, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile_atomic int16_t*, target, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile_atomic int64_t*, target, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile_atomic int8_t*, target, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile_atomic int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile_atomic int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, void* volatile_atomic*, target, void*, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile_atomic int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile_atomic int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile_atomic int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile_atomic int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile_atomic int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile_atomic int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile_atomic int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile_atomic int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile_atomic int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile_atomic int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile_atomic int8_t*, destination, int8_t, value);
```

## interlocked_add

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile_atomic int32_t*, addend, int32_t, value);
```
**SRS_INTERLOCKED_WIN32_43_001: [** `interlocked_add` shall call `InterlockedAdd` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_002: [** `interlocked_add` shall return the result of the addition. **]**

## interlocked_add_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_add_64, volatile_atomic int64_t*, addend, int64_t, value);
```
**SRS_INTERLOCKED_WIN32_43_064: [** `interlocked_add_64` shall call `InterlockedAdd64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_065: [** `interlocked_add_64` shall return the result of the addition. **]**

## interlocked_and

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile_atomic int32_t*, destination, int32_t, value);
```
**SRS_INTERLOCKED_WIN32_43_003: [** `interlocked_and` shall call `InterlockedAnd` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_004: [** `interlocked_and` shall return the initial value of `*destination`. **]**

## interlocked_and_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile_atomic int16_t*, destination, int16_t, value);
```
**SRS_INTERLOCKED_WIN32_43_005: [** `interlocked_and_16` shall call `InterlockedAnd16` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_006: [** `interlocked_and_16` shall return the initial value of `*destination`. **]**
 
 ## interlocked_and_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile_atomic int64_t*, destination, int64_t, value);
```
**SRS_INTERLOCKED_WIN32_43_007: [** `interlocked_and_64` shall call `InterlockedAnd64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_008: [** `interlocked_and_64` shall return the initial value of `*destination`. **]**

## interlocked_and_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile_atomic int8_t*, destination, int8_t, value);
```
**SRS_INTERLOCKED_WIN32_43_009: [** `interlocked_and_8` shall call `InterlockedAnd8` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_010: [** `interlocked_and_8` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile_atomic int32_t*, destination, int32_t, exchange, int32_t, comperand);
```
**SRS_INTERLOCKED_WIN32_43_011: [** `interlocked_compare_exchange` shall call `InterlockedCompareExchange` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_012: [** `interlocked_compare_exchange` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_128

```c
MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile_atomic int64_t*, destination, int64_t, exchange_high, int64_t, exchange_low, int64_t*, comperand_result);
```
**SRS_INTERLOCKED_WIN32_43_013: [** `interlocked_compare_exchange_128` shall call `InterlockedCompareExchange128` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_014: [** `interlocked_compare_exchange_128` shall return `true` if `*comperand_result` equals the original value of `*destination`.**]**

**SRS_INTERLOCKED_WIN32_43_063: [** `interlocked_compare_exchange_128` shall return `false` if `*comperand_result` does not equal the original value of `*destination`. **]**

## interlocked_compare_exchange_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile_atomic int16_t*, destination, int16_t, exchange, int16_t, comperand);
```
**SRS_INTERLOCKED_WIN32_43_015: [** `interlocked_compare_exchange_16` shall call `InterlockedCompareExchange16` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_016: [** `interlocked_compare_exchange_16` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile_atomic int64_t*, destination, int64_t, exchange, int64_t, comperand);

```
**SRS_INTERLOCKED_WIN32_43_017: [** `interlocked_compare_exchange_64` shall call `InterlockedCompareExchange64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_018: [** `interlocked_compare_exchange_64` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, void* volatile_atomic*, destination, void*, exchange, void*, comperand);

```
**SRS_INTERLOCKED_WIN32_43_019: [** `interlocked_compare_exchange_pointer` shall call `InterlockedCompareExchangePointer` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_020: [** `interlocked_compare_exchange_pointer` shall return the initial value of `*destination`. **]**

## interlocked_decrement

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile_atomic int32_t*, addend);

```
**SRS_INTERLOCKED_WIN32_43_021: [** `interlocked_decrement` shall call `InterlockedDecrement` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_022: [** `interlocked_decrement` shall return the resulting 32-bit integer value. **]**

## interlocked_decrement_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile_atomic int16_t*, addend);
```
**SRS_INTERLOCKED_WIN32_43_023: [** `interlocked_decrement_16` shall call `InterlockedDecrement16` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_024: [** `interlocked_decrement_16` shall return the resulting 16-bit integer value. **]**

## interlocked_decrement_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile_atomic int64_t*, addend);

```
**SRS_INTERLOCKED_WIN32_43_025: [** `interlocked_decrement_64` shall call `InterlockedDecrement64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_026: [** `interlocked_decrement_64` shall return the resulting 64-bit integer value. **]**

## interlocked_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile_atomic int32_t*, target, int32_t, value);

```
**SRS_INTERLOCKED_WIN32_43_027: [** `interlocked_exchange` shall call `InterlockedExchange` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_028: [** `interlocked_exchange` shall return the initial value pointed to by `target`. **]**


## interlocked_exchange_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile_atomic int16_t*, target, int16_t, value);

```
**SRS_INTERLOCKED_WIN32_43_029: [** `interlocked_exchange_16` shall call `InterlockedExchange16` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_030: [** `interlocked_exchange_16` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile_atomic int64_t*, target, int64_t, value);

```
**SRS_INTERLOCKED_WIN32_43_031: [** `interlocked_exchange_64` shall call `InterlockedExchange64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_032: [** `interlocked_exchange_64` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile_atomic int8_t*, target, int8_t, value);

```
**SRS_INTERLOCKED_WIN32_43_033: [** `interlocked_exchange_8` shall call `InterlockedExchange8` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_034: [** `interlocked_exchange_8` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_add

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile_atomic int32_t*, addend, int32_t, value);

```
**SRS_INTERLOCKED_WIN32_43_035: [** `interlocked_exchange_add` shall call `InterlockedExchangeAdd` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_036: [** `interlocked_exchange_add` shall return the initial value of `*addend`. **]**

## interlocked_exchange_add_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile_atomic int64_t*, addend, int64_t, value);

```
**SRS_INTERLOCKED_WIN32_43_037: [** `interlocked_exchange_add_64` shall call `InterlockedExchangeAdd64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_038: [** `interlocked_exchange_add_64` shall return the initial value of `*addend`. **]**

## interlocked_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, void* volatile_atomic*, target, void*, value);

```
**SRS_INTERLOCKED_WIN32_43_039: [** `interlocked_exchange_pointer` shall call `InterlockedExchangePointer` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_040: [**`interlocked_exchange_pointer` shall return the initial address pointed to by the `target` parameter **]**

## interlocked_increment

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile_atomic int32_t*, addend);

```
**SRS_INTERLOCKED_WIN32_43_041: [** `interlocked_increment` shall call `InterlockedIncrement` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_042: [** `interlocked_increment` shall return the incremented 32-bit integer. **]**

## interlocked_increment_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile_atomic int16_t*, addend);

```
**SRS_INTERLOCKED_WIN32_43_043: [** `interlocked_increment_16` shall call `InterlockedIncrement16` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_044: [** `interlocked_increment_16` shall return the incremented 16-bit integer. **]**

## interlocked_increment_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile_atomic int64_t*, addend);

```
**SRS_INTERLOCKED_WIN32_43_045: [** `interlocked_increment_64` shall call `InterlockedIncrement64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_046: [** `interlocked_increment_64` shall return the incremented 64-bit integer. **]**

## interlocked_or

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile_atomic int32_t*, destination, int32_t, value);

```
**SRS_INTERLOCKED_WIN32_43_047: [** `interlocked_or` shall call `InterlockedOr` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_048: [** `interlocked_or` shall return the initial value of `*destination`. **]**

## interlocked_or_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile_atomic int16_t*, destination, int16_t, value);

```
**SRS_INTERLOCKED_WIN32_43_049: [** `interlocked_or_16` shall call `InterlockedOr16` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_050: [** `interlocked_or_16` shall return the initial value of `*destination`. **]**

## interlocked_or_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile_atomic int64_t*, destination, int64_t, value);

```
**SRS_INTERLOCKED_WIN32_43_051: [** `interlocked_or_64` shall call `InterlockedOr64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_052: [** `interlocked_or_64` shall return the initial value of `*destination`. **]**

## interlocked_or_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile_atomic int8_t*, destination, int8_t, value);

```
**SRS_INTERLOCKED_WIN32_43_053: [** `interlocked_or_8` shall call `InterlockedOr8` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_054: [** `interlocked_or_8` shall return the initial value of `*destination`. **]**

## interlocked_xor

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile_atomic int32_t*, destination, int32_t, value);

```
**SRS_INTERLOCKED_WIN32_43_055: [** `interlocked_xor` shall call `InterlockedXor` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_056: [** `interlocked_xor` shall return the initial value of `*destination`. **]**

## interlocked_xor_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile_atomic int16_t*, destination, int16_t, value);

```
**SRS_INTERLOCKED_WIN32_43_057: [** `interlocked_xor_16` shall call `InterlockedXor16` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_058: [** `interlocked_xor_16` shall return the initial value of `*destination`. **]**

## interlocked_xor_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile_atomic int64_t*, destination, int64_t, value);

```
**SRS_INTERLOCKED_WIN32_43_059: [** `interlocked_xor_64` shall call `InterlockedXor64` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_060: [** `interlocked_xor_64` shall return the initial value of `*destination`. **]**

## interlocked_xor_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile_atomic int8_t*, destination, int8_t, value);

```
**SRS_INTERLOCKED_WIN32_43_061: [** `interlocked_xor_8` shall call `InterlockedXor8` from `windows.h`. **]**

**SRS_INTERLOCKED_WIN32_43_062: [** `interlocked_xor_8` shall return the initial value of `*destination`. **]**