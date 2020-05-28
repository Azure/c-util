# interlocked
================

## Overview

`interlocked` exports platform independent atomic operations. It is a platform abstraction and it requires a specific implementation for each platform.

The interface is based on the Windows [Interlocked API](https://docs.microsoft.com/en-us/windows/win32/sync/interlocked-variable-access).

## Notes

* The `interlocked` module does not perform validation on the arguments.
* All boolean operations are bitwise.
* All integer values are assumed to be using two's complement representation.

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

**SRS_INTERLOCKED_43_001: [** `interlocked_add` shall atomically add 32-bit integers `*addend` and `value` and store the result in `*addend`.**]**

**SRS_INTERLOCKED_43_032: [** `interlocked_add` shall return the result of the addition.**]**

## interlocked_add_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_add_64, volatile_atomic int64_t*, addend, int64_t, value);
```

**SRS_INTERLOCKED_43_065: [** `interlocked_add_64` shall atomically add 64-bit integers `*addend` and `value` and store the result in `*addend`. **]**

**SRS_INTERLOCKED_43_066: [** `interlocked_add_64` shall return the result of the addition. **]**

## interlocked_and

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_and, volatile_atomic int32_t*, destination, int32_t, value);
```

**SRS_INTERLOCKED_43_002: [** `interlocked_and` shall perform an atomic bitwise AND operation on the 32-bit integer values `*destination` and `value` and store the result in `*destination`.**]**

**SRS_INTERLOCKED_43_033: [** `interlocked_and` shall return the initial value of `*destination`.**]**

## interlocked_and_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_and_16, volatile_atomic int16_t*, destination, int16_t, value);
```

**SRS_INTERLOCKED_43_003: [** `interlocked_and_16` shall perform an atomic bitwise AND operation on the 16-bit integer values `*destination` and `value` and store the result in `*destination.`**]**

**SRS_INTERLOCKED_43_034: [** `interlocked_and_16` shall return the initial value of `*destination`.**]**

## interlocked_and_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_and_64, volatile_atomic int64_t*, destination, int64_t, value);
```

**SRS_INTERLOCKED_43_004: [** `interlocked_and_64` shall perform an atomic bitwise AND operation on the 64-bit integer values `*destination` and `value` and store the result in `*destination`.**]**

**SRS_INTERLOCKED_43_035: [** `interlocked_and_64` shall return the initial value of `*destination`.**]**


## interlocked_and_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_and_8, volatile_atomic int8_t*, destination, int8_t, value);
```

**SRS_INTERLOCKED_43_005: [** `interlocked_and_8` shall perform an atomic bitwise AND operation on the 8-bit integer values `*destination` and `value` and store the result in `*destination`**]**

**SRS_INTERLOCKED_43_036: [** `interlocked_and_8` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_compare_exchange, volatile_atomic int32_t*, destination, int32_t, exchange, int32_t, comperand);
```

**SRS_INTERLOCKED_43_006: [** `interlocked_compare_exchange` shall compare the 32-bit integers pointed to by `destination` and `comperand`. If they are equal, `*destination` is set to `exchange`. These operations are performed atomically.***]**

**SRS_INTERLOCKED_43_037: [** `interlocked_compare_exchange` shall return the initial value of `*destination`.**]**

## interlocked_compare_exchange_128

```c
MOCKABLE_FUNCTION(, bool, interlocked_compare_exchange_128, volatile_atomic int64_t*, destination, int64_t, exchange_high, int64_t, exchange_low, int64_t*, comperand_result);
```

Note: The `destination` and `comperand_result` parameters are arrays of two 64-bit integers considered as a 128-bit fields.

**SRS_INTERLOCKED_43_007: [** `interlocked_compare_exchange_128` shall compare `*destination` with `*comperand_result`. If they are equal, `destination[0]` is set to `exchange_low` and `destination[1]` is set to `exchange_high`. These operations are performed atomically.**]**

**SRS_INTERLOCKED_43_039: [** `interlocked_compare_exchange_128` shall store the initial value of `*destination` in `*comperand_result` regardless of the result of the comparison.` **]**

**SRS_INTERLOCKED_43_038: [** `interlocked_compare_exchange_128` shall return `true` if `*comperand_result` equals the original value of `*destination`.**]**

**SRS_INTERLOCKED_43_063: [** `interlocked_compare_exchange_128` shall return `false` if `*comperand_result` does not equal the original value of `*destination`. **]**

## interlocked_compare_exchange_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_compare_exchange_16, volatile_atomic int16_t*, destination, int16_t, exchange, int16_t, comperand);
```

**SRS_INTERLOCKED_43_009: [**`interlocked_compare_exchange_16` shall compare the 16-bit integers pointed to by `destination` and `comperand`. If they are equal, `*destination` is set to `exchange`. These operations are performed atomically.**]**

**SRS_INTERLOCKED_43_040: [** `interlocked_compare_exchange_16` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_compare_exchange_64, volatile_atomic int64_t*, destination, int64_t, exchange, int64_t, comperand);
```

**SRS_INTERLOCKED_43_008: [**`interlocked_compare_exchange_64` shall compare the 64-bit integers pointed to by `destination` and `comperand`. If they are equal, `*destination` is set to `exchange`. These operations are performed atomically.**]**

**SRS_INTERLOCKED_43_041: [** `interlocked_compare_exchange_64` shall return the initial value of `*destination`. **]**

## interlocked_compare_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_compare_exchange_pointer, void* volatile_atomic*, destination, void*, exchange, void*, comperand);
```

**SRS_INTERLOCKED_43_010: [**`interlocked_compare_exchange_pointer` shall compare the pointers `destination` and `comperand`. If they are equal, `*destination` is set to `exchange`. These operations are performed atomically.**]**

**SRS_INTERLOCKED_43_042: [** `interlocked_compare_exchange_pointer` shall return the initial value of `*destination`. **]**

## interlocked_decrement

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_decrement, volatile_atomic int32_t*, addend);
```

**SRS_INTERLOCKED_43_011: [** `interlocked_decrement` shall atomically decrement (decrease by one) the 32-bit variable `*addend`.**]**

**SRS_INTERLOCKED_43_043: [** `interlocked_decrement` shall return the decremented value. **]**

## interlocked_decrement_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_decrement_16, volatile_atomic int16_t*, addend);
```

**SRS_INTERLOCKED_43_012: [** `interlocked_decrement_16` shall atomically decrement (decrease by one) the 16-bit variable `*addend`.**]**

**SRS_INTERLOCKED_43_044: [** `interlocked_decrement_16` shall return the decremented value. **]**

## interlocked_decrement_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_decrement_64, volatile_atomic int64_t*, addend);
```

**SRS_INTERLOCKED_43_013: [** `interlocked_decrement_64` shall atomically decrement (decrease by one) the 64-bit variable `*addend`.**]**

**SRS_INTERLOCKED_43_045: [** `interlocked_decrement_64` shall return the decremented value. **]**


## interlocked_exchange

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange, volatile_atomic int32_t*, target, int32_t, value);
```

**SRS_INTERLOCKED_43_014: [** `interlocked_exchange` shall set the 32-bit variable pointed to by `target` to `value` as an atomic operation.**]**

**SRS_INTERLOCKED_43_046: [** `interlocked_exchange` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_exchange_16, volatile_atomic int16_t*, target, int16_t, value);
```

**SRS_INTERLOCKED_43_015: [** `interlocked_exchange_16` shall set the 16-bit variable pointed to by `target` to `value` as an atomic operation.**]**

**SRS_INTERLOCKED_43_047: [** `interlocked_exchange_16` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_64, volatile_atomic int64_t*, target, int64_t, value);
```

**SRS_INTERLOCKED_43_016: [** `interlocked_exchange_64` shall set the 64-bit variable pointed to by `target` to `value` as an atomic operation.**]**

**SRS_INTERLOCKED_43_048: [** `interlocked_exchange_64` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_exchange_8, volatile_atomic int8_t*, target, int8_t, value);
```

**SRS_INTERLOCKED_43_017: [** `interlocked_exchange_8` shall set the 8-bit variable pointed to by `target` to `value` as an atomic operation.**]**

**SRS_INTERLOCKED_43_049: [** `interlocked_exchange_8` shall return the initial value pointed to by `target`. **]**

## interlocked_exchange_add

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_exchange_add, volatile_atomic int32_t*, addend, int32_t, value);
```

**SRS_INTERLOCKED_43_018: [** `interlocked_exchange_add` shall perform an atomic addition of the 32-bit values `*addend` and `value` and store the result in `*addend`.**]**

**SRS_INTERLOCKED_43_050: [** `interlocked_exchange_add` shall return the initial value of `*addend`. **]**

## interlocked_exchange_add_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_exchange_add_64, volatile_atomic int64_t*, addend, int64_t, value);
```

**SRS_INTERLOCKED_43_019: [** `interlocked_exchange_add_64` shall perform an atomic addition of the 64-bit values `*addend` and `value` and store the result in `*addend`.**]**

**SRS_INTERLOCKED_43_064: [** `interlocked_exchange_add_64` shall return the initial value of `*addend`. **]**

## interlocked_exchange_pointer

```c
MOCKABLE_FUNCTION(, void*, interlocked_exchange_pointer, void* volatile_atomic*, target, void*, value);
```

**SRS_INTERLOCKED_43_020: [** `interlocked_exchange_pointer` shall atomically set `*target` to `value`**]**

**SRS_INTERLOCKED_43_051: [** `interlocked_exchange_pointer` shall return the initial value of `*target`. **]**

## interlocked_increment

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_increment, volatile_atomic int32_t*, addend);
```

**SRS_INTERLOCKED_43_021: [** `interlocked_increment` shall atomically increment (increase by one) the 32-bit variable `*addend`.**]**

**SRS_INTERLOCKED_43_052: [** `interlocked_increment` shall return the incremented value. **]**

## interlocked_increment_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_increment_16, volatile_atomic int16_t*, addend);
```

**SRS_INTERLOCKED_43_022: [** `interlocked_increment_16` shall atomically increment (increase by one) the 16-bit variable `*addend`.**]**

**SRS_INTERLOCKED_43_053: [** `interlocked_increment_16` shall return the incremented value. **]**

## interlocked_increment_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_increment_64, volatile_atomic int64_t*, addend);
```

**SRS_INTERLOCKED_43_023: [** `interlocked_increment_64` shall atomically increment (increase by one) the 64-bit variable `*addend`.**]**

**SRS_INTERLOCKED_43_054: [** `interlocked_increment_64` shall return the incremented value. **]**

## interlocked_or

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_or, volatile_atomic int32_t*, destination, int32_t, value);
```

**SRS_INTERLOCKED_43_024: [** `interlocked_or` shall perform an atomic bitwise OR operation on the 32-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_055: [** `interlocked_or` shall return the initial value of `*destination`. **]**

## interlocked_or_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_or_16, volatile_atomic int16_t*, destination, int16_t, value);
```

