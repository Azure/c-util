#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName async_type_helper_ref_counted_handler_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    test_ref_counted_async_type_helper_handler.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/async_type_helper_ref_counted_handler.h
    test_ref_counted_async_type_helper_handler.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_util c_pal_reals
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/async_type_helper_ref_counted_handler_ut_pch.h"
)