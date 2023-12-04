#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName bs_watchdog_int)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
../../inc/bs_watchdog.h
)

build_test_artifacts(${theseTestsName} "tests/block_storage_2" ADDITIONAL_LIBS bs_utils c_util)
