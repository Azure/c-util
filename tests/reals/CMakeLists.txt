#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(azure_c_util_reals_c_files
    real_constbuffer.c
    real_constbuffer_array.c
    real_constbuffer_array_batcher_nv.c
    real_crt_abstractions.c
    real_doublylinkedlist.c
    real_memory_data.c
    real_singlylinkedlist.c
    real_uuid.c
)

set(azure_c_util_reals_h_files
    real_constbuffer.h
    real_constbuffer_renames.h
    real_constbuffer_array.h
    real_constbuffer_array_renames.h
    real_constbuffer_array_batcher_nv.h
    real_constbuffer_array_batcher_nv_renames.h
    real_crt_abstractions.h
    real_crt_abstractions_renames.h
    real_doublylinkedlist.h
    real_doublylinkedlist_renames.h
    real_memory_data.h
    real_memory_data_renames.h
    real_singlylinkedlist.h
    real_uuid.h
    real_uuid_renames.h
)

if(MSVC)
    set(azure_c_util_reals_c_files real_sm.c)
    set(azure_c_util_reals_h_files real_sm.h real_sm_renames.h)
endif()

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../src)
add_library(azure_c_util_reals ${azure_c_util_reals_c_files} ${azure_c_util_reals_h_files})
target_include_directories(azure_c_util_reals PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/tests/reals>)
target_link_libraries(azure_c_util_reals azure_c_logging azure_c_pal azure_c_pal_reals)
