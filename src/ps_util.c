// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <stdbool.h>

#include "c_util/ps_util.h"

void ps_util_terminate_process(bool abort_process)
{
    if (abort_process)
    {
        /* Codes_SRS_PS_UTIL_01_001: [ ps_util_terminate_process shall call abort. ]*/
        abort();
    }
    else
    {
        exit(42);
    }
}
