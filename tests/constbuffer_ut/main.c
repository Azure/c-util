// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stddef.h>
#include "testrunnerswitcher.h"
#include <stdlib.h>
#include <stdio.h>

#include "vld.h"

#include "azure_c_pal/gballoc_hl.h"
#include "azure_c_pal/gballoc_hl_redirect.h"

#include "real_gballoc_hl.h"

#include "mimalloc.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(constbuffer_unittests, failedTestCount);
    return (int)failedTestCount;
}
