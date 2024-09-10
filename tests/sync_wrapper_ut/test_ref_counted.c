// Copyright (c) Microsoft. All rights reserved.

#include "c_pal/refcount.h"
#include "c_logging/logger.h"
#include "test_ref_counted.h"

typedef struct TEST_REFCOUNTED_TAG
{
    unsigned char dummy;
} TEST_REFCOUNTED;

DEFINE_REFCOUNT_TYPE(TEST_REFCOUNTED)

TEST_REFCOUNTED_HANDLE test_refcounted_create(void)
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

void test_refcounted_inc_ref(TEST_REFCOUNTED_HANDLE test_refcounted)
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

void test_refcounted_dec_ref(TEST_REFCOUNTED_HANDLE test_refcounted)
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
