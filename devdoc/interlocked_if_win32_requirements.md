InterLockedIF Win32
================

## Overview

InterLockedIF Win32 is the windows-specific implementation of the InterlockedIF interface. Each function calls the corresponding  `Interlocked` function from `wintt.h`.

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
    int32_t value
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
    int32_t value
);
```
**SRS_INTERLOCKED_IF_WIN32_43_001 [** `InterlockedIFAdd` shall call `InterlockedAdd` from `wintt.h`.**]**

## InterlockedIFAnd

```c
int64_t InterlockedIFAnd( 
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_002 [** `InterlockedIFAnd` shall call `InterlockedAnd` from `wintt.h`.**]**

## InterlockedIFAnd16

```c
int16_t InterlockedIFAnd16(
    int16_t volatile *destination,
    int16_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_003 [** `InterlockedIFAnd16` shall call `InterlockedAnd16` from `wintt.h`.**]**


## InterlockedIFAnd64

```c
int64_t InterlockedIFAnd64(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_004 [** `InterlockedIFAnd64` shall call `InterlockedAnd64` from `wintt.h`.**]**

## InterlockedIFAnd8

```c
int8_t InterlockedIFAnd8(
    int8_t volatile *destination,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_005 [** `InterlockedIFAnd8` shall call `InterlockedAnd8` from `wintt.h`.**]**

## InterlockedIFCompareExchange

```c
int64_t InterlockedIFCompareExchange(
    int64_t volatile *destination,
    int64_t exchange,
    int64_t comperand
);
```

**SRS_INTERLOCKED_IF_WIN32_43_006 [** `InterlockedIFCompareExchange` shall call `InterlockedCompareExchange` from `wintt.h`.**]**

## InterlockedIFCompareExchange16

```c
int16_t InterlockedIFCompareExchange16(
    int16_t volatile *destination,
    int16_t exchange,
    int16_t comperand
);  
```

**SRS_INTERLOCKED_IF_WIN32_43_007 [** `InterlockedIFCompareExchange16` shall call `InterlockedCompareExchange16` from `wintt.h`.**]**

## InterlockedIFCompareExchange64

```c
int64_t InterlockedIFCompareExchange64(
  int64_t volatile *destination,
  int64_t exchange,
  int64_t comperand
);
```

**SRS_INTERLOCKED_IF_WIN32_43_008 [** `InterlockedIFCompareExchange64` shall call `InterlockedCompareExchange64` from `wintt.h`.**]**

## InterlockedIFCompareExchangePointer

```c
void* InterlockedIFCompareExchangePointer(
    void* volatile *destination,
    void* exchange,
    void* comperand
);
```

**SRS_INTERLOCKED_IF_WIN32_43_09 [** `InterlockedIFCompareExchangePointer` shall call `InterlockedCompareExchangePointer` from `wintt.h`.**]**

## InterlockedIFDecrement

```c
int64_t InterlockedIFDecrement(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_WIN32_43_010[** `InterlockedIFDecrement` shall call `InterlockedDecrement` from `wintt.h`.**]**

## InterlockedIFDecrement16

```c
int16_t InterlockedIFDecrement16(
    int16_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_WIN32_43_011 [** `InterlockedIFDecrement16` shall call `InterlockedDecrement16` from `wintt.h`.**]**


## InterlockedIFDecrement64

```c
int64_t InterlockedIFDecrement64(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_WIN32_43_012 [** `InterlockedIFDecrement64` shall call `InterlockedDecrement64` from `wintt.h`.**]**

## InterlockedIFExchange

```c
int64_t InterlockedIFExchange(
    int64_t volatile *target,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_013 [** `InterlockedIFExchange` shall call `InterlockedExchange` from `wintt.h`.**]**

## InterlockedIFExchange16

```c
int16_t InterlockedIFExchange16(
    int16_t volatile *destination,
    int16_t exchange
);
```

**SRS_INTERLOCKED_IF_WIN32_43_014 [** `InterlockedIFExchange16` shall call `InterlockedExchange16` from `wintt.h`.**]**


## InterlockedIFExchange64

```c
int64_t InterlockedIFExchange64(
    int64_t volatile *target,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_015 [** `InterlockedIFExchange64` shall call `InterlockedExchange64` from `wintt.h`.**]**

## InterlockedIFExchange8

```c
int8_t InterlockedIFExchange8(
    int8_t volatile *target,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_016 [** `InterlockedIFExchange8` shall call `InterlockedExchange8` from `wintt.h`.**]**

## InterlockedIFExchangeAdd

```c
int64_t InterlockedIFExchangeAdd(
    int64_t volatile *addend,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_017 [** `InterlockedIFExchangeAdd` shall call `InterlockedExchangeAdd` from `wintt.h`.**]**

## InterlockedIFExchangeAdd64

```c
int64_t InterlockedIFExchangeAdd64(
    int64_t volatile *addend,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_018 [** `InterlockedIFExchangeAdd64` shall call `InterlockedExchangeAdd64` from `wintt.h`.**]**

## InterlockedIFExchangePointer

```c
void* InterlockedIFExchangePointer(
    void* volatile *target,
    void* value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_019 [** `InterlockedIFExchangePointer` shall call `InterlockedExchangePointer` from `wintt.h`.**]**

## InterlockedIFIncrement

```c
int64_t InterlockedIFIncrement(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_WIN32_43_020 [** `InterlockedIFIncrement` shall call `InterlockedIncrement` from `wintt.h`.**]**

## InterlockedIFIncrement16

```c
int16_t InterlockedIFIncrement16(
    int16_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_WIN32_43_021 [** `InterlockedIFIncrement16` shall call `InterlockedIncrement16` from `wintt.h`.**]**

## InterlockedIFIncrement64

```c
int64_t InterlockedIFIncrement64(
    int64_t volatile *addend
);
```

**SRS_INTERLOCKED_IF_WIN32_43_022 [** `InterlockedIFIncrement64` shall call `InterlockedIncrement64` from `wintt.h`.**]**

## InterlockedIFOr

```c
int64_t InterlockedIFOr(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_023 [** `InterlockedIFOr` shall call `InterlockedOr` from `wintt.h`.**]**

## InterlockedIFOr16

```c
int16_t InterlockedIFOr16(
    int16_t volatile *destination,
    int16_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_024 [** `InterlockedIFOr16` shall call `InterlockedOr16` from `wintt.h`.**]**

## InterlockedIFOr64

```c
int64_t InterlockedIFOr64(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_025 [** `InterlockedIFOr64` shall call `InterlockedOr64` from `wintt.h`.**]**


## InterlockedIFOr8

```c
int8_t InterlockedIFOr8(
    int8_t volatile *destination,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_026 [** `InterlockedIFOr8` shall call `InterlockedOr8` from `wintt.h`.**]**

## InterlockedIFXor

```c
int64_t InterlockedIFXor(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_027 [** `InterlockedIFXor` shall call `InterlockedXor` from `wintt.h`.**]**

## InterlockedIFXor16

```c
int16_t InterlockedIFXor16(
    int16_t volatile *destination,
    int16_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_028 [** `InterlockedIFXor16` shall call `InterlockedXor16` from `wintt.h`.**]**

## InterlockedIFXor64

```c
int64_t InterlockedIFXor64(
    int64_t volatile *destination,
    int64_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_029 [** `InterlockedIFXor64` shall call `InterlockedXor64` from `wintt.h`.**]**

## InterlockedIFXor8

```c
int8_t InterlockedIFXor8(
    int8_t volatile *destination,
    int8_t value
);
```

**SRS_INTERLOCKED_IF_WIN32_43_030 [** `InterlockedIFXor8` shall call `InterlockedXor8` from `wintt.h`.**]**