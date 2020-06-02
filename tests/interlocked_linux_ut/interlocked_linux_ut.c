// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#include <cstdatomic>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#endif
#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#include "azure_c_util/interlocked.h"


static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#define ENABLE_MOCKS
#include "mock_interlocked.h"
#undef ENABLE_MOCKS

static int16_t expected_atomic_compare_exchange_16_value;
static bool hook_mock_atomic_compare_exchange_16(volatile_atomic int16_t* object, int16_t* expected, int16_t desired)
{
    ASSERT_ARE_EQUAL(int16_t, expected_atomic_compare_exchange_16_value, *expected);
    int16_t original_expected = *expected;
    *expected = *object;
    return original_expected == *object;
}

static int32_t expected_atomic_compare_exchange_32_value;
static bool hook_mock_atomic_compare_exchange_32(volatile_atomic int32_t* object, int32_t* expected, int32_t desired)
{
    ASSERT_ARE_EQUAL(int32_t, expected_atomic_compare_exchange_32_value, *expected);
    int32_t original_expected = *expected;
    *expected = *object;
    return original_expected == *object;
}

static int64_t expected_atomic_compare_exchange_64_value;
static bool hook_mock_atomic_compare_exchange_64(volatile_atomic int64_t* object, int64_t* expected, int64_t desired)
{
    ASSERT_ARE_EQUAL(int64_t, expected_atomic_compare_exchange_64_value, *expected);
    int64_t original_expected = *expected;
    *expected = *object;
    return original_expected == *object;
}

static void* expected_atomic_compare_exchange_pointer_value;
static bool hook_mock_atomic_compare_exchange_pointer(void* volatile_atomic* object, void** expected, void* desired)
{
    ASSERT_ARE_EQUAL(void_ptr, expected_atomic_compare_exchange_pointer_value, *expected);
    void* original_expected = *expected;
    *expected = *object;
    return original_expected == *object;
}

BEGIN_TEST_SUITE(interlocked_linux_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    REGISTER_GLOBAL_MOCK_HOOK(mock_atomic_compare_exchange_16, hook_mock_atomic_compare_exchange_16);
    REGISTER_GLOBAL_MOCK_HOOK(mock_atomic_compare_exchange_32, hook_mock_atomic_compare_exchange_32);
    REGISTER_GLOBAL_MOCK_HOOK(mock_atomic_compare_exchange_64, hook_mock_atomic_compare_exchange_64);
    REGISTER_GLOBAL_MOCK_HOOK(mock_atomic_compare_exchange_pointer, hook_mock_atomic_compare_exchange_pointer);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}


