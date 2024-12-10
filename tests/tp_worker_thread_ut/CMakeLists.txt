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
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"

        "c_pal/execution_engine.h"
        "c_pal/interlocked.h"
        "c_pal/log_critical_and_terminate.h"
        "c_pal/sm.h"
        "c_pal/threadpool.h"
)
