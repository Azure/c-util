#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName batch_queue_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/batch_queue.c
batch_queue_ut_reals.c
)

set(${theseTestsName}_h_files
../../inc/batch_queue.h
)

build_test_artifacts(${theseTestsName} "tests/elastic_log_common"
    ADDITIONAL_LIBS c_pal_reals c_util_reals c_pal_umocktypes clds_reals elastic_log_test_helpers
    MOCK_PRECOMPILE_HEADERS
        "c_logging/logger.h"
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
        "c_pal/execution_engine.h"
        "c_pal/interlocked.h"
        "c_pal/log_critical_and_terminate.h"
        "c_pal/malloc_multi_flex.h"
        "c_pal/sm.h"
        "c_pal/thandle.h"
        "c_pal/threadpool.h"
        "c_pal/timer.h"
        "c_pal/thandle.h"
        "clds/mpsc_lock_free_queue.h"
        "tp_worker_thread.h"
        "c_util/tarray.h"
        "c_util/tarray_ll.h"
        "batch_queue_tarray_types.h"
)
