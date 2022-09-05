// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#define vsnprintf mocked_vsnprintf

extern int mocked_vsnprintf(char* s, size_t n, const char* format, ...);

#include "../../src/rc_string.c"
