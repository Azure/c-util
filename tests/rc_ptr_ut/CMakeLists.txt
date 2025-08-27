#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName rc_ptr_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/rc_ptr.c
)

set(${theseTestsName}_h_files
../../inc/c_util/rc_ptr.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_util_reals c_pal_reals c_pal
    NO_MOCK_PRECOMPILE_HEADERS
        "c_pal/interlocked.h"
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
)
