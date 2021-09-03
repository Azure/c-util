#Copyright (C) Microsoft Corporation. All rights reserved.

set(constbuffer_sample_h_files
)

set(constbuffer_sample_c_files
    main.c
)

add_executable(constbuffer_sample ${constbuffer_sample_h_files} ${constbuffer_sample_c_files})
target_link_libraries(constbuffer_sample c_util c_logging)


