#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName batch_queue_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
)

include_directories(${PROJECT_SOURCE_DIR}/version)

build_test_artifacts(${theseTestsName} "tests/elastic_log_common" ADDITIONAL_LIBS elastic_log_common c_util_test_helpers )

if("${building}" STREQUAL "exe")
    copy_ifxmetrics_dll(${theseTestsName}_exe_elastic_log $<TARGET_FILE_DIR:${theseTestsName}_exe_elastic_log>)
    copy_thread_notifications_lackey_outputs(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_exe_elastic_log>)
endif()

if("${building}" STREQUAL "dll")
    copy_ifxmetrics_dll(${theseTestsName}_dll_elastic_log $<TARGET_FILE_DIR:${theseTestsName}_dll_elastic_log>)
    copy_thread_notifications_lackey_outputs(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_dll_elastic_log>)
endif()
