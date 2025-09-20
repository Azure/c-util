#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName rc_ptr_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/rc_ptr.c
)

set(${theseTestsName}_h_files
../../inc/c_util/rc_ptr.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_util_reals c_pal_reals c_pal
    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/rc_ptr_ut_pch.h"
)