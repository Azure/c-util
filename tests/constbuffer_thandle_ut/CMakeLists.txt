cmake_minimum_required(VERSION 3.18)

set(theseTestsName constbuffer_thandle_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/constbuffer_thandle.c
    ../../src/memory_data.c #don't want any mocks generated for memory_data so grab the real functions for the purpose of testing
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_pal c_pal_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/constbuffer_thandle_ut_pch.h"
)