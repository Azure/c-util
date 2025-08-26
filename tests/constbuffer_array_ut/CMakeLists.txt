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
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_pal c_pal_reals c_util_reals
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
        "c_pal/interlocked.h"
        "c_util/constbuffer.h"
)
