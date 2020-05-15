// Copyright(C) Microsoft Corporation.All rights reserved.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#endif


#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;



#include "azure_c_util/interlocked_if.h"


BEGIN_TEST_SUITE(interlocked_if_int)


TEST_SUITE_INITIALIZE(a)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    
}

TEST_SUITE_CLEANUP(b)
{

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(c)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

}

TEST_FUNCTION_CLEANUP(d)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_INTERLOCKED_IF_43_001 [`InterlockedIFAdd` shall perform an atomic addition operation on the specified `long` values.] */
TEST_FUNCTION(InterlockedIFAdd_does_addition)
{
    ///arrange
    volatile long a = 1;
    long b = 2;

    ///act
    long c = InterlockedIFAdd(&a, b);

    ///assert
    ASSERT_ARE_EQUAL(long, 3, c);
}

END_TEST_SUITE(interlocked_if_int)
