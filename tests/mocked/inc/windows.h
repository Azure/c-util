// Copyright(C) Microsoft Corporation.All rights reserved.

#ifndef MOCKED_WINDOWS_H
#define MOCKED_WINDOWS_H

#include <stdint.h>

typedef void *HANDLE; /*works for most of the cases, except for those sneaky handles returned by CreateFile for which INVALID_HANDLE_VALUE is "the NULL"*/

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

typedef void* LONG_PTR; /*approximatively*/

typedef void* PTP_IO;

typedef int64_t LONGLONG;
typedef int64_t LONG64;
typedef int32_t LONG;
typedef unsigned long ULONG;
typedef uint64_t ULONG64;
typedef LONG HRESULT;
typedef unsigned long DWORD;
typedef void VOID;
typedef void* PVOID;
typedef size_t SIZE_T;
typedef long BOOL;

#define FALSE 0
#define TRUE 1
#define INFINITE 0xFFFFFFFF

#ifdef __cplusplus
extern "C" {
#endif

    LONGLONG InterlockedAdd64(
        LONGLONG volatile  * Addend,
        LONGLONG          Value
    );

    LONGLONG InterlockedCompareExchange64(
       LONG64 volatile *Destination,
       LONG64          Exchange,
       LONG64          Comparand
    );

    BOOL WaitOnAddress(
        volatile VOID * Address,
        PVOID CompareAddress,
        SIZE_T AddressSize,
        DWORD dwMilliseconds
        );

    LONG InterlockedExchange(
        LONG volatile* Target,
        LONG          Value
    );

    void WakeByAddressSingle(
        PVOID Address
    );

    LONG InterlockedAdd(
        LONG volatile *Addend,
        LONG          Value
    );

#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800

#define FormatMessage(...) 0
#define GetLastError() 0

#ifdef __cplusplus
}
#endif

#endif/*MOCKED_WINDOWS_H*/
