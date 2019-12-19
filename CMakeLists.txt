#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

if (TARGET azure_c_util)
    RETURN()
endif()

project(azure_c_util)

#the following variables are project-wide and can be used with cmake-gui
option(run_unittests "set run_unittests to ON to run unittests (default is OFF)" OFF)
option(run_e2e_tests "set run_e2e_tests to ON to run e2e tests (default is OFF). Chsare dutility does not have any e2e tests, but the option needs to exist to evaluate in IF statements" OFF)
option(run_int_tests "set run_int_tests to ON to integration tests (default is OFF)." OFF)
option(use_cppunittest "set use_cppunittest to ON to build CppUnitTest tests on Windows (default is ON)" ON)

if(WIN32)
    option(logerror_captures_stack_traces "LogError will make a reasonable attempt to capture the stack trace at the time of calling LogError. Only available on Windows" OFF)
    set(use_etw OFF CACHE STRING "set use_etw to OFF = No ETW logging, TRACELOGGING = Use ETW from tracelogging.h, TRACELOGGING_WITH_CONSOLE = Use ETW from tracelogging.h and call console logger. Default is OFF")
    set_property(CACHE use_etw PROPERTY STRINGS TRACELOGGING_WITH_CONSOLE TRACELOGGING OFF)
endif()

option(no_logging "disable logging (default is OFF)" OFF)

if(WIN32)

if(${use_etw} STREQUAL "TRACELOGGING_WITH_CONSOLE")
    add_definitions(-DUSE_TRACELOGGING)
    add_definitions(-DCALL_CONSOLE_LOGGER)
endif() #(${use_etw} STREQUAL "TRACELOGGING_WITH_CONSOLE")

if(${use_etw} STREQUAL "TRACELOGGING")
    add_definitions(-DUSE_TRACELOGGING)
endif() #(${use_etw} STREQUAL "TRACELOGGING")

endif()

if(${logerror_captures_stack_traces})
    #set a preprocessor option (LOGERROR_CAPTURES_STACK_TRACES) and add the files that provide the functionality
    add_definitions(-DLOGERROR_CAPTURES_STACK_TRACES)
    set(LOGGING_STACKTRACE_C_FILE ./src/logging_stacktrace.c)
    set(LOGGING_STACKTRACE_H_FILE ./inc/azure_c_util/logging_stacktrace.h)
else()
    set(LOGGING_STACKTRACE_C_FILE)
    set(LOGGING_STACKTRACE_H_FILE)
endif()

if(${no_logging})
    add_definitions(-DNO_LOGGING)
endif()

# This function focuses on setting files which are unique to a given hardware platform.
# The choice of tlsio is not unique per-platform, and is set in the main CMakeLists.txt
function(set_platform_files c_util_dir)
    if(WIN32)
        if(${use_etw} STREQUAL "OFF")
            set(XLOGGING_C_FILE ${c_util_dir}/src/xlogging.c PARENT_SCOPE)
            set(LOGGING_C_FILE ${c_util_dir}/src/consolelogger.c PARENT_SCOPE)
            set(LOGGING_H_FILE ${c_util_dir}/inc/azure_c_util/consolelogger.h PARENT_SCOPE)
        elseif(${use_etw} STREQUAL "TRACELOGGING")
            set(XLOGGING_C_FILE ${c_util_dir}/src/etwxlogging.c PARENT_SCOPE)
            set(LOGGING_C_FILE ${c_util_dir}/src/etwlogger_driver.c PARENT_SCOPE)
            set(LOGGING_H_FILE ${c_util_dir}/inc/azure_c_util/etwlogger_driver.h PARENT_SCOPE)
        elseif(${use_etw} STREQUAL "TRACELOGGING_WITH_CONSOLE")
            set(XLOGGING_C_FILE ${c_util_dir}/src/etwxlogging.c PARENT_SCOPE)
            set(LOGGING_C_FILE ${c_util_dir}/src/etwlogger_driver.c ${c_util_dir}/src/consolelogger.c PARENT_SCOPE)
            set(LOGGING_H_FILE ${c_util_dir}/inc/azure_c_util/etwlogger_driver.h ${c_util_dir}/inc/azure_c_util/consolelogger.h PARENT_SCOPE)
        endif()
    else()
        set(XLOGGING_C_FILE ${c_util_dir}/src/xlogging.c PARENT_SCOPE)
        set(LOGGING_C_FILE ${c_util_dir}/src/consolelogger.c PARENT_SCOPE)
        set(LOGGING_H_FILE ${c_util_dir}/inc/azure_c_util/consolelogger.h PARENT_SCOPE)
    endif()
    
    if(WIN32 OR MACOSX OR LINUX)
        set (ENVIRONMENT_VARIABLE_C_FILE ${c_util_dir}/adapters/envvariable.c PARENT_SCOPE)
    endif()
