// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef INTERLOCKED_IF_H
#define INTERLOCKED_IF_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif


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

#endif