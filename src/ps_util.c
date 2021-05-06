// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>

#include "c_util/ps_util.h"

void ps_util_terminate_process(void)
{
    /* Codes_SRS_PS_UTIL_01_001: [ ps_util_terminate_process shall call abort. ]*/
    abort();
}

void ps_util_exit_process(int exit_code)
{
    /* Codes_SRS_PS_UTIL_01_002: [ ps_util_exit_process shall call exit, passing exit_code as argument. ]*/
    exit(exit_code);
}
