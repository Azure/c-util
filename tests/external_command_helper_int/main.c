// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stddef.h>
#include "testrunnerswitcher.h"

int main(void)
{
    size_t failedTestCount = 0;
    RUN_TEST_SUITE(external_command_helper_int_tests, failedTestCount);
    return failedTestCount;
}
