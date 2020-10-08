#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName constbuffer_array_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/constbuffer_array.c
)

set(${theseTestsName}_h_files
    ../../inc/azure_c_util/constbuffer_array.h
)

build_test_artifacts(${theseTestsName} ON "tests/azure_c_util" ADDITIONAL_LIBS azure_c_pal azure_c_pal_reals azure_c_util_reals)
