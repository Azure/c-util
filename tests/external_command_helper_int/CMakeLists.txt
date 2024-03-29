#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName external_command_helper_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_cpp_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util c_pal)

if("${building}" STREQUAL "exe")
    add_dependencies(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} external_command_sample)
    copy_external_command_sample(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_exe_${CMAKE_PROJECT_NAME}>)

    add_definitions(-DEXTERNAL_COMMAND_PATH="$<TARGET_FILE_DIR:${theseTestsName}_exe_${CMAKE_PROJECT_NAME}>/ext/$<TARGET_FILE_NAME:external_command_sample>")

endif()

if("${building}" STREQUAL "dll")
    add_dependencies(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} external_command_sample)
    copy_external_command_sample(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_dll_${CMAKE_PROJECT_NAME}>)

    add_definitions(-DEXTERNAL_COMMAND_PATH="$<TARGET_FILE_DIR:${theseTestsName}_dll_${CMAKE_PROJECT_NAME}>/ext/$<TARGET_FILE_NAME:external_command_sample>")
endif()
