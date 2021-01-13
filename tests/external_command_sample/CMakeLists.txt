#Copyright (C) Microsoft Corporation. All rights reserved.

set(external_command_sample_h_files
)

set(external_command_sample_c_files
    main.c
)

add_executable(external_command_sample ${external_command_sample_h_files} ${external_command_sample_c_files})
target_link_libraries(external_command_sample)

function(copy_external_command_sample target output_directory)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:external_command_sample>/$<TARGET_FILE_NAME:external_command_sample>" "${output_directory}/ext/$<TARGET_FILE_NAME:external_command_sample>" )
    copy_disable_vld_ini(${target} "${output_directory}/ext")
endfunction()
