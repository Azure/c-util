#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName external_command_helper_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/external_command_helper.c
)

set(${theseTestsName}_h_files
../../inc/c_util/external_command_helper.h
)

build_test_artifacts(${theseTestsName} ON "tests/c_util" ADDITIONAL_LIBS c_pal_reals c_util_reals)