/*Tests_SRS_INTERLOCKED_LINUX_43_001: [ interlocked_add shall call atomic_fetch_add with addend as object and value as operand.]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_002: [ interlocked_add shall return the initial value of *addend plus value. ]*/
TEST_FUNCTION(interlocked_add_calls_atomic_fetch_add)
{
    ///arrange
    volatile_atomic int32_t addend;
    atomic_exchange(&addend, INT32_MAX);
    int32_t value = INT32_MIN;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_add_32(&addend, value))
        .SetReturn(INT32_MAX);

    ///act
    int32_t return_val = interlocked_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_064: [ interlocked_add_64 shall call atomic_fetch_add with addend as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_065: [ interlocked_add_64 shall return the initial value of *addend plus value. ]*/
TEST_FUNCTION(interlocked_add_64_calls_atomic_fetch_add)
{
    ///arrange
    volatile_atomic int64_t addend;
    atomic_exchange(&addend, INT64_MAX);
    int64_t value = INT64_MIN;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_add_64(&addend, value))
        .SetReturn(INT64_MAX);

    ///act
    int64_t return_val = interlocked_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_003: [ interlocked_and shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_004: [ interlocked_and shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_and_calls_atomic_fetch_and)
{
    ///arrange
    volatile_atomic uint32_t destination;
    atomic_exchange(&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_and_32(&destination, value))
        .SetReturn(0xF0F0F0F0);

    ///act
    uint32_t return_val = (uint32_t)interlocked_and((volatile_atomic int32_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_005: [ interlocked_and_16 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_006: [ interlocked_and_16 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_and_16_calls_atomic_fetch_and)
{
    ///arrange
    volatile_atomic uint16_t destination;
    atomic_exchange(&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_and_16(&destination, value))
        .SetReturn(0xF0F0);

    ///act
    uint16_t return_val = (uint16_t)interlocked_and_16((volatile_atomic int16_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_007: [ interlocked_and_64 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_008: [ interlocked_and_64 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_and_64_calls_atomic_fetch_and)
{
    ///arrange
    volatile_atomic uint64_t destination;
    atomic_exchange(&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_and_64(&destination, value))
        .SetReturn(0xF0F0F0F0F0F0F0F0);

    ///act
    uint64_t return_val = (uint64_t)interlocked_and_64((volatile_atomic int64_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_009: [ interlocked_and_8 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_010: [ interlocked_and_8 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_and_8_calls_atomic_fetch_and)
{
    ///arrange
    volatile_atomic uint8_t destination;
    atomic_exchange(&destination, (uint8_t)0xF0);
    uint8_t value = 0x0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_and_8(&destination, value))
        .SetReturn(0xF0);

    ///act
    uint8_t return_val = (uint8_t)interlocked_and_8((volatile_atomic int8_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_011: [ interlocked_compare_exchange shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_012: [ interlocked_compare_exchange shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_calls_atomic_compare_exchange_with_destination_equal_to_comperand)
{
    ///arrange
    volatile_atomic int32_t destination;
    atomic_exchange(&destination, INT32_MAX);
    int32_t comperand = INT32_MAX;
    int32_t exchange = INT32_MIN;
    expected_atomic_compare_exchange_32_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_32(&destination, IGNORED_ARG, exchange));

    ///act
    int32_t return_val = interlocked_compare_exchange(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_011: [ interlocked_compare_exchange shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_012: [ interlocked_compare_exchange shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_calls_atomic_compare_exchange_with_destination_not_equal_to_comperand)
{
    ///arrange
    volatile_atomic int32_t destination;
    atomic_exchange(&destination, INT32_MAX);
    int32_t comperand = INT32_MAX - 1;
    int32_t exchange = INT32_MIN;
    expected_atomic_compare_exchange_32_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_32(&destination, IGNORED_ARG, exchange));

    ///act
    int32_t return_val = interlocked_compare_exchange(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_069: [ interlocked_compare_exchange_16 shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_070: [ interlocked_compare_exchange_16 shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_16_calls_atomic_compare_exchange_with_destination_equal_to_comperand)
{
    ///arrange
    volatile_atomic int16_t destination;
    atomic_exchange(&destination, INT16_MAX);
    int16_t comperand = INT16_MAX;
    int16_t exchange = INT16_MIN;
    expected_atomic_compare_exchange_16_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_16(&destination, IGNORED_ARG, exchange));

    ///act
    int16_t return_val = interlocked_compare_exchange_16(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_069: [ interlocked_compare_exchange_16 shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_070: [ interlocked_compare_exchange_16 shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_16_calls_atomic_compare_exchange_with_destination_not_equal_to_comperand)
{
    ///arrange
    volatile_atomic int16_t destination;
    atomic_exchange(&destination, INT16_MAX);
    int16_t comperand = INT16_MAX - 1;
    int16_t exchange = INT16_MIN;
    expected_atomic_compare_exchange_16_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_16(&destination, IGNORED_ARG, exchange));

    ///act
    int16_t return_val = interlocked_compare_exchange_16(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_074: [ interlocked_compare_exchange_64 shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_075: [ interlocked_compare_exchange_64 shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_64_calls_atomic_compare_exchange_with_destination_equal_to_comperand)
{
    ///arrange
    volatile_atomic int64_t destination;
    atomic_exchange(&destination, INT64_MAX);
    int64_t comperand = INT64_MAX;
    int64_t exchange = INT64_MIN;
    expected_atomic_compare_exchange_64_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_64(&destination, IGNORED_ARG, exchange));

    ///act
    int64_t return_val = interlocked_compare_exchange_64(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_074: [ interlocked_compare_exchange_64 shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_075: [ interlocked_compare_exchange_64 shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_64_calls_atomic_compare_exchange_with_destination_not_equal_to_comperand)
{
    ///arrange
    volatile_atomic int64_t destination;
    atomic_exchange(&destination, INT64_MAX);
    int64_t comperand = INT64_MAX - 1;
    int64_t exchange = INT64_MIN;
    expected_atomic_compare_exchange_64_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_64(&destination, IGNORED_ARG, exchange));

    ///act
    int64_t return_val = interlocked_compare_exchange_64(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_079: [ interlocked_compare_exchange_pointer shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_080: [ interlocked_compare_exchange_pointer shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_pointer_calls_atomic_compare_exchange_with_destination_equal_to_comperand)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile_atomic destination;
    atomic_exchange(&destination, &value1);
    void* comperand = &value1;
    void* exchange = &value2;
    expected_atomic_compare_exchange_pointer_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_pointer(&destination, IGNORED_ARG, exchange));

    ///act
    void* return_val = interlocked_compare_exchange_pointer(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_079: [ interlocked_compare_exchange_pointer shall call atomic_compare_exchange_strong with destination as object, &comperand as expected and exchange as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_080: [ interlocked_compare_exchange_pointer shall return comperand. ]*/
TEST_FUNCTION(interlocked_compare_exchange_pointer_calls_atomic_compare_exchange_with_destination_not_equal_to_comperand)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile_atomic destination;
    atomic_exchange(&destination, &value1);
    void* comperand = &value2;
    void* exchange = &value2;
    expected_atomic_compare_exchange_pointer_value = comperand;
    STRICT_EXPECTED_CALL(mock_atomic_compare_exchange_pointer(&destination, IGNORED_ARG, exchange));

    ///act
    void* return_val = interlocked_compare_exchange_pointer(&destination, exchange, comperand);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_021: [ interlocked_decrement shall call atomic_fetch_sub with addend as object and 1 as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_022: [ interlocked_decrement shall return *addend minus 1. ]*/
TEST_FUNCTION(interlocked_decrement_calls_atomic_fetch_sub)
{
    ///arrange
    volatile_atomic int32_t addend;
    atomic_exchange(&addend, INT32_MAX);
    STRICT_EXPECTED_CALL(mock_atomic_fetch_sub_32(&addend, 1))
        .SetReturn(INT32_MAX);

    ///act
    int32_t return_val = interlocked_decrement(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MAX -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_023: [ interlocked_decrement_16 shall call atomic_fetch_sub with addend as object and 1 as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_024: [ interlocked_decrement_16 shall return *addend minus 1. ]*/
TEST_FUNCTION(interlocked_decrement_16_calls_atomic_fetch_sub)
{
    ///arrange
    volatile_atomic int16_t addend;
    atomic_exchange(&addend, INT16_MAX);
    STRICT_EXPECTED_CALL(mock_atomic_fetch_sub_16(&addend, 1))
        .SetReturn(INT16_MAX);

    ///act
    int16_t return_val = interlocked_decrement_16(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MAX -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_025: [ interlocked_decrement_64 shall call atomic_fetch_sub with addend as object and 1 as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_026: [ interlocked_decrement_64 shall return *addend minus 1. ]*/
TEST_FUNCTION(interlocked_decrement_64_calls_atomic_fetch_sub)
{
    ///arrange
    volatile_atomic int64_t addend;
    atomic_exchange(&addend, INT64_MAX);
    STRICT_EXPECTED_CALL(mock_atomic_fetch_sub_64(&addend, 1))
        .SetReturn(INT64_MAX);

    ///act
    int64_t return_val = interlocked_decrement_64(&addend);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MAX -1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_027: [ interlocked_exchange shall call atomic_exchange with target as object and value as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_028: [ interlocked_exchange shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_calls_atomic_exchange)
{
    ///arrange
    volatile_atomic int32_t target;
    atomic_exchange(&target, INT32_MIN);
    int32_t value = INT32_MAX;
    STRICT_EXPECTED_CALL(mock_atomic_exchange_32(&target, value))
        .SetReturn(INT32_MIN);

    ///act
    int32_t return_val = interlocked_exchange(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_029: [ interlocked_exchange_16 shall call atomic_exchange with target as object and value as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_030: [ interlocked_exchange_16 shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_16_calls_atomic_exchange)
{
    ///arrange
    volatile_atomic int16_t target;
    atomic_exchange(&target, INT16_MIN);
    int16_t value = INT16_MAX;
    STRICT_EXPECTED_CALL(mock_atomic_exchange_16(&target, value))
        .SetReturn(INT16_MIN);

    ///act
    int16_t return_val = interlocked_exchange_16(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int16_t, INT16_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_031: [ interlocked_exchange_64 shall call atomic_exchange with target as object and value as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_032: [ interlocked_exchange_64 shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_64_calls_atomic_exchange)
{
    ///arrange
    volatile_atomic int64_t target;
    atomic_exchange(&target, INT64_MIN);
    int64_t value = INT64_MAX;
    STRICT_EXPECTED_CALL(mock_atomic_exchange_64(&target, value))
        .SetReturn(INT64_MIN);

    ///act
    int64_t return_val = interlocked_exchange_64(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_033: [ interlocked_exchange_8 shall call atomic_exchange with target as object and value as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_034: [ interlocked_exchange_8 shall return the initial value pointed to by target. ]*/
TEST_FUNCTION(interlocked_exchange_8_calls_atomic_exchange)
{
    ///arrange
    volatile_atomic int8_t target;
    atomic_exchange(&target, INT8_MIN);
    int8_t value = INT8_MAX;
    STRICT_EXPECTED_CALL(mock_atomic_exchange_8(&target, value))
        .SetReturn(INT8_MIN);

    ///act
    int8_t return_val = interlocked_exchange_8(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int8_t, INT8_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_035: [ interlocked_exchange_add shall call atomic_fetch_add with addend as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_036: [ interlocked_exchange_add shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_calls_atomic_fetch_add)
{
    ///arrange
    volatile_atomic int32_t addend;
    atomic_exchange(&addend, INT32_MIN);
    int32_t value = INT32_MAX;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_add_32(&addend, value))
        .SetReturn(INT32_MIN);

    ///act
    int32_t return_val = interlocked_exchange_add(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int32_t, INT32_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_037: [ interlocked_exchange_add_64 shall call atomic_fetch_add with addend as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_038: [ interlocked_exchange_add_64 shall return the initial value of *addend. ]*/
TEST_FUNCTION(interlocked_exchange_add_64_calls_atomic_fetch_add)
{
    ///arrange
    volatile_atomic int64_t addend;
    atomic_exchange(&addend, INT64_MIN);
    int64_t value = INT64_MAX;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_add_64(&addend, value))
        .SetReturn(INT64_MIN);

    ///act
    int64_t return_val = interlocked_exchange_add_64(&addend, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(int64_t, INT64_MIN, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_039: [ interlocked_exchange_pointer shall call atomic_fetch_sub with target as object and value as desired. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_040: [interlocked_exchange_pointer shall return the initial address pointed to by the target parameter ]*/
TEST_FUNCTION(interlocked_exchange_pointer_calls_atomic_exchange)
{
    ///arrange
    int value1 = 1;
    int value2 = 2;
    void* volatile_atomic target = &value1;
    void* value = &value2;
    STRICT_EXPECTED_CALL(mock_atomic_exchange_pointer(&target, value))
        .SetReturn(&value1);

    ///act
    void* return_val = interlocked_exchange_pointer(&target, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(void_ptr, &value1, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_047: [ interlocked_or shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_048: [ interlocked_or shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_calls_atomic_fetch_or)
{
    ///arrange
    volatile_atomic uint32_t destination;
    atomic_exchange(&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_or_32(&destination, value))
        .SetReturn(0xF0F0F0F0);

    ///act
    uint32_t return_val = (uint32_t)interlocked_or((volatile_atomic int32_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_049: [ interlocked_or_16 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_050: [ interlocked_or_16 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_16_calls_atomic_fetch_or)
{
    ///arrange
    volatile_atomic uint16_t destination;
    atomic_exchange(&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_or_16(&destination, value))
        .SetReturn((uint16_t)0xF0F0);

    ///act
    uint16_t return_val = (uint16_t)interlocked_or_16((volatile_atomic int16_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_051: [ interlocked_or_64 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_052: [ interlocked_or_64 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_64_calls_atomic_fetch_or)
{
    ///arrange
    volatile_atomic uint64_t destination;
    atomic_exchange(&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_or_64(&destination, value))
        .SetReturn(0xF0F0F0F0F0F0F0F0);

    ///act
    uint64_t return_val = (uint64_t)interlocked_or_64((volatile_atomic int64_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_053: [ interlocked_or_8 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_054: [ interlocked_or_8 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_or_8_calls_atomic_fetch_or)
{
    ///arrange
    volatile_atomic uint8_t destination;
    atomic_exchange(&destination, (uint8_t)0xF0);
    uint8_t value = 0x0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_or_8(&destination, value))
        .SetReturn((uint8_t)0xF0);

    ///act
    uint8_t return_val = (uint8_t)interlocked_or_8((volatile_atomic int8_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_055: [ interlocked_xor shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_056: [ interlocked_xor shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_calls_atomic_fetch_xor)
{
    ///arrange
    volatile_atomic uint32_t destination;
    atomic_exchange(&destination, (uint32_t)0xF0F0F0F0);
    uint32_t value = 0x0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_xor_32(&destination, value))
        .SetReturn(0xF0F0F0F0);

    ///act
    uint32_t return_val = (uint32_t)interlocked_xor((volatile_atomic int32_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint32_t, 0xF0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_057: [ interlocked_xor_16 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_058: [ interlocked_xor_16 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_16_calls_atomic_fetch_xor)
{
    ///arrange
    volatile_atomic uint16_t destination;
    atomic_exchange(&destination, (uint16_t)0xF0F0);
    uint16_t value = 0x0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_xor_16(&destination, value))
        .SetReturn((uint16_t)0xF0F0);

    ///act
    uint16_t return_val = (uint16_t)interlocked_xor_16((volatile_atomic int16_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint16_t, 0xF0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_059: [ interlocked_xor_64 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_060: [ interlocked_xor_64 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_64_calls_atomic_fetch_xor)
{
    ///arrange
    volatile_atomic uint64_t destination;
    atomic_exchange(&destination, (uint64_t)0xF0F0F0F0F0F0F0F0);
    uint64_t value = 0x0F0F0F0F0F0F0F0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_xor_64(&destination, value))
        .SetReturn(0xF0F0F0F0F0F0F0F0);

    ///act
    uint64_t return_val = (uint64_t)interlocked_xor_64((volatile_atomic int64_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint64_t, 0xF0F0F0F0F0F0F0F0, return_val, "Return value is incorrect.");
}

/*Tests_SRS_INTERLOCKED_LINUX_43_061: [ interlocked_xor_8 shall call atomic_fetch_and with destination as object and value as operand. ]*/
/*Tests_SRS_INTERLOCKED_LINUX_43_062: [ interlocked_xor_8 shall return the initial value of *destination. ]*/
TEST_FUNCTION(interlocked_xor_8_calls_atomic_fetch_xor)
{
    ///arrange
    volatile_atomic uint8_t destination;
    atomic_exchange(&destination, (uint8_t)0xF0);
    uint8_t value = 0x0F;
    STRICT_EXPECTED_CALL(mock_atomic_fetch_xor_8(&destination, value))
        .SetReturn((uint8_t)0xF0);

    ///act
    uint8_t return_val = (uint8_t)interlocked_xor_8((volatile_atomic int8_t*)&destination, value);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(uint8_t, 0xF0, return_val, "Return value is incorrect.");
}
END_TEST_SUITE(interlocked_linux_unittests)
