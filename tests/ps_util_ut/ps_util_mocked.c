// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

void mock_abort(void);
void mock_exit(int exit_code);

#define abort mock_abort
#define exit mock_exit

#include "../../src/ps_util.c"
