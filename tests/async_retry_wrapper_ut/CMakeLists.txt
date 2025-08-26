#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName async_retry_wrapper_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/async_retry_wrapper.c
    test_async.c
    test_async_retry_wrappers.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/async_retry_wrapper.h
    test_async.h
    test_async_retry_wrappers.h
    test_ref_counted.h
)

include_directories(../../inc)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_util c_pal_reals
    NO_MOCK_PRECOMPILE_HEADERS
        c_pal/interlocked.h
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
        "c_pal/threadapi.h"
        "c_pal/threadpool.h"
        "c_pal/thandle.h"
        "c_pal/timer.h"
        "c_pal/interlocked_hl.h"
        "c_pal/log_critical_and_terminate.h"
        "c_util/async_type_helper.h"
)
