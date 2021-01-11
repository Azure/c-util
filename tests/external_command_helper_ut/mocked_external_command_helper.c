// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>

#if _WIN32
#define _popen mocked_popen
#define _pclose mocked_pclose
#else
#define popen mocked_popen
#define pclose mocked_pclose
#endif

int mocked_pclose(FILE* stream);

FILE* mocked_popen(
    const char* command,
    const char* mode);

#include "../../src/external_command_helper.c"
