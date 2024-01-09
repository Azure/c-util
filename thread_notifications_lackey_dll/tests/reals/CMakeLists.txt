#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(thread_notifications_lackey_dll_reals_c_files
    nothing.c
)

set(thread_notifications_lackey_dll_reals_h_files
)

if(WIN32)
    set(thread_notifications_lackey_dll_reals_c_files
        ${thread_notifications_lackey_dll_reals_c_files}
        real_thread_notifications_lackey_dll.c
    )

    set(thread_notifications_lackey_dll_reals_h_files
        ${thread_notifications_lackey_dll_reals_h_files}
        real_thread_notifications_lackey_dll.h
        real_thread_notifications_lackey_dll_renames.h
    )

    set(thread_notifications_lackey_dll_reals_def_files
        ${thread_notifications_lackey_dll_reals_def_files}
        real_thread_notifications_lackey_dll.def
    )
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(thread_notifications_lackey_dll_reals SHARED ${thread_notifications_lackey_dll_reals_c_files} ${thread_notifications_lackey_dll_reals_h_files})
target_include_directories(thread_notifications_lackey_dll_reals PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests/reals> $<TARGET_PROPERTY:thread_notifications_lackey_dll,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(thread_notifications_lackey_dll_reals c_logging_v2 c_pal_reals)
