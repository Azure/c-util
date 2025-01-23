#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName queue_processor_int)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util cpal)

if(WIN32)
    if("${building}" STREQUAL "exe")
        set_target_properties(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
    endif()

    if("${building}" STREQUAL "dll")
        set_target_properties(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "/ignore:4217")
    endif()
endif()
