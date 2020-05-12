# interlocked
================

## Overview

`interlocked` exports platform independent atomic operations. It is a platform abstraction and it requires a specific implementation for each platform.

The interface is based on the Windows Interlocked API.

## Exposed API

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_add, volatile int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile int32_t*, destination, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile int64_t*, destination, int64_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_and_8, volatile int8_t*, destination, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile int32_t*, destination, int32_t, exchange, int32_t, comperand);
MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile int64_t*, destination, int64_t, exchange_low, int64_t, exchange_high, int64_t, comperand_result);
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile int16_t*, destination, int16_t, exchange, int16_t, comperand);
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile int64_t*, destination, int64_t, exchange, int64_t, comperand);
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, volatile void*, destination, void*, exchange, void*, comperand);
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile int16_t*, addend);
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile int64_t*, addend);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile int32_t*, target, int32_t, value);
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile int16_t*, target, int16_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile int64_t*, target, int64_t, value);
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile int8_t*, target, int8_t, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile int32_t*, addend, int32_t, value);
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile int64_t*, addend, int64_t, value);
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, volatile void*, target, void*, value);
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile int32_t*, addend);
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile in       t16_t*, addend);
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

**SRS_INTERLOCKED_43_001 [** `interlocked_add` shall perform an atomic addition operation on the specified 32-bit integer values.**]**



## interlocked_and

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile int32_t*, destination, int32_t, value);
```

**SRS_INTERLOCKED_43_002 [** `interlocked_and` shall perform an atomic AND operation on the specified 32-bit integer values.**]**

## interlocked_and_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value);
```

**SRS_INTERLOCKED_43_003 [** `interlocked_and_16` shall perform an atomic AND operation on the specified 16-bit integer values.**]**


## interlocked_and_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile int64_t*, destination, int64_t, value);
```

**SRS_INTERLOCKED_43_004 [** `interlocked_and_64` shall perform an atomic AND operation on the specified 64-bit integer values.**]**

## interlocked_and_8

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_and_8, volatile int8_t*, destination, int8_t, value);
```

**SRS_INTERLOCKED_43_005 [** `interlocked_and_8` shall perform an atomic AND operation on the specified 8-bit integer values.**]**

## interlocked_compare_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile int32_t*, destination, int32_t, exchange, int32_t, comperand);
```

**SRS_INTERLOCKED_43_006 [** `interlocked_compare_exchange` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 32-bit values and exchanges with another 32-bit value based on the outcome of the comparison.**]**

## interlocked_compare_exchange_128

```c
MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile int64_t*, destination, int64_t, exchange_low, int64_t, exchange_high, int64_t, comperand_result);
```

**SRS_INTERLOCKED_43_007 [** `interlocked_compare_exchange_128` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 128-bit values and exchanges with another 128-bit value based on the outcome of the comparison.**]**

## interlocked_and_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile int16_t*, destination, int16_t, value);
```

**SRS_INTERLOCKED_43_008 [** `interlocked_and_16` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 16-bit values and exchanges with another 16-bit value based on the outcome of the comparison.**]**

## interlocked_compare_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile int64_t*, destination, int64_t, exchange, int64_t, comperand);
```

**SRS_INTERLOCKED_43_009 [** `interlocked_compare_exchange_64` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 64-bit values and exchanges with another 64-bit value based on the outcome of the comparison.**]**

## interlocked_compare_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, volatile void*, destination, void*, exchange, void*, comperand);
```

**SRS_INTERLOCKED_43_010 [** `interlocked_compare_exchange_pointer` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified pointer values and exchanges with another pointer value based on the outcome of the comparison.**]**

## interlocked_decrement

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile int32_t*, addend);
```

**SRS_INTERLOCKED_43_011[** `interlocked_decrement` shall decrement (decrease by one) the value of the specified 32-bit variable as an atomic operation.**]**

## interlocked_decrement_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile int16_t*, addend);
```

**SRS_INTERLOCKED_43_012 [** `interlocked_decrement_16` shall decrement (decrease by one) the value of the specified 16-bit variable as an atomic operation.**]**


