#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName external_command_helper_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/external_command_helper.c
)

set(${theseTestsName}_h_files
../../inc/c_util/external_command_helper.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_pal_reals c_util_reals
    NO_MOCK_PRECOMPILE_HEADERS
        "c_logging/logger.h"
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
        "c_pal/pipe.h"
        "c_util/rc_string.h"
        "c_util/rc_string_array.h"
)
