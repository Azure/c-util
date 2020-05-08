InterLockedIF
================

## Overview

InterLockedIF exports platform independent atomic operations. It is a platform abstraction and it requires a specific implementation for each platform.

The interface is based on windows interlocked api.

## Exposed API

```c
long InterlockedIFAdd( 
    long volatile *Addend,
    long           value
);

long InterlockedIFAnd( 
    long volatile *Destination,
    long          Value
);

short InterlockedIFAnd16(
  short volatile *Destination,
  short          Value
);

int64_t InterlockedIFAnd64(
  int64_t volatile *Destination,
  int64_t          Value
);

char InterlockedIFAnd8(
  char volatile *Destination,
  char          Value
);

long InterlockedIFCompareExchange(
  long volatile *Destination,
  long          ExChange,
  long          Comperand
);

short InterlockedIFCompareExchange16(
  short volatile *Destination,
  short          ExChange,
  short          Comperand
);

int64_t InterlockedIFCompareExchange64(
  int64_t volatile *Destination,
  int64_t          ExChange,
  int64_t          Comperand
);

void* InterlockedIFCompareExchangePointer(
  void* volatile *Destination,
  void*          Exchange,
  void*          Comperand
);

long InterlockedIFDecrement(
  long volatile *Addend
);

short InterlockedIFDecrement16(
  short volatile *Addend
);

int64_t InterlockedIFDecrement64(
  int64_t volatile *Addend
);

long InterlockedIFExchange(
  long volatile *Target,
  long          Value
);

short InterlockedIFExchange16(
  short volatile *Destination,
  short          ExChange
);

int64_t InterlockedIFExchange64(
  int64_t volatile *Target,
  int64_t          Value
);

char InterlockedIFExchange8(
  char volatile *Target,
  char          Value
);

long InterlockedIFExchangeAdd(
  long volatile *Addend,
  long          Value
);

int64_t InterlockedIFExchangeAdd64(
  int64_t volatile *Addend,
  int64_t          Value
);

void* InterlockedIFExchangePointer(
  void* volatile *Target,
  void*          Value
);

long InterlockedIFIncrement(
  long volatile *Addend
);

short InterlockedIFIncrement16(
  short volatile *Addend
);

int64_t InterlockedIFIncrement64(
  int64_t volatile *Addend
);

long InterlockedIFOr(
  long volatile *Destination,
  long          Value
);

short InterlockedIFOr16(
  short volatile *Destination,
  short          Value
);

int64_t InterlockedIFOr64(
  int64_t volatile *Destination,
  int64_t          Value
);

char InterlockedIFOr8(
  char volatile *Destination,
  char          Value
);

long InterlockedIFXor(
  long volatile *Destination,
  long          Value
);

short InterlockedIFXor16(
  short volatile *Destination,
  short          Value
);

int64_t InterlockedIFXor64(
  int64_t volatile *Destination,
  int64_t          Value
);

char InterlockedIFXor8(
  char volatile *Destination,
  char          Value
);
```

## InterlockedIFAdd

```c
long InterlockedIFAdd( 
    long volatile *Addend,
    long           value
);
```

**SRS_INTERLOCKED_IF_43_001 [** `InterlockedIFAdd` shall perform an atomic addition operation on the specified `long` values.**]**



## InterlockedIFAnd

```c
long InterlockedIFAnd( 
    long volatile *Destination,
    long          Value
);
```

**SRS_INTERLOCKED_IF_43_002 [** `InterlockedIFAnd` shall perform an atomic AND operation on the specified `long` values.**]**

## InterlockedIFAnd16

```c
short InterlockedIFAnd16(
  short volatile *Destination,
  short          Value
);
```

**SRS_INTERLOCKED_IF_43_003 [** `InterlockedIFAnd16` shall perform an atomic AND operation on the specified `short` values.**]**


## InterlockedIFAnd64

