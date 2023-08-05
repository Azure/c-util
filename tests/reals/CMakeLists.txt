#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(c_util_reals_c_files
    real_async_op.c
    real_channel.c
    real_channel_internal.c
    real_constbuffer.c
    real_constbuffer_array.c
    real_constbuffer_array_batcher_nv.c
    real_doublylinkedlist.c
    real_external_command_helper.c
    real_memory_data.c
    real_rc_ptr.c
    real_rc_string.c
    real_rc_string_array.c
    real_singlylinkedlist.c   
    real_uuid_string.c
)

set(c_util_reals_cpp_files
    real_hash.cpp
    real_murmurhash2.cpp
)

set(c_util_reals_h_files
    real_async_op.h
    real_async_op_renames.h
    real_channel_common_renames.h
    real_channel.h
    real_channel_renames.h
    real_channel_internal.h
    real_channel_internal_renames.h
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
    real_hash.h
    real_hash_renames.h
    real_memory_data.h
    real_memory_data_renames.h 
    real_murmurhash2.h
    real_murmurhash2_renames.h
    real_rc_ptr.h
    real_rc_ptr_renames.h
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

get_murmurhash2_directory(MURMURHASH2_DIR)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(c_util_reals ${c_util_reals_c_files} ${c_util_reals_cpp_files} ${c_util_reals_h_files})
target_include_directories(c_util_reals PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests/reals> ${MURMURHASH2_DIR} $<TARGET_PROPERTY:c_util,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(c_util_reals c_logging_v2 c_pal_reals)
