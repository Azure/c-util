#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(c_util_reals_c_files
    real_constbuffer.c
    real_constbuffer_array.c
    real_constbuffer_array_batcher_nv.c
    real_doublylinkedlist.c
    real_external_command_helper.c    
    real_memory_data.c
    real_rc_string.c
    real_rc_string_array.c
    real_singlylinkedlist.c   
    real_uuid_string.c
)

set(c_util_reals_h_files
    real_constbuffer.h
    real_constbuffer_renames.h
    real_constbuffer_array.h
    real_constbuffer_array_renames.h
    real_constbuffer_array_batcher_nv.h
    real_constbuffer_array_batcher_nv_renames.h
    real_doublylinkedlist.h
    real_doublylinkedlist_renames.h
    real_external_command_helper.h
    real_external_command_helper_renames.h 
    real_memory_data.h
    real_memory_data_renames.h 
    real_rc_string.h
    real_rc_string_renames.h
    real_rc_string_array.h
    real_rc_string_array_renames.h
    real_singlylinkedlist.h
    real_singlylinkedlist_renames.h
    real_uuid_string.h
    real_uuid_string_renames.h
)

if(WIN32)
    #add worker_thread only on Windows (until we fix it)
    set(c_util_reals_c_files
        ${c_util_reals_c_files}
        real_object_lifetime_tracker.c
        real_worker_thread.c
    )

    set(c_util_reals_h_files
        ${c_util_reals_h_files}
        real_object_lifetime_tracker_renames.h
        real_object_lifetime_tracker.h
        real_worker_thread.h
        real_worker_thread_renames.h
    )
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(c_util_reals ${c_util_reals_c_files} ${c_util_reals_h_files})
target_include_directories(c_util_reals PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests/reals>)
target_link_libraries(c_util_reals c_logging c_pal_reals)
