#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName azure_c_util_reals_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

build_c_tests(${theseTestsName} ON "tests/azure_c_util_tests" ADDITIONAL_LIBS azure_c_util azure_c_util_reals azure_c_pal)
