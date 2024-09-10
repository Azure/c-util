// Copyright (c) Microsoft. All rights reserved.

#include "c_util/async_type_helper_ref_counted_handler.h"
#include "test_ref_counted_async_type_helper_handler.h"

/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_003: [ DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER shall implement the copy handler by expanding to: ]*/
/* Tests_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_008: [ DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER shall implement the free handler by expanding to: ]*/
DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(TEST_REFCOUNTED_HANDLE, test_refcounted_inc_ref, test_refcounted_dec_ref);
