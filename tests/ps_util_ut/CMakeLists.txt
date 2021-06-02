#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName ps_util_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ps_util_mocked.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/ps_util.h
)

build_test_artifacts(${theseTestsName} "tests/c_util")
