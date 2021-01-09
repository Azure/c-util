#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

if(MSVC)
set(REAL_SM_C_FILES
    real_sm.c
)
set(REAL_SM_H_FILES
    real_sm.h
    real_sm_renames.h
)
endif()

set(c_util_reals_c_files
    real_constbuffer.c
    real_constbuffer_array.c
    real_constbuffer_array_batcher_nv.c
    real_doublylinkedlist.c
    real_interlocked_hl.c
    real_memory_data.c
    real_rc_string.c
    real_rc_string_array.c
    real_singlylinkedlist.c
    real_uuid.c
    ${REAL_SM_C_FILES}
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
    real_interlocked_hl.h
    real_interlocked_hl_renames.h
    real_memory_data.h
    real_memory_data_renames.h
    real_rc_string.h
    real_rc_string_renames.h
    real_rc_string_array.h
    real_rc_string_array_renames.h
    real_singlylinkedlist.h
    real_singlylinkedlist_renames.h
    real_uuid.h
    real_uuid_renames.h
    ${REAL_SM_H_FILES}
)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(c_util_reals ${c_util_reals_c_files} ${c_util_reals_h_files})
target_include_directories(c_util_reals PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests/reals>)
target_link_libraries(c_util_reals c_logging c_pal_reals)