**SRS_INTERLOCKED_43_025: [** `interlocked_or_16` shall perform an atomic bitwise OR operation on the 16-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_056: [** `interlocked_or_16` shall return the initial value of `*destination`. **]**

## interlocked_or_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_or_64, volatile_atomic int64_t*, destination, int64_t, value);
```

**SRS_INTERLOCKED_43_026: [** `interlocked_or_64` shall perform an atomic bitwise OR operation on the 64-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_057: [** `interlocked_or_64` shall return the initial value of `*destination`. **]**


## interlocked_or_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_or_8, volatile_atomic int8_t*, destination, int8_t, value);
```

**SRS_INTERLOCKED_43_027: [** `interlocked_or_8` shall perform an atomic bitwise OR operation on the 8-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_058: [** `interlocked_or_8` shall return the initial value of `*destination`. **]**

## interlocked_xor

```c
MOCKABLE_FUNCTION(, int32_t, interlocked_xor, volatile_atomic int32_t*, destination, int32_t, value);
```

**SRS_INTERLOCKED_43_028: [** `interlocked_xor` shall perform an atomic bitwise XOR operation on the 32-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_059: [** `interlocked_xor` shall return the initial value of `*destination`. **]**

## interlocked_xor_16

```c
MOCKABLE_FUNCTION(, int16_t, interlocked_xor_16, volatile_atomic int16_t*, destination, int16_t, value);
```

**SRS_INTERLOCKED_43_029: [** `interlocked_xor_16` shall perform an atomic bitwise XOR operation on the 16-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_060: [** `interlocked_xor_16` shall return the initial value of `*destination`. **]**

## interlocked_xor_64

```c
MOCKABLE_FUNCTION(, int64_t, interlocked_xor_64, volatile_atomic int64_t*, destination, int64_t, value);
```

**SRS_INTERLOCKED_43_030: [** `interlocked_xor_64` shall perform an atomic bitwise XOR operation on the 64-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_061: [** `interlocked_xor_64` shall return the initial value of `*destination`. **]**

## interlocked_xor_8

```c
MOCKABLE_FUNCTION(, int8_t, interlocked_xor_8, volatile_atomic int8_t*, destination, int8_t, value);
```

**SRS_INTERLOCKED_43_031: [** `interlocked_xor_8` shall perform an atomic bitwise XOR operation on the 8-bit integers `*destination` and `value` and store the result in `destination`.**]**

**SRS_INTERLOCKED_43_062: [** `interlocked_xor_8` shall return the initial value of `*destination`. **]**