//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

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

/*Tests_SRS_INTERLOCKED_43_001: [ interlocked_add shall atomically add 32-bit integers *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_032: [interlocked_add shall return the result of the addition.]*/
TEST_FUNCTION(interlocked_add_does_addition)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MAX);
    int32_t value = INT32_MIN;

    ///act
    int32_t return_val = interlocked_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, -1, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, -1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_001: [ interlocked_add shall atomically add 32-bit integers *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_032: [interlocked_add shall return the result of the addition.]*/
TEST_FUNCTION(interlocked_add_overflows_upper_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MAX);
    int32_t value = 1;

    ///act
    int32_t return_val = interlocked_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_43_001: [ interlocked_add shall atomically add 32-bit integers *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_032: [interlocked_add shall return the result of the addition.]*/
TEST_FUNCTION(interlocked_add_underflows_lower_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MIN);
    int32_t value = -1;

    ///act
    int32_t return_val = interlocked_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_065: [ interlocked_add_64 shall atomically add 64-bit integers *addend and value and store the result in *addend. ]*/
/*Tests_SRS_INTERLOCKED_43_066: [ interlocked_add_64 shall return the result of the addition. ]*/
TEST_FUNCTION(interlocked_add_64_does_addition)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MAX);
    int64_t value = INT64_MIN;

    ///act
    int64_t return_val = interlocked_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, -1, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, -1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_065: [ interlocked_add_64 shall atomically add 64-bit integers *addend and value and store the result in *addend. ]*/
/*Tests_SRS_INTERLOCKED_43_066: [ interlocked_add_64 shall return the result of the addition. ]*/
TEST_FUNCTION(interlocked_add_64_overflows_upper_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MAX);
    int64_t value = 1;

    ///act
    int64_t return_val = interlocked_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_065: [ interlocked_add_64 shall atomically add 64-bit integers *addend and value and store the result in *addend. ]*/
/*Tests_SRS_INTERLOCKED_43_066: [ interlocked_add_64 shall return the result of the addition. ]*/
TEST_FUNCTION(interlocked_add_64_underflows_lower_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MIN);
    int64_t value = -1;

    ///act
    int64_t return_val = interlocked_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_002: [interlocked_and shall perform an atomic bitwise AND operation on the 32 - bit integer values * destination and value and store the result in * destination.]*/
/*Tests_SRS_INTERLOCKED_43_033: [interlocked_and shall return the initial value of * destination.]*/
TEST_FUNCTION(interlocked_and_does_bitwise_and)
{
    ///arrange
    volatile uint32_t destination;
    interlocked_exchange((volatile int32_t*)&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0FFF;

    ///act
    uint32_t return_val = (uint32_t)interlocked_and((volatile int32_t*)&destination, (int32_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, 0x000000F0, interlocked_or((volatile int32_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_003: [ interlocked_and_16 shall perform an atomic bitwise AND operation on the 16-bit integer values *destination and value and store the result in *destination.]*/
/*Tests_SRS_INTERLOCKED_43_034: [ interlocked_and_16 shall return the initial value of *destination.]*/
TEST_FUNCTION(interlocked_and_16_does_bitwise_and)
{
    ///arrange
    volatile uint16_t destination;
    interlocked_exchange_16((volatile int16_t*)&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0FFF;

    ///act
    uint16_t return_val = (uint16_t)interlocked_and_16((volatile int16_t*)&destination, (int16_t)value);


    ///assert
    ASSERT_ARE_EQUAL(uint16_t, 0x00F0, interlocked_or_16((volatile int16_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_004: [ interlocked_and_64 shall perform an atomic bitwise AND operation on the 64-bit integer values *destination and value and store the result in *destination.]*/
/*Tests_SRS_INTERLOCKED_43_035: [ interlocked_and_64 shall return the initial value of *destination.]*/
TEST_FUNCTION(interlocked_and_64_does_bitwise_and)
{
    ///arrange
    volatile uint64_t destination;
    interlocked_exchange_64((volatile int64_t*)&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0FFF;

    ///act
    uint64_t return_val = (uint64_t)interlocked_and_64((volatile int64_t*)&destination, (int64_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint64_t, 0x00000000000000F0, interlocked_or_64((volatile int64_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_005: [ interlocked_and_8 shall perform an atomic bitwise AND operation on the 8-bit integer values *destination and value and store the result in *destination]*/
/*Tests_SRS_INTERLOCKED_43_036: [ interlocked_and_8 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_and_8_does_bitwise_and)
{
    ///arrange
    volatile uint8_t destination;
    interlocked_exchange_8((volatile int8_t*)&destination, (uint8_t)0xF0);
    uint8_t value = 0xAF;

    ///act
    uint8_t return_val = (uint8_t)interlocked_and_8((volatile int8_t*)&destination, (int8_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint8_t, 0xA0, interlocked_or_8((volatile int8_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_006: [ interlocked_compare_exchange shall compare the 32-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.*]*/
/*Tests_SRS_INTERLOCKED_43_037: [ interlocked_compare_exchange shall return the initial value of *destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_exchanges_when_equal)
{
    ///arrange
    volatile int32_t destination;
    interlocked_exchange(&destination, INT32_MAX);
    int32_t comperand = INT32_MAX;
    int32_t exchange = INT32_MIN;

    ///act
    int32_t return_val = interlocked_compare_exchange(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, interlocked_or(&destination, 0), "*destination is not equal to exchange.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_006: [ interlocked_compare_exchange shall compare the 32-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.*]*/
/*Tests_SRS_INTERLOCKED_43_037: [ interlocked_compare_exchange shall return the initial value of *destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_does_not_exchange_when_not_equal)
{
    ///arrange
    volatile int32_t destination;
    interlocked_exchange(&destination, INT32_MAX);
    int32_t comperand = INT32_MIN;
    int32_t exchange = INT32_MIN;

    ///act
    int32_t return_val = interlocked_compare_exchange(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, interlocked_or(&destination, 0), "*destination is not equal to the original value.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect");
}

#ifdef _WIN64
/*Tests_SRS_INTERLOCKED_43_007: [ interlocked_compare_exchange_128 shall compare *destination with *comperand_result. If they are equal, destination[0] is set to exchange_low and destination[1] is set to exchange_high. These operations are performed atomically.]*/
/*Tests_SRS_INTERLOCKED_43_039: [ interlocked_compare_exchange_128 shall store the initial value of *destination in *comperand_result regardless of the result of the comparison.` ]*/
/*Tests_SRS_INTERLOCKED_43_038: [ interlocked_compare_exchange_128 shall return true if *comperand_result equals the original value of *destination.]*/
TEST_FUNCTION(interlocked_compare_exchange_128_exchanges_when_equal)
{
    ///arrange
    volatile int64_t* destination;
    interlocked_exchange_pointer((void* volatile*)&destination, malloc(2 * sizeof(int64_t)));
    int64_t* comperand_result = (int64_t*)malloc(2 * sizeof(int64_t));

    destination[0] = INT64_MAX;
    destination[1] = INT64_MIN;
    comperand_result[0] = INT64_MAX;
    comperand_result[1] = INT64_MIN;

    int64_t exchange_high = 2;
    int64_t exchange_low = 3;

    ///act
    bool return_val = interlocked_compare_exchange_128(destination, exchange_high, exchange_low, comperand_result);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, 3, interlocked_or_64(&destination[0], 0), "destination[0] not equal to exchange_low.");
    ASSERT_ARE_EQUAL(int64_t, 2, interlocked_or_64(&destination[1], 0), "destination[1] not equal to exchange_high.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, comperand_result[0], "comperand_result[0] is not equal to destination[0].");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, comperand_result[1], "comperand_result[1] is not equal to destination[1].");
    ASSERT_IS_TRUE(return_val, "Return value is incorrect");

    ///cleanup
    free((void*)destination);
    free(comperand_result);
}


/*Tests_SRS_INTERLOCKED_43_039: [ interlocked_compare_exchange_128 shall store the initial value of *destination in *comperand_result regardless of the result of the comparison.` ]*/
/*Tests_SRS_INTERLOCKED_43_063: [ interlocked_compare_exchange_128 shall return false if *comperand_result does not equal the original value of *destination. ]*/
TEST_FUNCTION(interlocked_compare_exchange_128_does_not_exchange_when_not_equal)
{
    ///arrange
    volatile int64_t* destination;
    interlocked_exchange_pointer((void* volatile*)&destination, malloc(2 * sizeof(int64_t)));
    int64_t* comperand_result = (int64_t*)malloc(2 * sizeof(int64_t));

    destination[0] = INT64_MAX;
    destination[1] = INT64_MIN;
    comperand_result[0] = INT64_MIN;
    comperand_result[1] = INT64_MAX;

    int64_t exchange_high = 2;
    int64_t exchange_low = 3;

    ///act
    bool return_val = interlocked_compare_exchange_128(destination, exchange_high, exchange_low, comperand_result);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, interlocked_or_64(&destination[0], 0), "destination[0] is not equal to original value.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, interlocked_or_64(&destination[1], 0), "destination[1] is not equal to original value.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, comperand_result[0], "comperand_result[0] is not equal to destination[0].");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, comperand_result[1], "comperand_result[1] is not equal to destination[1].");
    ASSERT_IS_FALSE(return_val, "Return value is incorrect");

    ///cleanup
    free((void*)destination);
    free((void*)comperand_result);
}
#endif

/*Tests_SRS_INTERLOCKED_43_009: [interlocked_compare_exchange_16 shall compare the 16-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
/*Tests_SRS_INTERLOCKED_43_040: [ interlocked_compare_exchange_16 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_compare_exchange_16_exchanges_when_equal)
{
    ///arrange
    volatile int16_t destination;
    interlocked_exchange_16(&destination, INT16_MAX);
    int16_t comperand = INT16_MAX;
    int16_t exchange = INT16_MIN;

    ///act
    int16_t return_val = interlocked_compare_exchange_16(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, interlocked_or_16(&destination, 0), "*destination is not equal to exchange.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_009: [interlocked_compare_exchange_16 shall compare the 16-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
/*Tests_SRS_INTERLOCKED_43_040: [ interlocked_compare_exchange_16 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_compare_exchange_16_does_not_exchange_when_not_equal)
{
    ///arrange
    volatile int16_t destination;
    interlocked_exchange_16(&destination, INT16_MAX);
    int16_t comperand = INT16_MIN;
    int16_t exchange = INT16_MIN;

    ///act
    int16_t return_val = interlocked_compare_exchange_16(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, interlocked_or_16(&destination, 0), "*destination is not equal to the original value.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_008: [interlocked_compare_exchange_64 shall compare the 64-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
/*Tests_SRS_INTERLOCKED_43_041: [ interlocked_compare_exchange_64 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_compare_exchange_64_exchanges_when_equal)
{
    ///arrange
    volatile int64_t destination;
    interlocked_exchange_64(&destination, INT64_MAX);
    int64_t comperand = INT64_MAX;
    int64_t exchange = INT64_MIN;

    ///act
    int64_t return_val = interlocked_compare_exchange_64(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, interlocked_or_64(&destination, 0), "*destination is not equal to exchange.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_008: [interlocked_compare_exchange_64 shall compare the 64-bit integers pointed to by destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
/*Tests_SRS_INTERLOCKED_43_041: [ interlocked_compare_exchange_64 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_compare_exchange_64_does_not_exchange_when_not_equal)
{
    ///arrange
    volatile int64_t destination;
    interlocked_exchange_64(&destination, INT64_MAX);
    int64_t comperand = INT64_MIN;
    int64_t exchange = INT64_MIN;

    ///act
    int64_t return_val = interlocked_compare_exchange_64(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, interlocked_or_64(&destination, 0), "*destination is not equal to the original value.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_010: [interlocked_compare_exchange_pointer shall compare the pointers destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
/*Tests_SRS_INTERLOCKED_43_042: [ interlocked_compare_exchange_pointer shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_compare_exchange_pointer_exchanges_when_equal)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile destination;
    interlocked_exchange_pointer(&destination, &value1);
    void* comperand = &value1;
    void* exchange = &value2;

    ///act
    void* return_val = interlocked_compare_exchange_pointer(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, &value2, interlocked_compare_exchange_pointer(&destination, NULL, NULL), "*destination is not equal to exchange.");
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_010: [interlocked_compare_exchange_pointer shall compare the pointers destination and comperand. If they are equal, *destination is set to exchange. These operations are performed atomically.]*/
/*Tests_SRS_INTERLOCKED_43_042: [ interlocked_compare_exchange_pointer shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_compare_exchange_pointer_does_not_exchange_when_not_equal)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile destination;
    interlocked_exchange_pointer(&destination, &value1);
    void* comperand = &value2;
    void* exchange = &value2;

    ///act
    void* return_val = interlocked_compare_exchange_pointer(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, &value1, interlocked_compare_exchange_pointer(&destination, NULL, NULL), "*destination is not equal to original value.");
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_011: [ interlocked_decrement shall atomically decrement (decrease by one) the 32-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_043: [ interlocked_decrement shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_upper_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MAX);

    ///act
    int32_t return_val = interlocked_decrement(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX - 1, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX - 1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_011: [ interlocked_decrement shall atomically decrement (decrease by one) the 32-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_043: [ interlocked_decrement shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_lower_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MIN + 1);

    ///act
    int32_t return_val = interlocked_decrement(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect");

}

TEST_FUNCTION(interlocked_decrement_underflows_lower_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MIN);

    ///act
    int32_t return_val = interlocked_decrement(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect");

}

/*Tests_SRS_INTERLOCKED_43_012: [ interlocked_decrement_16 shall atomically decrement (decrease by one) the 16-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_044: [ interlocked_decrement_16 shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_16_upper_bound)
{
    ///arrange
    volatile int16_t addend;
    interlocked_exchange_16(&addend, INT16_MAX);

    ///act
    int16_t return_val = interlocked_decrement_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX - 1, interlocked_or_16(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX - 1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_012: [ interlocked_decrement_16 shall atomically decrement (decrease by one) the 16-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_044: [ interlocked_decrement_16 shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_16_lower_bound)
{
    ///arrange
    volatile int16_t addend;
    interlocked_exchange_16(&addend, INT16_MIN + 1);

    ///act
    int16_t return_val = interlocked_decrement_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, interlocked_or_16(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_012: [ interlocked_decrement_16 shall atomically decrement (decrease by one) the 16-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_044: [ interlocked_decrement_16 shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_16_underflows_lower_bound)
{
    ///arrange
    volatile int16_t addend;
    interlocked_exchange_16(&addend, INT16_MIN);

    ///act
    int16_t return_val = interlocked_decrement_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, interlocked_or_16(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_013: [ interlocked_decrement_64 shall atomically decrement (decrease by one) the 64-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_045: [ interlocked_decrement_64 shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_64_upper_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MAX);

    ///act
    int64_t return_val = interlocked_decrement_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX - 1, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX - 1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_013: [ interlocked_decrement_64 shall atomically decrement (decrease by one) the 64-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_045: [ interlocked_decrement_64 shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_64_lower_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MIN + 1);

    ///act
    int64_t return_val = interlocked_decrement_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_013: [ interlocked_decrement_64 shall atomically decrement (decrease by one) the 64-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_045: [ interlocked_decrement_64 shall return the decremented value. ]*/
TEST_FUNCTION(interlocked_decrement_64_underflows_lower_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MIN);

    ///act
    int64_t return_val = interlocked_decrement_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_014: [ interlocked_exchange shall set the 32-bit variable pointed to by target to value as an atomic operation.]*/
/*Tests_SRS_INTERLOCKED_43_046: [ interlocked_exchange shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_sets_target)
{
    ///arrange
    volatile int32_t target;
    interlocked_exchange(&target, INT32_MIN);
    int32_t value = INT32_MAX;

    ///act
    int32_t return_val = interlocked_exchange(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, interlocked_or(&target, 0), "Result stored in *target is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_015: [ interlocked_exchange_16 shall set the 16-bit variable pointed to by target to value as an atomic operation.]*/
/*Tests_SRS_INTERLOCKED_43_047: [ interlocked_exchange_16 shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_16_sets_target)
{
    ///arrange
    volatile int16_t target;
    interlocked_exchange_16(&target, INT16_MIN);
    int16_t value = INT16_MAX;

    ///act
    int16_t return_val = interlocked_exchange_16(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, interlocked_or_16(&target, 0), "Result stored in *target is incorrect.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_016: [ interlocked_exchange_64 shall set the 64-bit variable pointed to by target to value as an atomic operation.]*/
/*Tests_SRS_INTERLOCKED_43_048: [ interlocked_exchange_64 shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_64_sets_target)
{
    ///arrange
    volatile int64_t target;
    interlocked_exchange_64(&target, INT64_MIN);
    int64_t value = INT64_MAX;

    ///act
    int64_t return_val = interlocked_exchange_64(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, interlocked_or_64(&target, 0), "Result stored in *target is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_017: [ interlocked_exchange_8 shall set the 8-bit variable pointed to by target to value as an atomic operation.]*/
/*Tests_SRS_INTERLOCKED_43_049: [ interlocked_exchange_8 shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_8_sets_target)
{
    ///arrange
    volatile int8_t target;
    interlocked_exchange_8(&target, INT8_MIN);
    int8_t value = INT8_MAX;

    ///act
    int8_t return_val = interlocked_exchange_8(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(int8_t, INT8_MAX, interlocked_or_8(&target, 0), "Result stored in *target is incorrect.");
    ASSERT_ARE_EQUAL(int8_t, INT8_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_018: [ interlocked_exchange_add shall perform an atomic addition of the 32-bit values *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_050: [ interlocked_exchange_add shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_sets_target_to_sum)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MIN);
    int32_t value = INT32_MAX;

    ///act
    int32_t return_val = interlocked_exchange_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, -1, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_018: [ interlocked_exchange_add shall perform an atomic addition of the 32-bit values *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_050: [ interlocked_exchange_add shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_overflows_upper_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MAX);
    int32_t value = 1;

    ///act
    int32_t return_val = interlocked_exchange_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_018: [ interlocked_exchange_add shall perform an atomic addition of the 32-bit values *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_050: [ interlocked_exchange_add shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_underflows_lower_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MIN);
    int32_t value = -1;

    ///act
    int32_t return_val = interlocked_exchange_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_019: [ interlocked_exchange_add_64 shall perform an atomic addition of the 64-bit values *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_064: [ interlocked_exchange_add_64 shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_64_sets_target_to_sum)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MIN);
    int64_t value = INT64_MAX;

    ///act
    int64_t return_val = interlocked_exchange_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, -1, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_019: [ interlocked_exchange_add_64 shall perform an atomic addition of the 64-bit values *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_064: [ interlocked_exchange_add_64 shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_64_overflows_upper_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MAX);
    int64_t value = 1;

    ///act
    int64_t return_val = interlocked_exchange_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_019: [ interlocked_exchange_add_64 shall perform an atomic addition of the 64-bit values *addend and value and store the result in *addend.]*/
/*Tests_SRS_INTERLOCKED_43_064: [ interlocked_exchange_add_64 shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_64_underflows_lower_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MIN);
    int64_t value = -1;

    ///act
    int64_t return_val = interlocked_exchange_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_020: [ interlocked_exchange_pointer shall atomically set *target to value]*/
/*Tests_SRS_INTERLOCKED_43_051: [ interlocked_exchange_pointer shall return the initial value of *target. ]*/
TEST_FUNCTION(interlocked_exchange_pointer_sets_target)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile target = &value1;
    void* value = &value2;

    ///act
    void* return_val = interlocked_exchange_pointer(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, &value2, interlocked_compare_exchange_pointer(&target, NULL, NULL));
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_021: [ interlocked_increment shall atomically increment (increase by one) the 32-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_052: [ interlocked_increment shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_upper_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MAX-1);

    ///act
    int32_t return_val = interlocked_increment(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_021: [ interlocked_increment shall atomically increment (increase by one) the 32-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_052: [ interlocked_increment shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_lower_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MIN);

    ///act
    int32_t return_val = interlocked_increment(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN + 1, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN + 1, return_val, "Return value is incorrect");

}

/*Tests_SRS_INTERLOCKED_43_021: [ interlocked_increment shall atomically increment (increase by one) the 32-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_052: [ interlocked_increment shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_overflows_upper_bound)
{
    ///arrange
    volatile int32_t addend;
    interlocked_exchange(&addend, INT32_MAX);

    ///act
    int32_t return_val = interlocked_increment(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, interlocked_or(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_022: [ interlocked_increment_16 shall atomically increment (increase by one) the 16-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_053: [ interlocked_increment_16 shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_16_upper_bound)
{
    ///arrange
    volatile int16_t addend;
    interlocked_exchange_16(&addend, INT16_MAX - 1);

    ///act
    int16_t return_val = interlocked_increment_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, interlocked_or_16(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_022: [ interlocked_increment_16 shall atomically increment (increase by one) the 16-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_053: [ interlocked_increment_16 shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_16_lower_bound)
{
    ///arrange
    volatile int16_t addend;
    interlocked_exchange_16(&addend, INT16_MIN);

    ///act
    int16_t return_val = interlocked_increment_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN + 1, interlocked_or_16(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN + 1, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_022: [ interlocked_increment_16 shall atomically increment (increase by one) the 16-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_053: [ interlocked_increment_16 shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_16_overflows_upper_bound)
{
    ///arrange
    volatile int16_t addend;
    interlocked_exchange_16(&addend, INT16_MAX);

    ///act
    int16_t return_val = interlocked_increment_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, interlocked_or_16(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_023: [ interlocked_increment_64 shall atomically increment (increase by one) the 64-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_054: [ interlocked_increment_64 shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_64_upper_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MAX - 1);

    ///act
    int64_t return_val = interlocked_increment_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_023: [ interlocked_increment_64 shall atomically increment (increase by one) the 64-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_054: [ interlocked_increment_64 shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_64_lower_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MIN);

    ///act
    int64_t return_val = interlocked_increment_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN + 1, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN + 1, return_val, "Return value is incorrect");

}

/*Tests_SRS_INTERLOCKED_43_023: [ interlocked_increment_64 shall atomically increment (increase by one) the 64-bit variable *addend.]*/
/*Tests_SRS_INTERLOCKED_43_054: [ interlocked_increment_64 shall return the incremented value. ]*/
TEST_FUNCTION(interlocked_increment_64_overflows_upper_bound)
{
    ///arrange
    volatile int64_t addend;
    interlocked_exchange_64(&addend, INT64_MAX);

    ///act
    int64_t return_val = interlocked_increment_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, interlocked_or_64(&addend, 0), "Result stored in *addend is incorrect.");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_024: [ interlocked_or shall perform an atomic bitwise OR operation on the 32-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_055: [ interlocked_or shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_does_bitwise_or)
{
    ///arrange
    volatile uint32_t destination;
    interlocked_exchange((volatile int32_t*)&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0F0F;

    ///act
    uint32_t return_val = (uint32_t)interlocked_or((volatile int32_t*)&destination, (int32_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, 0xFFFFFFFF, interlocked_or((volatile int32_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_025: [ interlocked_or_16 shall perform an atomic bitwise OR operation on the 16-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_056: [ interlocked_or_16 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_16_does_bitwise_or)
{
    ///arrange
    volatile uint16_t destination;
    interlocked_exchange_16((volatile int16_t*)&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0F0F;

    ///act
    uint16_t return_val = (uint16_t)interlocked_or_16((volatile int16_t*)&destination, (int16_t)value);


    ///assert
    ASSERT_ARE_EQUAL(uint16_t, 0xFFFF, interlocked_or_16((volatile int16_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_026: [ interlocked_or_64 shall perform an atomic bitwise OR operation on the 64-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_057: [ interlocked_or_64 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_64_does_bitwise_or)
{
    ///arrange
    volatile uint64_t destination;
    interlocked_exchange_64((volatile int64_t*)&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;

    ///act
    uint64_t return_val = (uint64_t)interlocked_or_64((volatile int64_t*)&destination, (int64_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint64_t, 0xFFFFFFFFFFFFFFFF, interlocked_or_64((volatile int64_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_027: [ interlocked_or_8 shall perform an atomic bitwise OR operation on the 8-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_058: [ interlocked_or_8 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_8_does_bitwise_or)
{
    ///arrange
    volatile uint8_t destination;
    interlocked_exchange_8((volatile int8_t*)&destination, (uint8_t)0xF0);
    uint8_t value = 0x0F;

    ///act
    uint8_t return_val = (uint8_t)interlocked_or_8((volatile int8_t*)&destination, (int8_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint8_t, 0xFF, interlocked_or_8((volatile int8_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_028: [ interlocked_xor shall perform an atomic bitwise XOR operation on the 32-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_059: [ interlocked_xor shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_does_bitwise_xor)
{
    ///arrange
    volatile uint32_t destination;
    interlocked_exchange((volatile int32_t*)&destination, (uint32_t)0xF0F0F0FF);
    uint32_t value = 0x0F0F0F0F;

    ///act
    uint32_t return_val = (uint32_t)interlocked_xor((volatile int32_t*)&destination, (int32_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, 0xFFFFFFF0, interlocked_or((volatile int32_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0FF, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_029: [ interlocked_xor_16 shall perform an atomic bitwise XOR operation on the 16-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_060: [ interlocked_xor_16 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_16_does_bitwise_xor)
{
    ///arrange
    volatile uint16_t destination;
    interlocked_exchange_16((volatile int16_t*)&destination, (uint16_t)0xF0FF);
    uint16_t value = 0x0F0F;

    ///act
    uint16_t return_val = (uint16_t)interlocked_xor_16((volatile int16_t*)&destination, (int16_t)value);


    ///assert
    ASSERT_ARE_EQUAL(uint16_t, 0xFFF0, interlocked_or_16((volatile int16_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0FF, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_030: [ interlocked_xor_64 shall perform an atomic bitwise XOR operation on the 64-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_061: [ interlocked_xor_64 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_64_does_bitwise_xor)
{
    ///arrange
    volatile uint64_t destination;
    interlocked_exchange_64((volatile int64_t*)&destination, (uint64_t)0xF0F0F0F0F0F0F0FF);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;

    ///act
    uint64_t return_val = (uint64_t)interlocked_xor_64((volatile int64_t*)&destination, (int64_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint64_t, 0xFFFFFFFFFFFFFFF0, interlocked_or_64((volatile int64_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0FF, return_val, "Return value is incorrect");
}

/*Tests_SRS_INTERLOCKED_43_031: [ interlocked_xor_8 shall perform an atomic bitwise XOR operation on the 8-bit integers *destination and value and store the result in destination.]*/
/*Tests_SRS_INTERLOCKED_43_062: [ interlocked_xor_8 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_8_does_bitwise_xor)
{
    ///arrange
    volatile uint8_t destination;
    interlocked_exchange_8((volatile int8_t*)&destination, (uint8_t)0xFF);
    uint8_t value = 0x0F;

    ///act
    uint8_t return_val = (uint8_t)interlocked_xor_8((volatile int8_t*)&destination, (int8_t)value);

    ///assert
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, interlocked_or_8((volatile int8_t*)&destination, 0), "Result stored in *destination is incorrect.");
    ASSERT_ARE_EQUAL(uint8_t, 0xFF, return_val, "Return value is incorrect");
}

END_TEST_SUITE(interlocked_int)
