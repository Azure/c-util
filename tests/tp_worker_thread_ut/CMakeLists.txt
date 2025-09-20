#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName tp_worker_thread_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/tp_worker_thread.c
)

set(${theseTestsName}_h_files
../../inc/c_util/tp_worker_thread.h
)

build_test_artifacts(${theseTestsName} "tests/c_util"
    ADDITIONAL_LIBS c_util_reals c_util_test_helpers c_pal_reals c_pal_umocktypes

    ENABLE_TEST_FILES_PRECOMPILED_HEADERS "${CMAKE_CURRENT_LIST_DIR}/tp_worker_thread_ut_pch.h"
)