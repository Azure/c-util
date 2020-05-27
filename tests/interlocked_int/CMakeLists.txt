#Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName interlocked_int)

set(${theseTestsName}_test_files
${theseTestsName}.c
    )

set(${theseTestsName}_h_files
../../inc/azure_c_util/interlocked.h
)

build_c_tests(${theseTestsName} ON "tests/azure_c_util_tests" ADDITIONAL_LIBS azure_c_util)
