#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName async_type_helper_copy_value_handler_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
    ../../inc/c_util/async_type_helper_copy_value_handler.h
)

build_test_artifacts(${theseTestsName} "tests/block_storage_2" ADDITIONAL_LIBS c_util c_pal_reals)
