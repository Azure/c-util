// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_CRT_ABSTRACTIONS_H
#define REAL_CRT_ABSTRACTIONS_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "azure_macro_utils/macro_utils.h"
#include "azure_c_util/crt_abstractions.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CRT_ABSTRACTIONS_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        mallocAndStrcpy_s, \
        unsignedIntToString, \
        size_tToString, \
        strtoull_s, \
        strtof_s, \
        strtold_s \
    )

#ifdef __cplusplus
extern "C" {
#endif

int real_mallocAndStrcpy_s(char** destination, const char* source);
int real_unsignedIntToString(char* destination, size_t destinationSize, unsigned int value);
int real_size_tToString(char* destination, size_t destinationSize, size_t value);
unsigned long long real_strtoull_s(const char* nptr, char** endptr, int base);
float real_strtof_s(const char* nptr, char** endPtr);
long double real_strtold_s(const char* nptr, char** endPtr);

#ifdef __cplusplus
}
#endif

#endif // REAL_CRT_ABSTRACTIONS_H