```c
int64_t InterlockedIFAnd64(
  int64_t volatile *Destination,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_004 [** `InterlockedIFAnd64` shall perform an atomic AND operation on the specified `int64_t` values.**]**

## InterlockedIFAnd8

```c
char InterlockedIFAnd8(
  char volatile *Destination,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_005 [** `InterlockedIFAnd8` shall perform an atomic AND operation on the specified `char` values.**]**

## InterlockedIFCompareExchange

```c
long InterlockedIFCompareExchange(
  long volatile *Destination,
  long          ExChange,
  long          Comperand
);
```

**SRS_INTERLOCKED_IF_43_006 [** `InterlockedIFCompareExchange` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 32-bit values and exchanges with another 32-bit value based on the outcome of the comparison.**]**


## InterlockedIFCompareExchange16

```c
short InterlockedIFCompareExchange16(
  short volatile *Destination,
  short          ExChange,
  short          Comperand
);
```

**SRS_INTERLOCKED_IF_43_007 [** `InterlockedIFCompareExchange16` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 16-bit values and exchanges with another 16-bit value based on the outcome of the comparison.**]**

## InterlockedIFCompareExchange64

```c
int64_t InterlockedIFCompareExchange64(
  int64_t volatile *Destination,
  int64_t          ExChange,
  int64_t          Comperand
);
```

**SRS_INTERLOCKED_IF_43_008 [** `InterlockedIFCompareExchange64` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified 64-bit values and exchanges with another 64-bit value based on the outcome of the comparison.**]**

## InterlockedIFCompareExchangePointer

```c
void* InterlockedIFCompareExchangePointer(
  void* volatile *Destination,
  void*          Exchange,
  void*          Comperand
);
```

**SRS_INTERLOCKED_IF_43_09 [** `InterlockedIFCompareExchangePointer` shall perform an atomic compare-and-exchange operation on the specified values. The function compares two specified pointer values and exchanges with another pointer value based on the outcome of the comparison.**]**

## InterlockedIFDecrement

```c
long InterlockedIFDecrement(
  long volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_010[** `InterlockedIFDecrement` shall decrement (decrease by one) the value of the specified 32-bit variable as an atomic operation.**]**

## InterlockedIFDecrement16

```c
short InterlockedIFDecrement16(
  short volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_011 [** `InterlockedIFDecrement16` shall decrement (decrease by one) the value of the specified 16-bit variable as an atomic operation.**]**


## InterlockedIFDecrement64

```c
int64_t InterlockedIFDecrement64(
  int64_t volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_012 [** `InterlockedIFDecrement64` shall decrement (decrease by one) the value of the specified 64-bit variable as an atomic operation.**]**

## InterlockedIFExchange

```c
long InterlockedIFExchange(
  long volatile *Target,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_013 [** `InterlockedIFExchange` shall set a 32-bit variable to the specified value as an atomic operation.**]**

## InterlockedIFExchange16

```c
short InterlockedIFExchange16(
  short volatile *Destination,
  short          ExChange
);
```

**SRS_INTERLOCKED_IF_43_014 [** `InterlockedIFExchange16` shall set a 16-bit variable to the specified value as an atomic operation.**]**


## InterlockedIFExchange64

```c
int64_t InterlockedIFExchange64(
  int64_t volatile *Target,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_015 [** `InterlockedIFExchange64` shall set a 64-bit variable to the specified value as an atomic operation.**]**

## InterlockedIFExchange8

```c
char InterlockedIFExchange8(
  char volatile *Target,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_016 [** `InterlockedIFExchange8` shall set an 8-bit variable to the specified value as an atomic operation.**]**

## InterlockedIFExchangeAdd

```c
long InterlockedIFExchangeAdd(
  long volatile *Addend,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_017 [** `InterlockedIFExchangeAdd` shall perform an atomic addition of two 32-bit values.**]**

## InterlockedIFExchangeAdd64

```c
int64_t InterlockedIFExchangeAdd64(
  int64_t volatile *Addend,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_018 [** `InterlockedIFExchangeAdd64` shall perform an atomic addition of two 64-bit values.**]**

## InterlockedIFExchangePointer

```c
void* InterlockedIFExchangePointer(
  void* volatile *Target,
  void*          Value
);
```

**SRS_INTERLOCKED_IF_43_019 [** `InterlockedIFExchangePointer` shall atomically exchange a pair of addresses.**]**

## InterlockedIFIncrement

```c
long InterlockedIFIncrement(
  long volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_020 [** `InterlockedIFIncrement` shall increment (increase by one) the value of the specified 32-bit variable as an atomic operation.**]**

## InterlockedIFIncrement16

```c
short InterlockedIFIncrement16(
  short volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_021 [** `InterlockedIFIncrement16` shall increment (increase by one) the value of the specified 16-bit variable as an atomic operation.**]**

## InterlockedIFIncrement64

```c
int64_t InterlockedIFIncrement64(
  int64_t volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_022 [** `InterlockedIFIncrement64` shall increment (increase by one) the value of the specified 64-bit variable as an atomic operation.**]**

## InterlockedIFOr

```c
long InterlockedIFOr(
  long volatile *Destination,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_023 [** `InterlockedIFOr` shall perform an atomic OR operation on the specified `long` values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFOr16

```c
short InterlockedIFOr16(
  short volatile *Destination,
  short          Value
);
```

**SRS_INTERLOCKED_IF_43_024 [** `InterlockedIFOr16` shall perform an atomic OR operation on the specified `short` values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFOr64

```c
int64_t InterlockedIFOr64(
  int64_t volatile *Destination,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_025 [** `InterlockedIFOr64` shall perform an atomic OR operation on the specified `int64_t` values. The function prevents more than one thread from using the same variable simultaneously.**]**


## InterlockedIFOr8

```c
char InterlockedIFOr8(
  char volatile *Destination,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_026 [** `InterlockedIFOr8` shall perform an atomic OR operation on the specified `char` values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor

```c
long InterlockedIFXor(
  long volatile *Destination,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_027 [** `InterlockedIFXor` shall perform an atomic XOR operation on the specified `long` values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor16

```c
short InterlockedIFXor16(
  short volatile *Destination,
  short          Value
);
```

**SRS_INTERLOCKED_IF_43_028 [** `InterlockedIFXor16` shall perform an atomic XOR operation on the specified `short` values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor64

```c
int64_t InterlockedIFXor64(
  int64_t volatile *Destination,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_029 [** `InterlockedIFXor64` shall perform an atomic XOR operation on the specified `int64_t` values. The function prevents more than one thread from using the same variable simultaneously.**]**

## InterlockedIFXor8

```c
char InterlockedIFXor8(
  char volatile *Destination,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_030 [** `InterlockedIFXor8` shall perform an atomic XOR operation on the specified `char` values. The function prevents more than one thread from using the same variable simultaneously.**]**






