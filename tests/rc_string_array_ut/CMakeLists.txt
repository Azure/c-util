#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

compileAsC11()
set(theseTestsName rc_string_array_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/rc_string_array.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/rc_string_array.h
)

build_test_artifacts(${theseTestsName} ON "tests/block_storage_2" ADDITIONAL_LIBS c_pal c_util_reals c_pal_reals)