endfunction(set_platform_files)

macro(compileAsC99)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
      if (CXX_FLAG_CXX11)
        set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
      else()
        set (CMAKE_CXX_FLAGS "--std=c++0x ${CMAKE_CXX_FLAGS}")
      endif()
    endif()
  else()
    set (CMAKE_C_STANDARD 99)
    set (CMAKE_CXX_STANDARD 11)
  endif()
endmacro(compileAsC99)

macro(compileAsC11)
  if (CXX_FLAG_CXX11)
    if (CMAKE_VERSION VERSION_LESS "3.1")
      if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set (CMAKE_C_FLAGS "--std=c11 ${CMAKE_C_FLAGS}")
        set (CMAKE_C_FLAGS "-D_POSIX_C_SOURCE=200112L ${CMAKE_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
      endif()
    else()
      set (CMAKE_C_STANDARD 11)
      set (CMAKE_CXX_STANDARD 11)
    endif()
  else()
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
        set (CMAKE_CXX_FLAGS "--std=c++0x ${CMAKE_CXX_FLAGS}")
    else()
      set (CMAKE_C_STANDARD 11)
      set (CMAKE_CXX_STANDARD 11)
    endif()
  endif()
endmacro(compileAsC11)

function(compileTargetAsC99 theTarget)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      set_target_properties(${theTarget} PROPERTIES COMPILE_FLAGS "--std=c99")
    endif()
  else()
    set_target_properties(${theTarget} PROPERTIES C_STANDARD 99)
    set_target_properties(${theTarget} PROPERTIES CXX_STANDARD 11)
  endif()
endfunction()

function(compileTargetAsC11 theTarget)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
      if (CXX_FLAG_CXX11)
        set_target_properties(${theTarget} PROPERTIES COMPILE_FLAGS "--std=c11 -D_POSIX_C_SOURCE=200112L")
      else()
        set_target_properties(${theTarget} PROPERTIES COMPILE_FLAGS "--std=c99 -D_POSIX_C_SOURCE=200112L")
      endif()
    endif()
  else()
    set_target_properties(${theTarget} PROPERTIES C_STANDARD 11)
    set_target_properties(${theTarget} PROPERTIES CXX_STANDARD 11)
  endif()
endfunction()

include(CheckSymbolExists)
function(detect_architecture symbol arch)
    if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
        set(CMAKE_REQUIRED_QUIET 1)
        check_symbol_exists("${symbol}" "" ARCHITECTURE_${arch})
        unset(CMAKE_REQUIRED_QUIET)

        # The output variable needs to be unique across invocations otherwise
        # CMake's crazy scope rules will keep it defined
        if (ARCHITECTURE_${arch})
            set(ARCHITECTURE "${arch}" PARENT_SCOPE)
            set(ARCHITECTURE_${arch} 1 PARENT_SCOPE)
            add_definitions(-DARCHITECTURE_${arch}=1)
        endif()
    endif()
endfunction()
if (MSVC)
    detect_architecture("_M_AMD64" x86_64)
    detect_architecture("_M_IX86" x86)
    detect_architecture("_M_ARM" ARM)
else()
    detect_architecture("__x86_64__" x86_64)
    detect_architecture("__i386__" x86)
    detect_architecture("__arm__" ARM)
endif()
if (NOT DEFINED ARCHITECTURE OR ARCHITECTURE STREQUAL "")
    set(ARCHITECTURE "GENERIC")
endif()
message(STATUS "target architecture: ${ARCHITECTURE}")

#Setup the platform files
set_platform_files(${CMAKE_CURRENT_LIST_DIR})

include_directories(${UMOCK_C_INC_FOLDER})
include_directories(${UMOCK_C_INC_FOLDER})

set(azure_c_util_c_files
    ./adapters/agenttime.c
    ./src/buffer.c
    ./src/crt_abstractions.c
    ./src/constbuffer.c
    ./src/constbuffer_array.c
    ./src/constbuffer_array_batcher.c
    ./src/memory_data.c
    ${LOGGING_C_FILE}
    ${XLOGGING_C_FILE}
    ${LOCK_C_FILE}
    ${SRW_LOCK_C_FILE}
    ${TIMER_C_FILE}
    ${STRING_UTILS_C_FILE}
    ${TICKCOUTER_C_FILE}
    ${THREAD_C_FILE}
    ${UNIQUEID_C_FILE}
    ${ENVIRONMENT_VARIABLE_C_FILE}
    ${LOGGING_STACKTRACE_C_FILE}
)

set(azure_c_util_h_files
    ./inc/azure_c_util/agenttime.h
    ./inc/azure_c_util/buffer_.h
    ./inc/azure_c_util/constbuffer.h
    ./inc/azure_c_util/constbuffer_array.h
    ./inc/azure_c_util/constbuffer_array_batcher.h
    ./inc/azure_c_util/crt_abstractions.h
    ${LOGGING_H_FILE}
    ./inc/azure_c_util/envvariable.h
    ./inc/azure_c_util/gballoc.h
    ./inc/azure_c_util/lock.h
    ./inc/azure_c_util/memory_data.h
    ./inc/azure_c_util/refcount.h
    ./inc/azure_c_util/srw_lock.h
    ./inc/azure_c_util/string_utils.h
    ./inc/azure_c_util/timer.h
    ./inc/azure_c_util/tickcounter.h
    ./inc/azure_c_util/threadapi.h
    ./inc/azure_c_util/uniqueid.h
    ./inc/azure_c_util/uuid.h
    ./inc/azure_c_util/xlogging.h
    ${LOGGING_STACKTRACE_H_FILE}
)

FILE(GLOB azure_c_util_md_files "devdoc/*.md")
SOURCE_GROUP(devdoc FILES ${azure_c_util_md_files})

if(MSVC)
    set(azure_c_util_h_files ${azure_c_util_h_files}
        ./pal/windows/refcount_os.h
    )
else()
    set(azure_c_util_h_files ${azure_c_util_h_files}
        ./pal/linux/refcount_os.h
    )
endif()

#this is the product (a library)
add_library(azure_c_util ${azure_c_util_c_files} ${azure_c_util_h_files} ${azure_c_util_md_files})

if(MSVC)
    target_include_directories(azure_c_util PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/pal/windows>)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/pal/windows)
