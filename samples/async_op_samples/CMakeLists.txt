#Copyright (C) Microsoft Corporation. All rights reserved.

set(async_op_samples_h_files
    hl_async_module.h
    hl_async_module_cancel_all.h
    ll_async_module_fake_cancel.h
    ll_async_module_real_cancel.h
    ml_async_module.h
    ml_async_module_with_async_chain.h
    ml_async_module_with_retries.h
)

set(async_op_samples_c_files
    hl_async_module.c
    hl_async_module_cancel_all.c
    ll_async_module_fake_cancel.c
    ll_async_module_real_cancel.c
    ml_async_module.c
    ml_async_module_with_async_chain.c
    ml_async_module_with_retries.c
    main.c
)

set(async_op_samples_rc_files
    ${LOGGING_RC_FILE}
)

add_executable(async_op_samples ${async_op_samples_h_files} ${async_op_samples_c_files} ${async_op_samples_rc_files})
target_link_libraries(async_op_samples c_util)

set_target_properties(async_op_samples PROPERTIES
    FOLDER "samples")