## interlocked_decrement_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile int64_t*, addend);
```

**SRS_INTERLOCKED_43_013 [** `interlocked_decrement_64` shall decrement (decrease by one) the value of the specified 64-bit variable as an atomic operation.**]**

## interlocked_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile int32_t*, target, int32_t, value);
```

**SRS_INTERLOCKED_43_014 [** `interlocked_exchange` shall set a 32-bit variable to the specified value as an atomic operation.**]**

## interlocked_exchange_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile int16_t*, target, int16_t, value);
```

**SRS_INTERLOCKED_43_015 [** `interlocked_exchange_16` shall set a 16-bit variable to the specified value as an atomic operation.**]**


## interlocked_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile int64_t*, target, int64_t, value);
```

**SRS_INTERLOCKED_43_016 [** `interlocked_exchange_64` shall set a 64-bit variable to the specified value as an atomic operation.**]**

## interlocked_exchange_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile int8_t*, target, int8_t, value);
```

**SRS_INTERLOCKED_43_017 [** `interlocked_exchange_8` shall set an 8-bit variable to the specified value as an atomic operation.**]**

## interlocked_exchange_add

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile int32_t*, addend, int32_t, value);
```

**SRS_INTERLOCKED_43_018 [** `interlocked_exchange_add` shall perform an atomic addition of two 32-bit values.**]**

## interlocked_exchange_add_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile int64_t*, addend, int64_t, value);
```

**SRS_INTERLOCKED_43_019 [** `interlocked_exchange_add_64` shall perform an atomic addition of two 64-bit values.**]**

## interlocked_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, volatile void*, target, void*, value);
```

**SRS_INTERLOCKED_43_020 [** `interlocked_exchange_pointer` shall atomically exchange a pair of addresses.**]**

## interlocked_increment

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile int32_t*, addend);
```

**SRS_INTERLOCKED_43_021 [** `interlocked_increment` shall increment (increase by one) the value of the specified 32-bit variable as an atomic operation.**]**

## interlocked_increment_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile int16_t*, addend);
```

**SRS_INTERLOCKED_43_022 [** `interlocked_increment_16` shall increment (increase by one) the value of the specified 16-bit variable as an atomic operation.**]**

## interlocked_increment_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile int64_t*, addend);
```

**SRS_INTERLOCKED_43_023 [** `interlocked_increment_64` shall increment (increase by one) the value of the specified 64-bit variable as an atomic operation.**]**

## interlocked_or

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile int32_t*, destination, int32_t, value);
```

**SRS_INTERLOCKED_43_024 [** `interlocked_or` shall perform an atomic OR operation on the specified 32-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## interlocked_or_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile int16_t*, destination, int16_t, value);
```

**SRS_INTERLOCKED_43_025 [** `interlocked_or_16` shall perform an atomic OR operation on the specified 16-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## interlocked_or_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile int64_t*, destination, int64_t, value);
```

**SRS_INTERLOCKED_43_026 [** `interlocked_or_64` shall perform an atomic OR operation on the specified 64-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**


## interlocked_or_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile int8_t*, destination, int8_t, value);
```

**SRS_INTERLOCKED_43_027 [** `interlocked_or_8` shall perform an atomic OR operation on the specified 8-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## interlocked_xor

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile int32_t*, destination, int32_t, value);
```

**SRS_INTERLOCKED_43_028 [** `interlocked_xor` shall perform an atomic XOR operation on the specified 32-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## interlocked_xor_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile int16_t*, destination, int16_t, value);
```

**SRS_INTERLOCKED_43_029 [** `interlocked_xor_16` shall perform an atomic XOR operation on the specified 16-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## interlocked_xor_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile int64_t*, destination, int64_t, value);
```

**SRS_INTERLOCKED_43_030 [** `interlocked_xor_64` shall perform an atomic XOR operation on the specified 64-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## interlocked_xor_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile int8_t*, destination, int8_t, value);
```

**SRS_INTERLOCKED_43_031 [** `interlocked_xor_8` shall perform an atomic XOR operation on the specified 8-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**