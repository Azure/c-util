#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName uniqueid_ut)

if(WIN32)
    set(uniqueid_ut_test_file ${theseTestsName}_win32.c)
else()
    set(uniqueid_ut_test_file ${theseTestsName}_linux.c)
endif()

set(${theseTestsName}_test_files
    ${uniqueid_ut_test_file}
)

if(WIN32)
set(${theseTestsName}_c_files
    uniqueid_win32_mocked.c
)
else()
set(${theseTestsName}_c_files
    ${UNIQUEID_C_FILE}
)
endif()

set(${theseTestsName}_h_files
    ../../inc/azure_c_util/uniqueid.h
)

if(APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(UUID REQUIRED uuid)
endif()

if(WIN32)
    build_c_tests(${theseTestsName} ON "tests/azure_c_util_tests" ADDITIONAL_LIBS rpcrt4.lib)
elseif(UNIX)
    if(APPLE)
        build_c_tests(${theseTestsName} ON "tests/azure_c_util_tests" ADDITIONAL_LIBS -L${UUID_LIBRARY_DIRS} ${UUID_LIBRARIES})
    else()
        build_c_tests(${theseTestsName} ON "tests/azure_c_util_tests" ADDITIONAL_LIBS uuid)
    endif()
endif()
