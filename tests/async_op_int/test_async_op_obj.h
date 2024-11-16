// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_ASYNC_OP_H
#define TEST_ASYNC_OP_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "c_util/async_op.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct TEST_ASYNC_OP_TAG TEST_ASYNC_OP;

    THANDLE_TYPE_DECLARE(TEST_ASYNC_OP);

    typedef void(*TEST_CALLBACK)(void* context);

    MOCKABLE_FUNCTION(, THANDLE(TEST_ASYNC_OP), test_async_op_create, uint32_t, id);

    MOCKABLE_FUNCTION(, int, test_async_op_open, THANDLE(TEST_ASYNC_OP), test_async);
    MOCKABLE_FUNCTION(, void, test_async_op_close, THANDLE(TEST_ASYNC_OP), test_async);

    MOCKABLE_FUNCTION(, int, test_async_op_start_call_async, THANDLE(TEST_ASYNC_OP), test_async, TEST_CALLBACK, test_callback, void*, test_ctx, THANDLE(ASYNC_OP)*, async_op);

#ifdef __cplusplus
}
#endif

#endif // TEST_ASYNC_OP_H
