#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(${run_unittests})
    add_subdirectory(buffer_ut)
    add_subdirectory(constbuffer_array_ut)
    add_subdirectory(constbuffer_array_batcher_ut)
    if(WIN32)
        add_subdirectory(logging_ut)
        add_subdirectory(srw_lock_ut)
        add_subdirectory(timer_win32_ut)
    endif()
endif()

if(${run_int_tests})
endif()
