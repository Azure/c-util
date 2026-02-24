// Copyright (c) Microsoft. All rights reserved.

#include "c_util/async_type_helper_ref_counted_handler.h"
#include "test_ref_counted_async_type_helper_handler.h"

DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(TEST_REFCOUNTED_HANDLE, test_refcounted_inc_ref, test_refcounted_dec_ref);
