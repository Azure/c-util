#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName c_util_reals_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util c_util_reals c_pal)

if(WIN32)
    if("${building}" STREQUAL "exe")
        copy_thread_notifications_lackey_outputs(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_exe_${CMAKE_PROJECT_NAME}>)
    endif()

    if("${building}" STREQUAL "dll")
        copy_thread_notifications_lackey_outputs(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_dll_${CMAKE_PROJECT_NAME}>)
    endif()
endif()