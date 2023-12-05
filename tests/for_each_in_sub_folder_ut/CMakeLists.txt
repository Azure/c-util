#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName for_each_in_sub_folder_ut)


set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
for_each_in_sub_folder_mocked.c
)

set(${theseTestsName}_h_files
../../inc/c_util/for_each_in_sub_folder.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal_reals c_util_reals)
