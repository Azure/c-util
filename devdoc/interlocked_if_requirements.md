InterLockedIF
================

## Overview

InterLockedIF exports platform independent atomic operations. It is a platform abstraction and it requires a specific implementation for each platform.

The interface is based on windows interlocked api.

## Exposed API

```c
int32_t InterlockedIFAdd( 
    int32_t volatile *addend,
    int32_t value
);

int32_t InterlockedIFAnd( 
    int32_t volatile *destination,
    int32_t value
);

int32_t InterlockedIFAnd16(
    int32_t volatile *destination,
    int32_t value
); 

int64_t InterlockedIFAnd64(
    int64_t volatile *destination,
    int64_t value
);

int16_t InterlockedIFAnd8(
    int16_t volatile *destination,
    int16_t value
);

int32_t InterlockedIFCompareExchange(
    int32_t volatile *destination,
    int32_t exchange,
    int32_t comperand
);

int32_t InterlockedIFCompareExchange16(
    int32_t volatile *destination,
    int32_t exchange,
    int32_t comperand
);

int64_t InterlockedIFCompareExchange64(
    int64_t volatile *destination,
    int64_t exchange,
    int64_t comperand
);

void* InterlockedIFCompareExchangePointer(
    void* volatile *destination,
    void* exchange,
    void* comperand
);

int32_t InterlockedIFDecrement(
    int32_t volatile *addend
);

int32_t InterlockedIFDecrement16(
    int32_t volatile *addend
);

int64_t InterlockedIFDecrement64(
    int64_t volatile *addend
);

int32_t InterlockedIFExchange(
    int32_t volatile *target,
    int32_t value
);

int32_t InterlockedIFExchange16(
    int32_t volatile *destination,
    int32_t exchange
);

int64_t InterlockedIFExchange64(
    int64_t volatile *target,
    int64_t value
);

int16_t InterlockedIFExchange8(
    int16_t volatile *target,
    int16_t value
);

int32_t InterlockedIFExchangeAdd(
    int32_t volatile *addend,
    int32_t value
);

int64_t InterlockedIFExchangeAdd64(
    int64_t volatile *addend,
    int64_t value
);

void* InterlockedIFExchangePointer(
    void* volatile *target,
    void* value
);

int32_t InterlockedIFIncrement(
    int32_t volatile *addend
);

int32_t InterlockedIFIncrement16(
    int32_t volatile *addend
);

int64_t InterlockedIFIncrement64(
    int64_t volatile *addend
);

int32_t InterlockedIFOr(
    int32_t volatile *destination,
    int32_t          value
);

int32_t InterlockedIFOr16(
    int32_t volatile *destination,
    int32_t value
);

int64_t InterlockedIFOr64(
    int64_t volatile *destination,
    int64_t value
);

int16_t InterlockedIFOr8(
    int16_t volatile *destination,
    int16_t value
);

int32_t InterlockedIFXor(
    int32_t volatile *destination,
    int32_t value
);

int32_t InterlockedIFXor16(
    int32_t volatile *destination,
    int32_t value
);

int64_t InterlockedIFXor64(
    int64_t volatile *destination,
    int64_t value
);

int16_t InterlockedIFXor8(
    int16_t volatile *destination,
    int16_t value
);
```

## InterlockedIFAdd

```c
int32_t InterlockedIFAdd( 
    int32_t volatile *addend,
    int32_t           value
);
```

**SRS_INTERLOCKED_IF_43_001 [** `InterlockedIFAdd` shall perform an atomic addition operation on the specified 32-bit integer values.**]**



## InterlockedIFAnd

