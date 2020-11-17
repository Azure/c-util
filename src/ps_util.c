// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>

#include "c_util/ps_util.h"

void ps_util_terminate_process(void)
{
    /* Codes_SRS_PS_UTIL_01_001: [ `ps_util_terminate_process` shall call `abort`. ]*/
    abort();
}
