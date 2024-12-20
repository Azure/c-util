#Copyright (C) Microsoft Corporation. All rights reserved.

set(theseTestsName async_op_samples_int)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    common_async_op_module_interface.c
    hl_async_op_module.c
    hl_async_op_module_cancel_all.c
    ll_async_op_module_fake_cancel.c
    ll_async_op_module_real_cancel.c
    ml_async_op_module.c
    ml_async_op_module_with_async_chain.c
    ml_async_op_module_with_retries.c
)

set(${theseTestsName}_h_files
    common_async_op_module_interface.h
    hl_async_op_module.h
    hl_async_op_module_cancel_all.h
    ll_async_op_module_fake_cancel.h
    ll_async_op_module_real_cancel.h
    ml_async_op_module.h
    ml_async_op_module_with_async_chain.h
    ml_async_op_module_with_retries.h
)

build_test_artifacts(${theseTestsName} "samples" ADDITIONAL_LIBS c_util)
