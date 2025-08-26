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

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_util  c_pal_reals c_util_reals
    NO_MOCK_PRECOMPILE_HEADERS
        c_pal/interlocked.h
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
        "c_util/constbuffer.h"
        "c_util/constbuffer_array.h"
)
