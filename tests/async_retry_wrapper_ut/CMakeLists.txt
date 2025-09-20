#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName async_retry_wrapper_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/async_retry_wrapper.c
    test_async.c
    test_async_retry_wrappers.c
    real_test_ref_counted.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/async_retry_wrapper.h
    test_async.h
    test_async_retry_wrappers.h
    test_ref_counted.h
    real_test_ref_counted.h
    real_test_ref_counted_renames.h
)

include_directories(../../inc)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_util c_pal_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/async_retry_wrapper_ut_pch.h"
)