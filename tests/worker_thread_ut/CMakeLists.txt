#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

compileAsC11()
set(theseTestsName worker_thread_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
worker_thread_mocked.c
)

set(${theseTestsName}_h_files
../../inc/c_util/worker_thread.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util c_pal_reals c_util_reals)

if("${building}" STREQUAL "exe")
#   /ignore:4217 is: "warning LNK4217: symbol 'symbol' defined in 'filename_1.obj' is imported by 'filename_2.obj' in function 'function'"
    set_target_properties(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()

if("${building}" STREQUAL "dll")
#   /ignore:4217 is: "warning LNK4217: symbol 'symbol' defined in 'filename_1.obj' is imported by 'filename_2.obj' in function 'function'"
    set_target_properties(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
endif()
