#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName sync_wrapper_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    test_async.c
    test_sync_wrappers.c
    test_ref_counted.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/sync_wrapper.h
    test_async.h
    test_sync_wrappers.h
    test_ref_counted.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal_reals c_util_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/sync_wrapper_ut_pch.h"
)