#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName rc_string_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    rc_string_mocked.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/rc_string.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal c_pal_reals)
