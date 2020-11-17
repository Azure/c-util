// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>

#include "c_util/log_critical_and_terminate.h"

void log_critical_terminate_process(void)
{
    abort();
}
