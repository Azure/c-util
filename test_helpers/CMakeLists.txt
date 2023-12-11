#Copyright (C) Microsoft Corporation. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(c_util_test_helpers_h_files
    inc/c_util_test_helpers/rc_string_test_type.h
    inc/c_util_test_helpers/rc_string_test_decl.h
)

set(c_util_test_helpers_c_files
    src/rc_string_test_type.c
)

if(WIN32)
set(c_util_test_helpers_h_files
    ${c_util_test_helpers_h_files}
    inc/c_util_test_helpers/c_util_test_env_params.h
    inc/c_util_test_helpers/guid_as_string.h
    inc/c_util_test_helpers/leak_log_checker.h
    inc/c_util_test_helpers/leak_log_folder.h
)

set(c_util_test_helpers_c_files
    ${c_util_test_helpers_c_files}
    src/c_util_test_env_params.c
    src/guid_as_string.c
    src/leak_log_checker.c
    src/leak_log_folder.c
)
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)

add_library(c_util_test_helpers ${c_util_test_helpers_h_files} ${c_util_test_helpers_c_files})
target_link_libraries(c_util_test_helpers c_pal c_util_reals)
target_include_directories(c_util_test_helpers PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
