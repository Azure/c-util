// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <string.h>

#define vsnprintf proxy_mocked_vsnprintf
#define strlen mocked_strlen

int proxy_mocked_vsnprintf(char * restrict s, size_t n, const char * restrict format, va_list arg);
size_t mocked_strlen(const char* s);

#include "../../src/rc_string.c"
