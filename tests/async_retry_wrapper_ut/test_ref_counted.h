// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_REF_COUNTED_H
#define TEST_REF_COUNTED_H

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"



typedef struct TEST_REFCOUNTED_TAG* TEST_REFCOUNTED_HANDLE;

MOCKABLE_FUNCTION(, TEST_REFCOUNTED_HANDLE, test_refcounted_create);
MOCKABLE_FUNCTION(, void, test_refcounted_inc_ref, TEST_REFCOUNTED_HANDLE, test_refcounted);
MOCKABLE_FUNCTION(, void, test_refcounted_dec_ref, TEST_REFCOUNTED_HANDLE, test_refcounted);



#endif // TEST_REF_COUNTED_H
