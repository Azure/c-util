#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName thread_notifications_dispatcher_wo_init_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/thread_notifications_dispatcher.c
)

set(${theseTestsName}_h_files
../../inc/c_util/thread_notifications_dispatcher.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" 
    ADDITIONAL_LIBS c_pal c_pal_reals c_util c_util_reals thread_notifications_lackey_dll
    MOCK_PRECOMPILE_HEADERS
        "c_pal/gballoc_hl.h"
        "c_pal/gballoc_hl_redirect.h"
        "c_util/tcall_dispatcher_thread_notification_call.h"
)

if("${building}" STREQUAL "exe")
    copy_thread_notifications_lackey_outputs(${theseTestsName}_exe_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_exe_${CMAKE_PROJECT_NAME}>)
endif()

if("${building}" STREQUAL "dll")
    copy_thread_notifications_lackey_outputs(${theseTestsName}_dll_${CMAKE_PROJECT_NAME} $<TARGET_FILE_DIR:${theseTestsName}_dll_${CMAKE_PROJECT_NAME}>)
endif()