```c
int64_t InterlockedIFAnd( 
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_002 [** `InterlockedIFAnd` shall perform an atomic AND operation on the specified 32-bit integer values.**]**

## InterlockedIFAnd16

```c
int16_t InterlockedIFAnd16(
    int16_t volatile *destination,
    int16_t value
);
```

**SRS_INTERLOCKED_IF_43_003 [** `InterlockedIFAnd16` shall perform an atomic AND operation on the specified 16-bit integer values.**]**


## InterlockedIFAnd64

```c
int64_t InterlockedIFAnd64(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_004 [** `InterlockedIFAnd64` shall perform an atomic AND operation on the specified 64-bit integer values.**]**

## InterlockedIFAnd8

```c
int8_t InterlockedIFAnd8(
    int8_t volatile *destination,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_43_005 [** `InterlockedIFAnd8` shall perform an atomic AND operation on the specified 8-bit integer values.**]**

## InterlockedIFCompareExchange

```c
int64_t InterlockedIFCompareExchange(
    int64_t volatile *destination,
    int64_t exchange,
    int64_t comperand
);
```

**SRS_INTERLOCKED_IF_43_006 [** `InterlockedIFCompareExchange` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 32-bit values and exchanges with another 32-bit value based on the outcome of the comparison.**]**


## InterlockedIFCompareExchange16

```c
int16_t InterlockedIFCompareExchange16(
    int16_t volatile *destination,
    int16_t exchange,
    int16_t comperand
);
```

**SRS_INTERLOCKED_IF_43_007 [** `InterlockedIFCompareExchange16` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 16-bit values and exchanges with another 16-bit value based on the outcome of the comparison.**]**

## InterlockedIFCompareExchange64

```c
int64_t InterlockedIFCompareExchange64(
    int64_t volatile *destination,
    int64_t exchange,
    int64_t comperand
);
```

**SRS_INTERLOCKED_IF_43_008 [** `InterlockedIFCompareExchange64` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 64-bit values and exchanges with another 64-bit value based on the outcome of the comparison.**]**

## InterlockedIFCompareExchangePointer

```c
void* InterlockedIFCompareExchangePointer(
    void* volatile *destination,
    void* exchange,
    void* comperand
);
```

**SRS_INTERLOCKED_IF_43_09 [** `InterlockedIFCompareExchangePointer` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified pointer values and exchanges with another pointer value based on the outcome of the comparison.**]**

## InterlockedIFDecrement

```c
int64_t InterlockedIFDecrement(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_43_010[** `InterlockedIFDecrement` shall decrement (decrease by one) the value of the specified 32-bit variable as an atomic operation.**]**

## InterlockedIFDecrement16

```c
int16_t InterlockedIFDecrement16(
    int16_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_43_011 [** `InterlockedIFDecrement16` shall decrement (decrease by one) the value of the specified 16-bit variable as an atomic operation.**]**


## InterlockedIFDecrement64

```c
int64_t InterlockedIFDecrement64(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_43_012 [** `InterlockedIFDecrement64` shall decrement (decrease by one) the value of the specified 64-bit variable as an atomic operation.**]**

## InterlockedIFExchange

```c
int64_t InterlockedIFExchange(
    int64_t volatile *target,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_013 [** `InterlockedIFExchange` shall set a 32-bit variable to the specified value as an atomic operation.**]**

## InterlockedIFExchange16

```c
int16_t InterlockedIFExchange16(
    int16_t volatile *destination,
    int16_t exchange
);
```

**SRS_INTERLOCKED_IF_43_014 [** `InterlockedIFExchange16` shall set a 16-bit variable to the specified value as an atomic operation.**]**


## InterlockedIFExchange64

```c
int64_t InterlockedIFExchange64(
    int64_t volatile *target,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_015 [** `InterlockedIFExchange64` shall set a 64-bit variable to the specified value as an atomic operation.**]**

## InterlockedIFExchange8

```c
int8_t InterlockedIFExchange8(
    int8_t volatile *target,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_43_016 [** `InterlockedIFExchange8` shall set an 8-bit variable to the specified value as an atomic operation.**]**

## InterlockedIFExchangeAdd

```c
int64_t InterlockedIFExchangeAdd(
    int64_t volatile *addend,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_017 [** `InterlockedIFExchangeAdd` shall perform an atomic addition of two 32-bit values.**]**

## InterlockedIFExchangeAdd64

```c
int64_t InterlockedIFExchangeAdd64(
    int64_t volatile *addend,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_018 [** `InterlockedIFExchangeAdd64` shall perform an atomic addition of two 64-bit values.**]**

## InterlockedIFExchangePointer

```c
void* InterlockedIFExchangePointer(
    void* volatile *target,
    void* value
);
```

**SRS_INTERLOCKED_IF_43_019 [** `InterlockedIFExchangePointer` shall atomically exchange a pair of addresses.**]**

## InterlockedIFIncrement

```c
int64_t InterlockedIFIncrement(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_43_020 [** `InterlockedIFIncrement` shall increment (increase by one) the value of the specified 32-bit variable as an atomic operation.**]**

## InterlockedIFIncrement16

```c
int16_t InterlockedIFIncrement16(
    int16_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_43_021 [** `InterlockedIFIncrement16` shall increment (increase by one) the value of the specified 16-bit variable as an atomic operation.**]**

## InterlockedIFIncrement64

```c
int64_t InterlockedIFIncrement64(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_43_022 [** `InterlockedIFIncrement64` shall increment (increase by one) the value of the specified 64-bit variable as an atomic operation.**]**

## InterlockedIFOr

```c
int64_t InterlockedIFOr(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_023 [** `InterlockedIFOr` shall perform an atomic OR operation on the specified 32-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFOr16

```c
int16_t InterlockedIFOr16(
    int16_t volatile *destination,
    int16_t value
);
```

**SRS_INTERLOCKED_IF_43_024 [** `InterlockedIFOr16` shall perform an atomic OR operation on the specified 16-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFOr64

```c
int64_t InterlockedIFOr64(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_025 [** `InterlockedIFOr64` shall perform an atomic OR operation on the specified 64-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**


## InterlockedIFOr8

```c
int8_t InterlockedIFOr8(
    int8_t volatile *destination,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_43_026 [** `InterlockedIFOr8` shall perform an atomic OR operation on the specified 8-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor

```c
int64_t InterlockedIFXor(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_027 [** `InterlockedIFXor` shall perform an atomic XOR operation on the specified 32-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor16

```c
int16_t InterlockedIFXor16(
    int16_t volatile *destination,
    int16_t value
);
```

**SRS_INTERLOCKED_IF_43_028 [** `InterlockedIFXor16` shall perform an atomic XOR operation on the specified 16-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor64

```c
int64_t InterlockedIFXor64(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_43_029 [** `InterlockedIFXor64` shall perform an atomic XOR operation on the specified 64-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor8

```c
int8_t InterlockedIFXor8(
    int8_t volatile *destination,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_43_030 [** `InterlockedIFXor8` shall perform an atomic XOR operation on the specified 8-bit integer values. The function prevents more than one thread from using the same variable simultaneously.**]**