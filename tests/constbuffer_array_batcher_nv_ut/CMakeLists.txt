#Copyright (c) Microsoft. All rights reserved.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName constbuffer_array_batcher_nv_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/constbuffer_array_batcher_nv.c
    ../reals/real_constbuffer.c
    ../reals/real_constbuffer_array.c
    ../reals/real_memory_data.c
)

set(${theseTestsName}_h_files
    ../../inc/azure_c_util/constbuffer_array_batcher_nv.h
    ../reals/real_constbuffer.h
    ../reals/real_constbuffer_renames.h
    ../reals/real_constbuffer_array.h
    ../reals/real_constbuffer_array_renames.h
    ../reals/real_memory_data.h
    ../reals/real_memory_data_renames.h
)

build_c_tests(${theseTestsName} ON "tests/azure_c_util_tests")
