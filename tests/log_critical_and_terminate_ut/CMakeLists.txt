#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName log_critical_and_terminate_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
    ../../inc/c_util/log_critical_and_terminate.h
)

build_test_artifacts(${theseTestsName} ON "tests/c_util")
