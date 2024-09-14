#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName constbuffer_array_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/constbuffer_array.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/constbuffer_array.h
    my_mocks.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal c_pal_reals c_util_reals)

target_compile_options(${theseTestsName}_lib_c_util PRIVATE /wd4505)

target_precompile_headers(${theseTestsName}_lib_c_util PUBLIC
    <stdlib.h>
    <stddef.h>
    [["macro_utils/macro_utils.h"]]
    [["windows.h"]]
)

