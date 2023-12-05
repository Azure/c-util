#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName bs_filename_helper_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/bs_filename_helper.c
)

set(${theseTestsName}_h_files
../../inc/c_util/bs_filename_helper.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal_reals)
