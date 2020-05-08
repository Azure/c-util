#ifndef INTERLOCKEDIFIF_H
#define INTERLOCKEDIFIF_H

#include <stdint.h>

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

#endif


