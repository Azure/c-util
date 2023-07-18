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

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util_reals c_pal_reals c_pal)
