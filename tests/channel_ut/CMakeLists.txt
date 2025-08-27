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
    NO_MOCK_PRECOMPILE_HEADERS
        "c_pal/execution_engine.h"
        "c_pal/interlocked.h"
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
        "c_pal/interlocked_hl.h"
        "c_pal/srw_lock.h"
        "c_pal/threadpool.h"
        "c_pal/thandle_log_context_handle.h"
        "c_pal/sm.h"
        "c_pal/log_critical_and_terminate.h"
        "c_util/doublylinkedlist.h"
        "c_util/async_op.h"
        "c_util/rc_ptr.h"
        "c_util/rc_string.h"
)
