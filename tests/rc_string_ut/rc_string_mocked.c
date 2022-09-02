// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "stddef.h"

#include "../../src/rc_string.c"

#undef vsnprintf

#define vsnprintf mocked_vsnprintf

int mocked_vsnprintf(char* const _Buffer, size_t const _BufferCount, char const* const _Format, va_list _ArgList);

