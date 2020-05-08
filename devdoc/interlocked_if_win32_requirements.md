InterLockedIF Win32
================

## Overview

InterLockedIF Win32 is the windows-specific implementation of the InterlockedIF interface. Each function calls the corresponding  `Interlocked` function from `wintt.h`.

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

**SRS_INTERLOCKED_IF_43_001 [** `InterlockedIFAdd` shall call `InterlockedAdd` from `wintt.h`.**]**



## InterlockedIFAnd

```c
long InterlockedIFAnd( 
    long volatile *Destination,
    long          Value
);
```

**SRS_INTERLOCKED_IF_43_002 [** `InterlockedIFAnd` shall call `InterlockedAnd` from `wintt.h`.**]**

## InterlockedIFAnd16

```c
short InterlockedIFAnd16(
  short volatile *Destination,
  short          Value
);
```

**SRS_INTERLOCKED_IF_43_003 [** `InterlockedIFAnd16` shall call `InterlockedAnd16` from `wintt.h`.**]**


## InterlockedIFAnd64

```c
int64_t InterlockedIFAnd64(
  int64_t volatile *Destination,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_004 [** `InterlockedIFAnd64` shall call `InterlockedAnd64` from `wintt.h`.**]**

## InterlockedIFAnd8

```c
char InterlockedIFAnd8(
  char volatile *Destination,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_005 [** `InterlockedIFAnd8` shall call `InterlockedAnd8` from `wintt.h`.**]**

## InterlockedIFCompareExchange

```c
long InterlockedIFCompareExchange(
  long volatile *Destination,
  long          ExChange,
  long          Comperand
);
```

**SRS_INTERLOCKED_IF_43_006 [** `InterlockedIFCompareExchange` shall call `InterlockedCompareExchange` from `wintt.h`.**]**

## InterlockedIFCompareExchange16

```c
short InterlockedIFCompareExchange16(
  short volatile *Destination,
  short          ExChange,
  short          Comperand
);
```

**SRS_INTERLOCKED_IF_43_007 [** `InterlockedIFCompareExchange16` shall call `InterlockedCompareExchange16` from `wintt.h`.**]**

## InterlockedIFCompareExchange64

```c
int64_t InterlockedIFCompareExchange64(
  int64_t volatile *Destination,
  int64_t          ExChange,
  int64_t          Comperand
);
```

**SRS_INTERLOCKED_IF_43_008 [** `InterlockedIFCompareExchange64` shall call `InterlockedCompareExchange64` from `wintt.h`.**]**

## InterlockedIFCompareExchangePointer

```c
void* InterlockedIFCompareExchangePointer(
  void* volatile *Destination,
  void*          Exchange,
  void*          Comperand
);
```

**SRS_INTERLOCKED_IF_43_09 [** `InterlockedIFCompareExchangePointer` shall call `InterlockedCompareExchangePointer` from `wintt.h`.**]**

## InterlockedIFDecrement

```c
long InterlockedIFDecrement(
  long volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_010[** `InterlockedIFDecrement` shall call `InterlockedDecrement` from `wintt.h`.**]**

## InterlockedIFDecrement16

```c
short InterlockedIFDecrement16(
  short volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_011 [** `InterlockedIFDecrement16` shall call `InterlockedDecrement16` from `wintt.h`.**]**


## InterlockedIFDecrement64

```c
int64_t InterlockedIFDecrement64(
  int64_t volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_012 [** `InterlockedIFDecrement64` shall call `InterlockedDecrement64` from `wintt.h`.**]**

## InterlockedIFExchange

```c
long InterlockedIFExchange(
  long volatile *Target,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_013 [** `InterlockedIFExchange` shall call `InterlockedExchange` from `wintt.h`.**]**

## InterlockedIFExchange16

```c
short InterlockedIFExchange16(
  short volatile *Destination,
  short          ExChange
);
```

**SRS_INTERLOCKED_IF_43_014 [** `InterlockedIFExchange16` shall call `InterlockedExchange16` from `wintt.h`.**]**


## InterlockedIFExchange64

```c
int64_t InterlockedIFExchange64(
  int64_t volatile *Target,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_015 [** `InterlockedIFExchange64` shall call `InterlockedExchange64` from `wintt.h`.**]**

## InterlockedIFExchange8

```c
char InterlockedIFExchange8(
  char volatile *Target,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_016 [** `InterlockedIFExchange8` shall call `InterlockedExchange8` from `wintt.h`.**]**

## InterlockedIFExchangeAdd

```c
long InterlockedIFExchangeAdd(
  long volatile *Addend,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_017 [** `InterlockedIFExchangeAdd` shall call `InterlockedExchangeAdd` from `wintt.h`.**]**

## InterlockedIFExchangeAdd64

```c
int64_t InterlockedIFExchangeAdd64(
  int64_t volatile *Addend,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_018 [** `InterlockedIFExchangeAdd64` shall call `InterlockedExchangeAdd64` from `wintt.h`.**]**

## InterlockedIFExchangePointer

```c
void* InterlockedIFExchangePointer(
  void* volatile *Target,
  void*          Value
);
```

**SRS_INTERLOCKED_IF_43_019 [** `InterlockedIFExchangePointer` shall call `InterlockedExchangePointer` from `wintt.h`.**]**

## InterlockedIFIncrement

```c
long InterlockedIFIncrement(
  long volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_020 [** `InterlockedIFIncrement` shall call `InterlockedIncrement` from `wintt.h`.**]**

## InterlockedIFIncrement16

```c
short InterlockedIFIncrement16(
  short volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_021 [** `InterlockedIFIncrement16` shall call `InterlockedIncrement16` from `wintt.h`.**]**

## InterlockedIFIncrement64

```c
int64_t InterlockedIFIncrement64(
  int64_t volatile *Addend
);
```

**SRS_INTERLOCKED_IF_43_022 [** `InterlockedIFIncrement64` shall call `InterlockedIncrement64` from `wintt.h`.**]**

## InterlockedIFOr

```c
long InterlockedIFOr(
  long volatile *Destination,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_023 [** `InterlockedIFOr` shall call `InterlockedOr` from `wintt.h`.**]**

## InterlockedIFOr16

```c
short InterlockedIFOr16(
  short volatile *Destination,
  short          Value
);
```

**SRS_INTERLOCKED_IF_43_024 [** `InterlockedIFOr16` shall call `InterlockedOr16` from `wintt.h`.**]**

## InterlockedIFOr64

```c
int64_t InterlockedIFOr64(
  int64_t volatile *Destination,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_025 [** `InterlockedIFOr64` shall call `InterlockedOr64` from `wintt.h`.**]**


## InterlockedIFOr8

```c
char InterlockedIFOr8(
  char volatile *Destination,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_026 [** `InterlockedIFOr8` shall call `InterlockedOr8` from `wintt.h`.**]**

## InterlockedIFXor

```c
long InterlockedIFXor(
  long volatile *Destination,
  long          Value
);
```

**SRS_INTERLOCKED_IF_43_027 [** `InterlockedIFXor` shall call `InterlockedXor` from `wintt.h`.**]**

## InterlockedIFXor16

```c
short InterlockedIFXor16(
  short volatile *Destination,
  short          Value
);
```

**SRS_INTERLOCKED_IF_43_028 [** `InterlockedIFXor16` shall call `InterlockedXor16` from `wintt.h`.**]**

## InterlockedIFXor64

```c
int64_t InterlockedIFXor64(
  int64_t volatile *Destination,
  int64_t          Value
);
```

**SRS_INTERLOCKED_IF_43_029 [** `InterlockedIFXor64` shall call `InterlockedXor64` from `wintt.h`.**]**

## InterlockedIFXor8

```c
char InterlockedIFXor8(
  char volatile *Destination,
  char          Value
);
```

**SRS_INTERLOCKED_IF_43_030 [** `InterlockedIFXor8` shall call `InterlockedXor8` from `wintt.h`.**]**






