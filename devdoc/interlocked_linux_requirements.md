interlocked linux
================

## Overview

`interlocked linux` is the Linux-specific implementation of the `interlocked` module. Each function calls the corresponding `atomic` function from `stdatomic.h`. The default memory order for `stdatomic.h` functions is `memory_order_seq_cst` which uses a full memory barrier for each operation.


## Exposed API

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_add_64, volatile int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile int32_t*, destination, int32_t, exchange, int32_t, comperand);
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile int16_t*, destination, int16_t, exchange, int16_t, comperand);
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile int64_t*, destination, int64_t, exchange, int64_t, comperand);
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, void* volatile*, destination, void*, exchange, void*, comperand);
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile int32_t*, target, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile int16_t*, target, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile int64_t*, target, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile int8_t*, target, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, void* volatile*, target, void*, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile int8_t*, destination, int8_t, value);
```

## interlocked_add

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile int32_t*, addend, int32_t, value);
```
**SRS_INTERLOCKED_LINUX_43_001: [** `interlocked_add` shall call `atomic_fetch_add` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_002: [** `interlocked_add` shall return the result of the addition. **]**

## interlocked_add_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_add_64, volatile int64_t*, addend, int64_t, value);
```
**SRS_INTERLOCKED_LINUX_43_064: [** `interlocked_add_64` shall call `atomic_fetch_add` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_065: [** `interlocked_add_64` shall return the result of the addition. **]**

## interlocked_and

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile int32_t*, destination, int32_t, value);
```
**SRS_INTERLOCKED_LINUX_43_003: [** `interlocked_and` shall call `atomic_fetch_and` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_004: [** `interlocked_and` shall return the initial value of `*destination`. **]**

## interlocked_and_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value);
```
**SRS_INTERLOCKED_LINUX_43_005: [** `interlocked_and_16` shall call `atomic_fetch_and` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_006: [** `interlocked_and_16` shall return the initial value of `*destination`. **]**
 
 ## interlocked_and_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile int64_t*, destination, int64_t, value);
```
**SRS_INTERLOCKED_LINUX_43_007: [** `interlocked_and_64` shall call `atomic_fetch_and` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_008: [** `interlocked_and_64` shall return the initial value of `*destination`. **]**

## interlocked_and_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile int8_t*, destination, int8_t, value);
```
**SRS_INTERLOCKED_LINUX_43_009: [** `interlocked_and_8` shall call `atomic_fetch_and` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_010: [** `interlocked_and_8` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile int32_t*, destination, int32_t, exchange, int32_t, comperand);
```
**SRS_INTERLOCKED_LINUX_43_011: [** `interlocked_compare_exchange` shall call `atomic_compare_exchange_strong` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_012: [** `interlocked_compare_exchange` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile int16_t*, destination, int16_t, exchange, int16_t, comperand);
```
**SRS_INTERLOCKED_LINUX_43_015: [** `interlocked_compare_exchange_16` shall call `atomic_compare_exchange_strong` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_016: [** `interlocked_compare_exchange_16` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile int64_t*, destination, int64_t, exchange, int64_t, comperand);

```
**SRS_INTERLOCKED_LINUX_43_017: [** `interlocked_compare_exchange_64` shall call `atomic_compare_exchange_strong` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_018: [** `interlocked_compare_exchange_64` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, void* volatile*, destination, void*, exchange, void*, comperand);

```
**SRS_INTERLOCKED_LINUX_43_019: [** `interlocked_compare_exchange_pointer` shall call `atomic_compare_exchange_strong` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_020: [** `interlocked_compare_exchange_pointer` shall return the initial value of `*destination`. **]**

## interlocked_decrement

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile int32_t*, addend);

```
**SRS_INTERLOCKED_LINUX_43_021: [** `interlocked_decrement` shall call `atomic_fetch_sub` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_022: [** `interlocked_decrement` shall return the resulting 32-bit integer value. **]**

## interlocked_decrement_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile int16_t*, addend);
```
**SRS_INTERLOCKED_LINUX_43_023: [** `interlocked_decrement_16` shall call `atomic_fetch_sub` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_024: [** `interlocked_decrement_16` shall return the resulting 16-bit integer value. **]**

## interlocked_decrement_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile int64_t*, addend);

```
**SRS_INTERLOCKED_LINUX_43_025: [** `interlocked_decrement_64` shall call `atomic_fetch_sub` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_026: [** `interlocked_decrement_64` shall return the resulting 64-bit integer value. **]**

## interlocked_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile int32_t*, target, int32_t, value);

```
**SRS_INTERLOCKED_LINUX_43_027: [** `interlocked_exchange` shall call `atomic_exchange` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_028: [** `interlocked_exchange` shall return the initial value pointed to by `target`. **]**


## interlocked_exchange_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile int16_t*, target, int16_t, value);

```
**SRS_INTERLOCKED_LINUX_43_029: [** `interlocked_exchange_16` shall call `atomic_exchange` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_030: [** `interlocked_exchange_16` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile int64_t*, target, int64_t, value);

```
**SRS_INTERLOCKED_LINUX_43_031: [** `interlocked_exchange_64` shall call `atomic_exchange` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_032: [** `interlocked_exchange_64` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile int8_t*, target, int8_t, value);

```
**SRS_INTERLOCKED_LINUX_43_033: [** `interlocked_exchange_8` shall call `atomic_exchange` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_034: [** `interlocked_exchange_8` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_add

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile int32_t*, addend, int32_t, value);

```
**SRS_INTERLOCKED_LINUX_43_035: [** `interlocked_exchange_add` shall call `atomic_fetch_add` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_036: [** `interlocked_exchange_add` shall return the initial value of `*addend`. **]**

## interlocked_exchange_add_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile int64_t*, addend, int64_t, value);

```
**SRS_INTERLOCKED_LINUX_43_037: [** `interlocked_exchange_add_64` shall call `atomic_fetch_add` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_038: [** `interlocked_exchange_add_64` shall return the initial value of `*addend`. **]**

## interlocked_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, void* volatile*, target, void*, value);

```
**SRS_INTERLOCKED_LINUX_43_039: [** `interlocked_exchange_pointer` shall call `atomic_exchange` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_040: [**`interlocked_exchange_pointer` shall return the initial address pointed to by the `target` parameter **]**

## interlocked_increment

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile int32_t*, addend);

```
**SRS_INTERLOCKED_LINUX_43_041: [** `interlocked_increment` shall call `atomic_fetch_add` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_042: [** `interlocked_increment` shall return the incremented 32-bit integer. **]**

## interlocked_increment_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile int16_t*, addend);

```
**SRS_INTERLOCKED_LINUX_43_043: [** `interlocked_increment_16` shall call `atomic_fetch_add` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_044: [** `interlocked_increment_16` shall return the incremented 16-bit integer. **]**

## interlocked_increment_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile int64_t*, addend);

```
**SRS_INTERLOCKED_LINUX_43_045: [** `interlocked_increment_64` shall call `atomic_fetch_add` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_046: [** `interlocked_increment_64` shall return the incremented 64-bit integer. **]**

## interlocked_or

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile int32_t*, destination, int32_t, value);

```
**SRS_INTERLOCKED_LINUX_43_047: [** `interlocked_or` shall call `atomic_fetch_or` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_048: [** `interlocked_or` shall return the initial value of `*destination`. **]**

## interlocked_or_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile int16_t*, destination, int16_t, value);

```
**SRS_INTERLOCKED_LINUX_43_049: [** `interlocked_or_16` shall call `atomic_fetch_or` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_050: [** `interlocked_or_16` shall return the initial value of `*destination`. **]**

## interlocked_or_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile int64_t*, destination, int64_t, value);

```
**SRS_INTERLOCKED_LINUX_43_051: [** `interlocked_or_64` shall call `atomic_fetch_or` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_052: [** `interlocked_or_64` shall return the initial value of `*destination`. **]**

## interlocked_or_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile int8_t*, destination, int8_t, value);

```
**SRS_INTERLOCKED_LINUX_43_053: [** `interlocked_or_8` shall call `atomic_fetch_or` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_054: [** `interlocked_or_8` shall return the initial value of `*destination`. **]**

## interlocked_xor

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile int32_t*, destination, int32_t, value);

```
**SRS_INTERLOCKED_LINUX_43_055: [** `interlocked_xor` shall call `atomic_fetch_xor` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_056: [** `interlocked_xor` shall return the initial value of `*destination`. **]**

## interlocked_xor_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile int16_t*, destination, int16_t, value);

```
**SRS_INTERLOCKED_LINUX_43_057: [** `interlocked_xor_16` shall call `atomic_fetch_xor` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_058: [** `interlocked_xor_16` shall return the initial value of `*destination`. **]**

## interlocked_xor_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile int64_t*, destination, int64_t, value);

```
**SRS_INTERLOCKED_LINUX_43_059: [** `interlocked_xor_64` shall call `atomic_fetch_xor` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_060: [** `interlocked_xor_64` shall return the initial value of `*destination`. **]**

## interlocked_xor_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile int8_t*, destination, int8_t, value);

```
**SRS_INTERLOCKED_LINUX_43_061: [** `interlocked_xor_8` shall call `atomic_fetch_xor` from `stdatomic.h`. **]**

**SRS_INTERLOCKED_LINUX_43_062: [** `interlocked_xor_8` shall return the initial value of `*destination`. **]**