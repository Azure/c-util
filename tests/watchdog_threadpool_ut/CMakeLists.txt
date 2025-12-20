#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName watchdog_threadpool_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/watchdog_threadpool.c
)

set(${theseTestsName}_h_files
../../inc/c_util/watchdog_threadpool.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal_reals c_util_reals 
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/watchdog_threadpool_ut_pch.h"
)