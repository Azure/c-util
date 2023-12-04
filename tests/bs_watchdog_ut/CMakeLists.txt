#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName bs_watchdog_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/bs_watchdog.c
)

set(${theseTestsName}_h_files
../../inc/bs_watchdog.h
)

build_test_artifacts(${theseTestsName} "tests/block_storage_2" ADDITIONAL_LIBS c_pal_reals c_util_reals)
