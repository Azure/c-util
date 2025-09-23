#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName filename_helper_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/filename_helper.c
)

set(${theseTestsName}_h_files
../../inc/c_util/filename_helper.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/filename_helper_ut_pch.h"
)