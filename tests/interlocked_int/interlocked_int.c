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



#include "azure_c_util/interlocked.h"


BEGIN_TEST_SUITE(interlocked_int)


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

/*
Tests_SRS_INTERLOCKED_43_001 [ interlocked_add shall atomically add *addend with value and store the result in *addend.]
Tests_SRS_INTERLOCKED_43_032: [interlocked_add shall return the result of the addition.]
*/

TEST_FUNCTION(interlocked_add_does_addition)
{
    ///arrange
    volatile int32_t addend = INT32_MAX;
    int32_t value = INT32_MIN;

    ///act
    int32_t return_val = interlocked_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, -1, addend);
    ASSERT_ARE_EQUAL(int32_t, -1, return_val);
}

/*
Tests_SRS_INTERLOCKED_43_002[interlocked_and shall perform an atomic bitwise AND operation on the 32 - bit integer values * destination and value and store the result in * destination.]
Tests_SRS_INTERLOCKED_43_033: [interlocked_and shall return the initial value of * destination.]
*/

TEST_FUNCTION(interlocked_and_does_bitwise_and)
{
    ///arrange
    volatile uint32_t destination = 0xF0F0F0F0;
    uint32_t value = 0x0F0F0F0F;

    ///act
    uint32_t return_val = (uint32_t)interlocked_and((volatile int32_t*)&destination, (int32_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, -1, destination);
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val);
}

/*
Tests_SRS_INTERLOCKED_43_003 [ interlocked_and_16 shall perform an atomic bitwise AND operation on the 16-bit integer values *destination and value and store the result in *destination.]
Tests_SRS_INTERLOCKED_43_034: [ interlocked_and_16 shall return the initial value of *destination.]
*/

TEST_FUNCTION(interlocked_and_16_does_bitwise_and)
{
    ///arrange
    volatile uint16_t destination = 0xF0F0;
    uint16_t value = 0x0F0F;

    ///act
    uint16_t return_val = (uint16_t)interlocked_and_16((volatile int16_t*)&destination, (int16_t)value);


    ///assert
    ASSERT_ARE_EQUAL(uint16_t, -1, destination);
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val);
}

/*
SRS_INTERLOCKED_43_004 [ interlocked_and_64 shall perform an atomic bitwise AND operation on the 64-bit integer values *destination and value and store the result in *destination.]

SRS_INTERLOCKED_43_035: [ interlocked_and_64 shall return the initial value of *destination.]
*/

TEST_FUNCTION(interlocked_and_64_does_bitwise_and)
{
    ///arrange
    volatile uint64_t destination = 0xF0F0F0F0F0F0F0F0;
    uint64_t value = 0x0F0F0F0F0F0F0F0F;

    ///act
    uint64_t return_val = (uint64_t)interlocked_and_64((volatile int64_t*)&destination, (int64_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint64_t, -1, destination);
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val);
}

/*
SRS_INTERLOCKED_43_005 [ interlocked_and_8 shall perform an atomic bitwise AND operation on the 8-bit integer values *destination and value and store the result in *destination]

SRS_INTERLOCKED_43_036: [ interlocked_and_8 shall return the initial value of *destination. ]
*/
TEST_FUNCTION(interlocked_and_8_does_bitwise_and)
{
    ///arrange
    volatile uint8_t destination = 0xF0;
    uint8_t value = 0x0F0;

    ///act
    uint8_t return_val = (uint8_t)interlocked_and_8((volatile int8_t*)&destination, (int8_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint8_t, -1, destination);
    ASSERT_ARE_EQUAL(uint8_t, 0xF0F0F0F0F0F0F0F0, return_val);
}

/*
SRS_INTERLOCKED_43_006 [ interlocked_compare_exchange shall compare the 32-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.*]

SRS_INTERLOCKED_43_037: [ interlocked_compare_exchange shall return the initial value of *destination.]
*/

TEST_FUNCTION(interlocked_compare_exchange_equal_case)
{
    ///arrange
    volatile int32_t destination = INT32_MAX;
    int32_t comperand = INT32_MAX;
    int32_t exchange = INT32_MIN;

    ///act
    int32_t return_val = interlocked_compare_exchange(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int8_t, INT32_MIN, destination);
    ASSERT_ARE_EQUAL(int8_t, INT32_MAX, return_val);
}
/*
SRS_INTERLOCKED_43_006 [ interlocked_compare_exchange shall compare the 32-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.*]

SRS_INTERLOCKED_43_037: [ interlocked_compare_exchange shall return the initial value of *destination.]
*/
TEST_FUNCTION(interlocked_compare_exchange_not_equal_case)
{
    ///arrange
    volatile int32_t destination = INT32_MAX;
    int32_t comperand = INT32_MIN;
    int32_t exchange = INT32_MIN;

    ///act
    int32_t return_val = interlocked_compare_exchange(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int8_t, INT32_MAX, destination);
    ASSERT_ARE_EQUAL(int8_t, INT32_MAX, return_val);
}

/*
SRS_INTERLOCKED_43_007 [ interlocked_compare_exchange_128 shall compare *destination with *comperand_result. If they are equal, exchange_high and exchange_low are stored in the array specified by destination. These operations are performed atomically.]

SRS_INTERLOCKED_43_039: [ interlocked_compare_exchange_128 shall store the initial value of *destination in *comperand_result regardless of the result of the comparison.` ]

SRS_INTERLOCKED_43_038: [ interlocked_compare_exchange_128 shall return true if *comperand_result equals the original value of *destination.]

*/

TEST_FUNCTION(interlocked_compare_exchange_128_equal_case)
{
    ///arrange
    volatile int64_t destination[2] = { INT64_MAX, INT64_MIN };
    int64_t comperand_result[2] = { INT64_MAX, INT64_MIN };
    int64_t exchange_high = 2;
    int64_t exchange_low = 3;

    ///act
    bool return_val = interlocked_compare_exchange_128(destination, exchange_high, exchange_low, comperand_result);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, 2, destination[0]);
    ASSERT_ARE_EQUAL(int64_t, 3, destination[1]);
    ASSERT_ARE_EQUAL(int64_t, INT32_MAX, comperand_result[0]);
    ASSERT_ARE_EQUAL(int64_t, INT32_MIN, comperand_result[1]);
    ASSERT_ARE_EQUAL(bool, true, return_val);
}

/*
SRS_INTERLOCKED_43_039: [ interlocked_compare_exchange_128 shall store the initial value of *destination in *comperand_result regardless of the result of the comparison.` ]

SRS_INTERLOCKED_43_063: [ interlocked_compare_exchange_128 shall return false if *comperand_result does not equal the original value of *destination. ]
*/
TEST_FUNCTION(interlocked_compare_exchange_128_not_equal_case)
{
    ///arrange
    volatile int64_t destination[2] = { INT64_MAX, INT64_MIN };
    int64_t comperand_result[2] = { INT64_MIN, INT64_MAX };
    int64_t exchange_high = 2;
    int64_t exchange_low = 3;

    ///act
    bool return_val = interlocked_compare_exchange_128(destination, exchange_high, exchange_low, comperand_result);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT32_MAX, destination[0]);
    ASSERT_ARE_EQUAL(int64_t, INT32_MIN, destination[1]);
    ASSERT_ARE_EQUAL(int64_t, INT32_MAX, comperand_result[0]);
    ASSERT_ARE_EQUAL(int64_t, INT32_MIN, comperand_result[1]);
    ASSERT_ARE_EQUAL(bool, false, return_val);
}



END_TEST_SUITE(interlocked_int)
