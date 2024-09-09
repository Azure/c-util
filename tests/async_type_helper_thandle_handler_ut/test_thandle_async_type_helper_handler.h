// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_THANDLE_ASYNC_TYPE_HELPER_HANDLER_H
#define TEST_THANDLE_ASYNC_TYPE_HELPER_HANDLER_H

#include "test_thandle.h"
#include "c_util/async_type_helper_thandle_handler.h"



    /* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_001: [ DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall declare the copy handler by expanding to: ]*/
    /* Tests_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_002: [ DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall declare the free handler by expanding to: ]*/
    DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(TEST_THANDLE);



#endif // TEST_THANDLE_ASYNC_TYPE_HELPER_HANDLER_H
