// Copyright (c) Microsoft. All rights reserved.

#include <stddef.h>

#include "c_logging/logger.h"

#include "c_pal/refcount.h"

#include "real_gballoc_ll.h"

#include "test_ref_counted.h"

typedef struct TEST_REFCOUNTED_TAG
{
    unsigned char dummy;
} TEST_REFCOUNTED;

DEFINE_REFCOUNT_TYPE_WITH_CUSTOM_ALLOC(TEST_REFCOUNTED, real_gballoc_ll_malloc, real_gballoc_ll_malloc_flex, real_gballoc_ll_free);

IMPLEMENT_MOCKABLE_FUNCTION(, TEST_REFCOUNTED_HANDLE, test_refcounted_create)
{
    TEST_REFCOUNTED_HANDLE result = REFCOUNT_TYPE_CREATE(TEST_REFCOUNTED);
    if (result == NULL)
    {
        LogError("REFCOUNT_TYPE_CREATE(TEST_REFCOUNTED) failed");
    }
    else
    {
        // all OK
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, test_refcounted_inc_ref, TEST_REFCOUNTED_HANDLE, test_refcounted)
{
    if (test_refcounted == NULL)
    {
        LogError("Invalid arguments: TEST_REFCOUNTED_HANDLE test_refcounted=%p", test_refcounted);
    }
    else
    {
        INC_REF(TEST_REFCOUNTED, test_refcounted);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, test_refcounted_dec_ref, TEST_REFCOUNTED_HANDLE, test_refcounted)
{
    if (test_refcounted == NULL)
    {
        LogError("Invalid arguments: TEST_REFCOUNTED_HANDLE test_refcounted=%p", test_refcounted);
    }
    else
    {
        if (DEC_REF(TEST_REFCOUNTED, test_refcounted) == 0)
        {
            REFCOUNT_TYPE_DESTROY(TEST_REFCOUNTED, test_refcounted);
        }
    }
}
