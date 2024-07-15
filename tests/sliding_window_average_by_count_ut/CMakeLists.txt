#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName sliding_window_average_by_count_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/sliding_window_average_by_count.c
)

set(${theseTestsName}_h_files
../../inc/c_util/sliding_window_average_by_count.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal c_pal_reals c_pal_umocktypes)
