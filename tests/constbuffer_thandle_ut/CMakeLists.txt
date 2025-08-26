cmake_minimum_required(VERSION 3.18)

set(theseTestsName constbuffer_thandle_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/constbuffer_thandle.c
    ../../src/memory_data.c #don't want any mocks generated for memory_data so grab the real functions for the purpose of testing
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_pal c_pal_reals
    NO_MOCK_PRECOMPILE_HEADERS
        "c_logging/logger.h"
        "c_pal/thandle.h"
    MOCK_PRECOMPILE_HEADERS
        "c_util/buffer_.h"
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
)
