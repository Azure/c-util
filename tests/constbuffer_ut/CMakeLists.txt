#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName constbuffer_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/constbuffer.c
../../src/memory_data.c #don't want any mocks generated for memory_data so grab the real functions for the purpose of testing
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_pal c_pal_reals
    MOCK_PRECOMPILE_HEADERS
        "c_util/buffer_.h"
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
)
