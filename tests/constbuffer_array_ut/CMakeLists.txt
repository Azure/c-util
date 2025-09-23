#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName constbuffer_array_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/constbuffer_array.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/constbuffer_array.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_pal c_pal_reals c_util_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/constbuffer_array_ut_pch.h"
)