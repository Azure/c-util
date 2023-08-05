#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName channel_internal_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/channel_internal.c
)

set(${theseTestsName}_h_files
../../inc/c_util/channel_internal.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal c_util_reals c_pal_reals)
