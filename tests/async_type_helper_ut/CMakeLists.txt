#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName async_type_helper_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/async_type_helper.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/async_type_helper.h
)

include_directories(../../../deps/azure-zrpc/pal/interfaces/inc)

build_test_artifacts(${theseTestsName} "tests/block_storage_2" ADDITIONAL_LIBS c_util synchronization c_pal_reals c_util_reals)
