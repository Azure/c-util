#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName sm_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/sm.c
)

set(${theseTestsName}_h_files
../../inc/c_util/sm.h
)

build_test_artifacts(${theseTestsName} ON "tests/c_util" ADDITIONAL_LIBS synchronization c_pal c_pal_reals c_util_reals)

