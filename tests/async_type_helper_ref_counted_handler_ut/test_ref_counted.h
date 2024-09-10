// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_REF_COUNTED_H
#define TEST_REF_COUNTED_H

#include "c_pal/refcount.h"
#include "umock_c/umock_c.h"



    typedef struct TEST_REFCOUNTED_TAG* TEST_REFCOUNTED_HANDLE;

    TEST_REFCOUNTED_HANDLE test_refcounted_create(void);
    MOCKABLE_FUNCTION(, void, test_refcounted_inc_ref, TEST_REFCOUNTED_HANDLE, test_refcounted);
    MOCKABLE_FUNCTION(, void, test_refcounted_dec_ref, TEST_REFCOUNTED_HANDLE, test_refcounted);



#endif // TEST_REF_COUNTED_H
