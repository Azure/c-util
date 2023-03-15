#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName object_lifetime_tracker_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/object_lifetime_tracker.c
)

set(${theseTestsName}_h_files
../../inc/c_util/object_lifetime_tracker.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util_reals c_pal_reals)
