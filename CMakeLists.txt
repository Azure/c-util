#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

# canon way of using azure-c-util from another repo is below. It assumes the using repo has placed azure-c-util in "deps"

#if ((NOT TARGET azure_c_util) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-util/CMakeLists.txt))
#    add_subdirectory(deps/azure-c-util)
#    include_directories(${C_UTIL_INC_FOLDER})
#endif() 

if (TARGET azure_c_util)
    RETURN()
endif()

project(azure_c_util)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF). Chsare dutility does not have any e2e tests, but the option needs to exist to evaluate in IF statements" OFF)
option(run_int_tests "set run_int_tests to ON to integration tests (default is OFF)." OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is ON)" ON)
option(run_traceability "run traceability tool (default is ON)" ON)

set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_unittests ${run_unittests})
set(original_run_int_tests ${run_int_tests})
set(original_run_traceability ${run_traceability})
set(original_run_perf_tests ${run_perf_tests})

set(run_e2e_tests OFF)
set(run_unittests OFF)
set(run_int_tests OFF)
set(run_traceability OFF)
set(run_perf_tests OFF)

if ((NOT TARGET azure_c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-build-tools/CMakeLists.txt))
    add_subdirectory(deps/azure-c-build-tools)
    set_default_build_options()
endif() 

if ((NOT TARGET azure_macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-macro-utils-c/CMakeLists.txt))
    add_subdirectory(deps/azure-macro-utils-c)
    include_directories(${MACRO_UTILS_INC_FOLDER})
endif() 

if ((NOT TARGET azure_c_logging) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-logging/CMakeLists.txt))
    add_subdirectory(deps/azure-c-logging)
    include_directories(deps/azure-c-logging/inc)
endif() 

if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-ctest/CMakeLists.txt))
    add_subdirectory(deps/azure-ctest)
    include_directories(${CTEST_INC_FOLDER})
endif() 

if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-testrunnerswitcher/CMakeLists.txt))
    add_subdirectory(deps/azure-c-testrunnerswitcher)
    include_directories(${TESTRUNNERSWITCHER_INC_FOLDER})
endif() 

if ((NOT TARGET umock_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/umock-c/CMakeLists.txt))
    add_subdirectory(deps/umock-c)
    include_directories(${UMOCK_C_INC_FOLDER})
endif() 

if ((NOT TARGET azure_c_pal) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-pal/CMakeLists.txt))
    add_subdirectory(deps/azure-c-pal)
    include_directories(${C_PAL_INC_FOLDER})
endif() 

if (
    (NOT TARGET mimalloc-obj) AND 
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

include(CTest)
enable_testing()

set(C_UTIL_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using sharedLib lib" FORCE)

# This function focuses on setting files which are unique to a given hardware platform.
# The choice of tlsio is not unique per-platform, and is set in the main CMakeLists.txt
function(set_platform_files c_util_dir)
    if(WIN32)
        set(SM_C_FILE ${c_util_dir}/src/sm.c PARENT_SCOPE)
        set(SM_H_FILE ${c_util_dir}/inc/azure_c_util/sm.h PARENT_SCOPE)
    else()
        set(SM_C_FILE) #there's no SM for linux
        set(SM_H_FILE) #there's no SM for linux
    endif()
 endfunction(set_platform_files)

#Setup the platform files
set_platform_files(${CMAKE_CURRENT_LIST_DIR})

include_directories(${UMOCK_C_INC_FOLDER})

set(azure_c_util_c_files
    ./src/azure_base64.c
    ./src/buffer.c
    ./src/constbuffer.c
    ./src/constbuffer_array.c
    ./src/constbuffer_array_batcher_nv.c
    ./src/doublylinkedlist.c
    ./src/interlocked_hl.c
    ./src/map.c
    ./src/memory_data.c
    ./src/rc_string.c
    ./src/singlylinkedlist.c
    ./src/strings.c
    ./src/uuid.c
    ${SM_C_FILE}
)

set(azure_c_util_h_files
    ./inc/azure_c_util/azure_base64.h
    ./inc/azure_c_util/buffer_.h
    ./inc/azure_c_util/constbuffer.h
    ./inc/azure_c_util/constbuffer_array.h
    ./inc/azure_c_util/constbuffer_array_batcher_nv.h
    ./inc/azure_c_util/doublylinkedlist.h
    ./inc/azure_c_util/interlocked_hl.h
    ./inc/azure_c_util/map.h
    ./inc/azure_c_util/memory_data.h
    ./inc/azure_c_util/singlylinkedlist.h
    ./inc/azure_c_util/rc_string.h
    ./inc/azure_c_util/strings.h
    ./inc/azure_c_util/strings_types.h
    ./inc/azure_c_util/thandle.h
    ./inc/azure_c_util/uuid.h
    ${SM_H_FILE}
)

FILE(GLOB azure_c_util_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${azure_c_util_md_files})

#this is the product (a library)
add_library(azure_c_util ${azure_c_util_c_files} ${azure_c_util_h_files} ${azure_c_util_md_files})

target_link_libraries(azure_c_util azure_c_pal)

target_include_directories(azure_c_util PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>)

set(azure_c_util_target_libs)

if(${logerror_captures_stack_traces})
    set(azure_c_util_target_libs ${azure_c_util_target_libs} Dbghelp)
endif()

if(${use_applessl})
    set(azure_c_util_target_libs ${azure_c_util_target_libs} ${cf_foundation} ${cf_network})
endif()

include_directories(${MACRO_UTILS_INC_FOLDER})
include_directories(${UMOCK_C_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)


if((WIN32) AND (${run_traceability}))
    #add traceability custom target
    add_custom_target(azure_c_util_traceability ALL
        COMMAND traceabilitytool -buildcheck -e ${CMAKE_CURRENT_LIST_DIR}/deps -i ${CMAKE_CURRENT_LIST_DIR})
    add_dependencies(azure_c_util_traceability traceabilitytool)
endif()

add_subdirectory(tests)

include(CMakePackageConfigHelpers)

compileTargetAsC99(azure_c_util)
