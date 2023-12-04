#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName for_each_in_sub_folder_ut)


set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
for_each_in_sub_folder_mocked.c
)

set(${theseTestsName}_h_files
../../inc/for_each_in_sub_folder.h
)

build_test_artifacts(${theseTestsName} "tests/block_storage_2_sf" ADDITIONAL_LIBS c_util block_storage_2 c_pal_reals)
