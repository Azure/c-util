#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName queue_processor_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/queue_processor.c
)

set(${theseTestsName}_h_files
../../inc/c_util/queue_processor.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util_reals c_pal c_pal_reals clds clds_reals)
