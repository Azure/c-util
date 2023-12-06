#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName bs_watchdog_threadpool_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/bs_watchdog_threadpool.c
)

set(${theseTestsName}_h_files
../../inc/c_util/bs_watchdog_threadpool.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal_reals c_util_reals)
