#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

if(NOT DEFINED CMAKE_MINIMUM_REQUIRED_VERSION)
    cmake_minimum_required(VERSION 3.18)
endif()

# canon way of using c-util from another repo is below. It assumes the using repo has placed c-util in "deps"

#if ((NOT TARGET c_util) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-util/CMakeLists.txt))
#    add_subdirectory(deps/c-util)
#    include_directories(${C_UTIL_INC_FOLDER})
#endif()

if (TARGET c_util)
    RETURN()
endif()

project(c_util)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF). Chsare dutility does not have any e2e tests, but the option needs to exist to evaluate in IF statements" OFF)
option(run_int_tests "set run_int_tests to ON to integration tests (default is OFF)." OFF)
option(run_perf_tests "set run_perf_tests to ON to build performance tests (default is OFF)." OFF)
option(run_reals_check "set run_reals_check to ON to run reals check (default is OFF)." OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is OFF)" OFF)
option(run_traceability "run traceability tool (default is ON)" ON)

set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_unittests ${run_unittests})
set(original_run_int_tests ${run_int_tests})
set(original_run_traceability ${run_traceability})
set(original_run_perf_tests ${run_perf_tests})
set(original_run_reals_check ${run_reals_check})

set(run_e2e_tests OFF)
set(run_unittests OFF)
set(run_int_tests OFF)
set(run_traceability OFF)
set(run_perf_tests OFF)
set(run_reals_check OFF)

if ((NOT TARGET c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-build-tools/CMakeLists.txt))
    add_subdirectory(deps/c-build-tools)
    set_default_build_options()
endif()

if ((WIN32) AND ("${GBALLOC_LL_TYPE}" STREQUAL "JEMALLOC"))
    # Bring in vcpkg
    use_vcpkg(${CMAKE_CURRENT_LIST_DIR}/deps/vcpkg)
endif()

if ((NOT TARGET macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/macro-utils-c/CMakeLists.txt))
    add_subdirectory(deps/macro-utils-c)
    include_directories(${MACRO_UTILS_INC_FOLDER})
endif()

if ((NOT TARGET c_logging) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-logging/CMakeLists.txt))
    add_subdirectory(deps/c-logging)
    include_directories(deps/c-logging/v2/inc)
endif()

if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/ctest/CMakeLists.txt))
    add_subdirectory(deps/ctest)
    include_directories(${CTEST_INC_FOLDER})
endif()

if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-testrunnerswitcher/CMakeLists.txt))
    add_subdirectory(deps/c-testrunnerswitcher)
    include_directories(${TESTRUNNERSWITCHER_INC_FOLDER})
endif()

if ((NOT TARGET umock_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/umock-c/CMakeLists.txt))
    add_subdirectory(deps/umock-c)
    include_directories(${UMOCK_C_INC_FOLDER})
endif()

if ((NOT TARGET c_pal) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/c-pal/CMakeLists.txt))
    add_subdirectory(deps/c-pal)
    include_directories(${C_PAL_INC_FOLDER})
endif()

