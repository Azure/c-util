#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName channel_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/channel.c
)

set(${theseTestsName}_h_files
../../inc/c_util/channel.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_util c_pal c_util_reals c_pal_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/channel_ut_pch.h"
)