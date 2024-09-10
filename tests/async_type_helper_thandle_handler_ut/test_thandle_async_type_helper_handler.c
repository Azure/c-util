// Copyright (c) Microsoft. All rights reserved.

#include "c_util/async_type_helper_thandle_handler.h"
#include "test_thandle_async_type_helper_handler.h"

/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_003: [ DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall implement the copy handler by expanding to: ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_008: [ DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall implement the free handler by expanding to: ]*/
DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(TEST_THANDLE);