if (
    (NOT TARGET mimalloc-obj) AND
    (${GBALLOC_LL_TYPE} STREQUAL "MIMALLOC") AND
    (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/mimalloc/CMakeLists.txt)
    )
        set(MI_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE) #not building a dll allows building on 32 bit, otherwise there's some errors on init.c about not finding a imported symbol
        set(MI_BUILD_TESTS OFF CACHE BOOL "Build test executables" FORCE)
        #for mimalloc disable this warning: Warning C4459: declaration of 'os_page_size' hides global declaration
        #for mimalloc disable this warning: Warning C4100: 'try_alignment': unreferenced formal parameter
        #for mimalloc disable this warning: warning C4505: 'mi_os_get_aligned_hint': unreferenced local function has been removed

        set(PREV_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
        set(PREV_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        if(WIN32)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4459 /wd4100 /wd4505")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4459 /wd4100 /wd4505")
        endif()

        add_subdirectory(deps/mimalloc)
        include_directories(deps/mimalloc/include)

        set(CMAKE_C_FLAGS ${PREV_CMAKE_C_FLAGS})
        set(CMAKE_CXX_FLAGS ${PREV_CMAKE_CXX_FLAGS})
endif()

if (
    (WIN32) AND
    (NOT TARGET jemalloc) AND
    (${GBALLOC_LL_TYPE} STREQUAL "JEMALLOC") AND
    (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/jemalloc/README)
    )
    set(PREV_CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
    set(PREV_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    set(PREV_CMAKE_STATIC_LINKER_FLAGS ${CMAKE_STATIC_LINKER_FLAGS})

    if(WIN32)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4100 /wd4127 /wd4459 /wd4201 /wd4456 /wd4457 /wd4702 /wd4244 /wd4701 /wd4706 /wd4703 /wd4189 /wd4267 /wd4204 /wd4565 /wd4310 /wd4334 /wd4028 /DJEMALLOC_NO_PRIVATE_NAMESPACE /D_REENTRANT /DJEMALLOC_EXPORT= /D_LIB")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100 /wd4127 /wd4459 /wd4201 /wd4456 /wd4457 /wd4702 /wd4244 /wd4701 /wd4706 /wd4703 /wd4189 /wd4267 /wd4204 /wd4565 /wd4310 /wd4334 /wd4028 /DJEMALLOC_NO_PRIVATE_NAMESPACE /D_REENTRANT /DJEMALLOC_EXPORT= /D_LIB")
        set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
    endif()

    add_subdirectory(jemalloc_build)

    set(CMAKE_C_FLAGS ${PREV_CMAKE_C_FLAGS})
    set(CMAKE_CXX_FLAGS ${PREV_CMAKE_CXX_FLAGS})
    set(CMAKE_STATIC_LINKER_FLAGS ${PREV_CMAKE_STATIC_LINKER_FLAGS})

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /DJEMALLOC_NO_PRIVATE_NAMESPACE /D_REENTRANT /DJEMALLOC_EXPORT= /D_LIB")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DJEMALLOC_NO_PRIVATE_NAMESPACE /D_REENTRANT /DJEMALLOC_EXPORT= /D_LIB")

endif()

set(run_e2e_tests ${original_run_e2e_tests})
set(run_unittests ${original_run_unittests})
set(run_int_tests ${original_run_int_tests})
set(run_traceability ${original_run_traceability})
set(run_perf_tests ${original_run_perf_tests})
set(run_reals_check ${original_run_reals_check})

include(CTest)
enable_testing()

set(C_UTIL_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using sharedLib lib" FORCE)

include_directories(${UMOCK_C_INC_FOLDER})

set(c_util_c_files
    ./src/async_op.c
    ./src/azure_base64.c
    ./src/filename_helper.c
    ./src/buffer.c
    ./src/cancellation_token.c
    ./src/channel_common.c
    ./src/channel.c
    ./src/channel_internal.c
    ./src/constbuffer.c
    ./src/constbuffer_array.c
    ./src/constbuffer_array_batcher_nv.c
    ./src/constbuffer_array_splitter.c
    ./src/constbuffer_array_tarray.c
    ./src/doublylinkedlist.c
    ./src/external_command_helper.c
    ./src/map.c
    ./src/memory_data.c
    ./src/object_lifetime_tracker.c
    ./src/rc_ptr.c
    ./src/rc_string.c
    ./src/rc_string_array.c
    ./src/rc_string_utils.c
    ./src/sliding_window_average_by_count.c
    ./src/singlylinkedlist.c
    ./src/strings.c
    ./src/tarray.c
    ./src/tcall_dispatcher.c
    ./src/tcall_dispatcher_cancellation_token_cancel_call.c
    ./src/tqueue.c
    ./src/uuid_string.c
    ./src/watchdog.c
    ./src/watchdog_threadpool.c
    ./src/worker_thread.c
)

set(c_util_cpp_files
    ./src/hash.cpp
)

set(c_util_h_files
    ./inc/c_util/async_op.h
    ./inc/c_util/azure_base64.h
    ./inc/c_util/filename_helper.h
    ./inc/c_util/buffer_.h
    ./inc/c_util/cancellation_token.h
    ./inc/c_util/channel.h
    ./inc/c_util/channel_common.h
    ./inc/c_util/constbuffer.h
    ./inc/c_util/constbuffer_format.h
    ./inc/c_util/constbuffer_version.h
    ./inc/c_util/constbuffer_array.h
    ./inc/c_util/constbuffer_array_batcher_nv.h
    ./inc/c_util/constbuffer_array_splitter.h
    ./inc/c_util/constbuffer_array_tarray.h
    ./inc/c_util/doublylinkedlist.h
    ./inc/c_util/external_command_helper.h
    ./inc/c_util/flags_to_string.h
    ./inc/c_util/hash.h
    ./inc/c_util/map.h
    ./inc/c_util/memory_data.h
    ./inc/c_util/object_lifetime_tracker.h
    ./inc/c_util/rc_ptr.h
    ./inc/c_util/rc_string.h
    ./inc/c_util/rc_string_array.h
    ./inc/c_util/rc_string_utils.h
    ./inc/c_util/sliding_window_average_by_count.h
    ./inc/c_util/singlylinkedlist.h
    ./inc/c_util/strings.h
    ./inc/c_util/strings_types.h
    ./inc/c_util/tarray_ll.h
    ./inc/c_util/tarray.h
    ./inc/c_util/tcall_dispatcher.h
    ./inc/c_util/tcall_dispatcher_ll.h
    ./inc/c_util/tcall_dispatcher_cancellation_token_cancel_call.h
    ./inc/c_util/tqueue.h
    ./inc/c_util/tqueue_ll.h
    ./inc/c_util/two_d_array.h
    ./inc/c_util/two_d_array_ll.h
    ./inc/c_util/thandle_tuple_array.h
    ./inc/c_util/uuid_string.h
    ./inc/c_util/watchdog.h
    ./inc/c_util/watchdog_threadpool.h
    ./inc/c_util/worker_thread.h
)

if (WIN32)
    set(c_util_h_files
        ${c_util_h_files}
        ./inc/c_util/for_each_in_folder.h
        ./inc/c_util/for_each_in_sub_folder.h
        ./inc/c_util/thread_notifications_dispatcher.h
        ./inc/c_util/tcall_dispatcher_thread_notification_call.h
    )
endif()

if (WIN32)
    set(c_util_c_files
        ${c_util_c_files}
        ./src/for_each_in_folder.c
        ./src/for_each_in_sub_folder.c
        ./src/thread_notifications_dispatcher.c
        ./src/tcall_dispatcher_thread_notification_call.c
    )
endif()

function(get_murmurhash2_directory MURMURHASH2_DIR)
    if(EXISTS "${PROJECT_SOURCE_DIR}/deps/smhasher/src/MurmurHash2.cpp" )
        set(MURMURHASH2_DIR "${PROJECT_SOURCE_DIR}/deps/smhasher/src" PARENT_SCOPE)
    elseif(EXISTS "${PROJECT_SOURCE_DIR}/../smhasher/src/MurmurHash2.cpp")
        set(MURMURHASH2_DIR "${PROJECT_SOURCE_DIR}/../smhasher/src" PARENT_SCOPE)
    endif()
endfunction(get_murmurhash2_directory)

get_murmurhash2_directory(MURMURHASH2_DIR)

if (EXISTS ${MURMURHASH2_DIR}/MurmurHash2.cpp )
    set(c_util_cpp_files ${c_util_cpp_files} ${MURMURHASH2_DIR}/MurmurHash2.cpp)
    set(c_util_h_files ${c_util_h_files} ${MURMURHASH2_DIR}/MurmurHash2.h)
else()
    message(FATAL_ERROR "${CMAKE_CURRENT_LIST_DIR}/deps/smhasher/src/MurmurHash2.cpp does not exist (need to update submodules?)" )
endif()

FILE(GLOB c_util_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${c_util_md_files})

#this is the product (a library)
add_library(c_util ${c_util_c_files} ${c_util_cpp_files} ${c_util_h_files} ${c_util_md_files})

target_link_libraries(c_util c_pal)

target_include_directories(c_util PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc> ${MURMURHASH2_DIR})

set(c_util_target_libs)

if(${logerror_captures_stack_traces})
    set(c_util_target_libs ${c_util_target_libs} Dbghelp)
endif()

if(${use_applessl})
    set(c_util_target_libs ${c_util_target_libs} ${cf_foundation} ${cf_network})
endif()

if(WIN32)
    # maybe this should make its way to c-pal at some point ... maybe
    add_subdirectory(thread_notifications_lackey_dll)

    target_link_libraries(c_util thread_notifications_lackey_dll)
endif()

include_directories(${MACRO_UTILS_INC_FOLDER})
include_directories(${UMOCK_C_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)

if((CMAKE_GENERATOR MATCHES "Visual Studio") AND (${run_traceability}))
    #add traceability custom target
    add_custom_target(c_util_traceability ALL
        COMMAND traceabilitytool -buildcheck -e ${CMAKE_CURRENT_LIST_DIR}/deps -i ${CMAKE_CURRENT_LIST_DIR})
    add_dependencies(c_util_traceability traceabilitytool)
endif()

# make an install target so we can produce a Linux native client package.
install_library_with_prefix(c_util c_util  ${c_util_h_files} )

set(VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER
    ${VALGRIND_SUPPRESSIONS_FILE_EXTRA_PARAMETER}
    "--suppressions=${CMAKE_CURRENT_LIST_DIR}/build_functions/c_util_suppressions.sup"
    "--suppressions=${CMAKE_CURRENT_LIST_DIR}/deps/c-pal/build_functions/c-pal_suppressions.sup"
)

add_subdirectory(test_helpers)
add_subdirectory(tests)

if(${run_reals_check})
    add_reals_check_target()
endif()

include(CMakePackageConfigHelpers)