else()
    target_include_directories(azure_c_util PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/pal/linux>)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/pal/linux)
endif()

target_include_directories(azure_c_util PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>)

set(azure_c_util_target_libs)

if(${logerror_captures_stack_traces})
    set(azure_c_util_target_libs ${azure_c_util_target_libs} Dbghelp)
endif()

if(${use_applessl})
    set(azure_c_util_target_libs ${azure_c_util_target_libs} ${cf_foundation} ${cf_network})
endif()

if(WIN32)
    set(azure_c_util_target_libs ${azure_c_util_target_libs} rpcrt4.lib)
endif()

if(LINUX)
    set(azure_c_util_target_libs ${azure_c_util_target_libs} pthread m rt)
    set(azure_c_util_target_libs ${azure_c_util_target_libs} uuid)
endif()

target_link_libraries(azure_c_util ${azure_c_util_target_libs})

set(original_run_e2e_tests ${run_e2e_tests})
set(original_run_unittests ${run_unittests})
set(original_run_int_tests ${run_int_tests})

set(run_e2e_tests OFF)
set(run_unittests OFF)
set(run_int_tests OFF)

if ((NOT TARGET azure_c_build_tools) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-build-tools/CMakeLists.txt))
        add_subdirectory(deps/azure-c-build-tools)
endif()

if ((NOT TARGET azure_macro_utils_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-macro-utils-c/CMakeLists.txt))
        add_subdirectory(deps/azure-macro-utils-c)
        include_directories(deps/azure-macro-utils-c/inc)
endif()

if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-ctest/CMakeLists.txt))
        add_subdirectory(deps/azure-ctest)
        include_directories(deps/azure-ctest/inc)
endif()

if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-testrunnerswitcher/CMakeLists.txt))
        add_subdirectory(deps/azure-c-testrunnerswitcher)
        include_directories(deps/azure-c-testrunnerswitcher/inc)
endif()

if ((NOT TARGET umock_c) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/umock-c/CMakeLists.txt))
        add_subdirectory(deps/umock-c)
        include_directories(deps/umock-c/inc)
endif()

include_directories(${MACRO_UTILS_INC_FOLDER})
include_directories(${UMOCK_C_INC_FOLDER})
include_directories(${CMAKE_CURRENT_LIST_DIR}/inc)

set(run_e2e_tests ${original_run_e2e_tests})
set(run_unittests ${original_run_unittests})
set(run_int_tests ${original_run_int_tests})

if (${run_unittests} OR ${run_int_tests})
    add_subdirectory(tests)
endif()

include(CMakePackageConfigHelpers)

compileTargetAsC99(azure_c_util)
