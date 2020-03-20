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

#include "windows.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* s)
{
    free(s);
}

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

static TEST_MUTEX_HANDLE g_testByTest;

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"

#include "azure_c_util/interlocked_hl.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static LONG64 hook_InterlockedCompareExchange64(LONG64 volatile * Destination, LONG64 Exchange, LONG64 Comparand)
{
    LONG64 initialDestination = *Destination;
    if (*Destination == Comparand)
    {
        *Destination = Exchange;
    }
    return initialDestination;
}

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, LONG64, InterlockedCompareExchange64, LONG64 volatile *, Destination, LONG64, Exchange, LONG64, Comparand);

MOCK_FUNCTION_WITH_CODE(, LONG64, InterlockedExchange64, LONG64 volatile *, Destination, LONG64, Exchange)
LONG64 initialDestination = *Destination;
*Destination = Exchange;
MOCK_FUNCTION_END(initialDestination)

MOCK_FUNCTION_WITH_CODE(, LONG, InterlockedAdd, LONG volatile*, Addend, LONG, Value)
MOCK_FUNCTION_END(*Addend + Value)

MOCKABLE_FUNCTION(, LONG, InterlockedExchange, LONG volatile*, Addend, LONG, Value);
MOCKABLE_FUNCTION(, void, WakeByAddressSingle, PVOID, Address);

MOCK_FUNCTION_WITH_CODE(, LONGLONG, InterlockedAdd64, LONGLONG volatile *, Addend, LONGLONG, Value)
MOCK_FUNCTION_END(*Addend + Value)

MOCK_FUNCTION_WITH_CODE(, bool, TEST_IS_GREATER, LONG64, original_target, LONG64, exchange)
MOCK_FUNCTION_END(original_target<exchange)
#undef ENABLE_MOCKS

extern BOOL WaitOnAddress(
    volatile VOID * Address,
    PVOID CompareAddress,
    SIZE_T AddressSize,
    DWORD dwMilliseconds
);

MOCK_FUNCTION_WITH_CODE(, BOOL, WaitOnAddress, volatile VOID *, Address, PVOID, CompareAddress, SIZE_T, AddressSize, DWORD, dwMilliseconds)
BOOL my_result;
if (memcmp((void*)Address, CompareAddress, AddressSize) != 0)
{
    my_result = TRUE;
}
else
{
    my_result = FALSE;
}
MOCK_FUNCTION_END(my_result)

typedef  struct ADDEND_AND_VALUE_TAG
{ 
    volatile LONGLONG Addend; 
    LONGLONG Value; 
} ADDEND_AND_VALUE;

typedef  struct ADDEND_CEILING_AND_VALUE_TAG
{
    volatile LONGLONG Addend;
    LONGLONG Ceiling;
    LONGLONG Value;
} ADDEND_CEILING_AND_VALUE;

static void hook_WakeByAddressSingle(
    PVOID Address
)
{
    (void)Address;
}

static LONG hook_InterlockedExchange(
    LONG volatile* Target,
    LONG          Value
)
{
    LONG t = *Target;
    *Target = Value;
    return t;
}

TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

BEGIN_TEST_SUITE(interlocked_hl_win32_ut)


TEST_SUITE_INITIALIZE(a)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    (void)umocktypes_stdint_register_types();
   
    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
    REGISTER_UMOCK_ALIAS_TYPE(LONGLONG, int64_t);
    REGISTER_UMOCK_ALIAS_TYPE(LONG64, int64_t);
    REGISTER_UMOCK_ALIAS_TYPE(PVOID, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SIZE_T, size_t);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, unsigned long);
    REGISTER_UMOCK_ALIAS_TYPE(LONG, int32_t);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, long);

    REGISTER_GLOBAL_MOCK_HOOK(InterlockedCompareExchange64, hook_InterlockedCompareExchange64);
    REGISTER_GLOBAL_MOCK_HOOK(InterlockedExchange, hook_InterlockedExchange);
    REGISTER_GLOBAL_MOCK_HOOK(WakeByAddressSingle, hook_WakeByAddressSingle);

}

TEST_SUITE_CLEANUP(b)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(c)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(d)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_INTERLOCKED_HL_02_001: [ If Addend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_007: [ In all failure cases InterlockedHL_Add64WithCeiling shall not modify Addend or originalAddend*/
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_with_Addend_NULL_fails)
{
    ///arrange
    LONGLONG originalAddend=300;

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(NULL, 10, 4, &originalAddend);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(int64_t, 300, originalAddend);

    ///cleanup
}

/*Tests_SRS_INTERLOCKED_HL_02_006: [ If originalAddend is NULL then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_with_originalAddend_NULL_fails)
{
    ///arrange
    volatile LONGLONG Addend = 5;

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&Addend, 10, 4, NULL);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(int64_t, 5, Addend);

    ///cleanup
}

/*Tests_SRS_INTERLOCKED_HL_02_002: [ If Addend + Value would underflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_when_underflow_it_fails)
{
    ///arrange
    LONGLONG originalAddend = 400;
    ADDEND_AND_VALUE inputValues[] =
    {
        /*Addend*/              /*Value*/
        { LLONG_MIN,            -1},
        { LLONG_MIN + 1,        -2},
        { -1,                   LLONG_MIN },
        { LLONG_MIN,            LLONG_MIN}
    }, *cloneOfInputValues;

    cloneOfInputValues = (ADDEND_AND_VALUE*)my_gballoc_malloc(sizeof(inputValues));
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {

        ///arrange
        STRICT_EXPECTED_CALL(InterlockedAdd64(&(inputValues[i].Addend), 0));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), LLONG_MAX, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
        ASSERT_ARE_EQUAL(int64_t, 400, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }
    
    ///cleanup
    my_gballoc_free(cloneOfInputValues);
}

/*Tests_SRS_INTERLOCKED_HL_02_003: [ If Addend + Value would overflow then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_when_overflow_it_fails)
{
    ///arrange
    LONGLONG originalAddend = 500;
    ADDEND_AND_VALUE inputValues[] =
    {
        /*Addend*/              /*Value*/
        { LLONG_MAX,            1 },
        { LLONG_MAX -1,         2 },
        { 1,                    LLONG_MAX },
        { LLONG_MAX,            LLONG_MAX }
    }, *cloneOfInputValues;

    cloneOfInputValues = (ADDEND_AND_VALUE*)my_gballoc_malloc(sizeof(inputValues));
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {
        ///arrange
        STRICT_EXPECTED_CALL(InterlockedAdd64(&(inputValues[i].Addend), 0));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), LLONG_MAX, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
        ASSERT_ARE_EQUAL(int64_t, 500, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    ///cleanup
    my_gballoc_free(cloneOfInputValues);
}

/*Tests_SRS_INTERLOCKED_HL_02_004: [ If Addend + Value would be greater than Ceiling then InterlockedHL_Add64WithCeiling shall fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_when_over_the_ceiling_it_fails)
{
    ///arrange
    LONGLONG originalAddend = 600;
    ADDEND_CEILING_AND_VALUE inputValues[] =
    {
        /*Addend*/              /*Ceiling*/             /*Value*/
        { LLONG_MAX - 2,        LLONG_MAX - 1,          2 },
        { 2,                    LLONG_MAX - 1,          LLONG_MAX - 2 },

        { -1,                   0,                      2 },
        { 2,                    0,                      -1 },

        { LLONG_MIN,            LLONG_MIN + 1,          2 },
        { 2,                    LLONG_MIN + 1,          LLONG_MIN },
    }, *cloneOfInputValues;

    cloneOfInputValues = (ADDEND_CEILING_AND_VALUE*)my_gballoc_malloc(sizeof(inputValues));
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {
        ///arrange
        STRICT_EXPECTED_CALL(InterlockedAdd64(&(inputValues[i].Addend), 0));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), inputValues[i].Ceiling, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
        ASSERT_ARE_EQUAL(int64_t, 600, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    ///cleanup
    my_gballoc_free(cloneOfInputValues);
}

/*Tests_SRS_INTERLOCKED_HL_02_005: [ Otherwise, InterlockedHL_Add64WithCeiling shall atomically write in Addend the sum of Addend and Value, succeed and return INTERLOCKED_HL_OK. ]*/
TEST_FUNCTION(InterlockedHL_Add64WithCeiling_succeeds)
{
    ///arrange
    LONGLONG originalAddend = 700;
    ADDEND_CEILING_AND_VALUE inputValues[] = 
    {
        /*Addend*/              /*Ceiling*/             /*Value*/
        { LLONG_MAX - 2,        LLONG_MAX - 1,          1 },
        { 1,                    LLONG_MAX - 1,          LLONG_MAX - 2 },

        { -1,                   0,                      1 },
        { 1,                    0,                      -1 },

        { LLONG_MIN,            LLONG_MIN + 1,          1 },
        { 1,                    LLONG_MIN + 1,          LLONG_MIN },
    }, *cloneOfInputValues;

    cloneOfInputValues = (ADDEND_CEILING_AND_VALUE*)my_gballoc_malloc(sizeof(inputValues));
    (void)memcpy(cloneOfInputValues, inputValues, sizeof(inputValues));

    for (size_t i = 0; i < sizeof(inputValues) / sizeof(inputValues[0]); i++)
    {

        ///arrange
        STRICT_EXPECTED_CALL(InterlockedAdd64(&(inputValues[i].Addend), 0));
        STRICT_EXPECTED_CALL(InterlockedCompareExchange64(&(inputValues[i].Addend), IGNORED_ARG, inputValues[i].Addend));

        ///act
        INTERLOCKED_HL_RESULT result = InterlockedHL_Add64WithCeiling(&(inputValues[i].Addend), inputValues[i].Ceiling, inputValues[i].Value, &originalAddend);

        ///assert
        ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend, originalAddend);
        ASSERT_ARE_EQUAL(int64_t, cloneOfInputValues[i].Addend+ cloneOfInputValues[i].Value, inputValues[i].Addend);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    ///cleanup
    my_gballoc_free(cloneOfInputValues);
}

/* InterlockedHL_WaitForValue */

/* Tests_SRS_INTERLOCKED_HL_01_002: [ If address is NULL, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedHL_WaitForValue_with_NULL_address_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;

    // act
    result = InterlockedHL_WaitForValue(NULL, 0x42, INFINITE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_003: [ If the value at address is equal to value, InterlockedHL_WaitForValue shall return INTERLOCKED_HL_OK. ]*/
TEST_FUNCTION(when_the_value_equals_target_value_InterlockedHL_WaitForValue_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    LONG value = 0x42;

    STRICT_EXPECTED_CALL(InterlockedAdd(&value, 0));

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, INFINITE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_004: [ If the value at address is not equal to value, InterlockedHL_WaitForValue shall wait until the value at address changes in order to compare it again to value by using WaitOnAddress. ]*/
/* Tests_SRS_INTERLOCKED_HL_01_005: [ When waiting for the value at address to change, the milliseconds argument value shall be used as timeout. ]*/
/* Tests_SRS_INTERLOCKED_HL_01_007: [ When WaitOnAddress succeeds, the value at address shall be compared to the target value passed in value by using InterlockedAdd. ]*/
TEST_FUNCTION(when_the_value_equals_target_value_after_waiting_InterlockedHL_WaitForValue_returns_OK)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    LONG value = 0x41;
    LONG target_value = 0x42;

    STRICT_EXPECTED_CALL(InterlockedAdd(&value, 0));
    STRICT_EXPECTED_CALL(WaitOnAddress(&value, IGNORED_ARG, sizeof(LONG), INFINITE))
        .CopyOutArgumentBuffer_Address(&target_value, sizeof(LONG));
    STRICT_EXPECTED_CALL(InterlockedAdd(&value, 0));

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, INFINITE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_003: [ If the value at address is equal to value, InterlockedHL_WaitForValue shall return INTERLOCKED_HL_OK. ]*/
/* Tests_SRS_INTERLOCKED_HL_01_005: [ When waiting for the value at address to change, the milliseconds argument value shall be used as timeout. ]*/
/* Tests_SRS_INTERLOCKED_HL_01_007: [ When WaitOnAddress succeeds, the value at address shall be compared to the target value passed in value by using InterlockedAdd. ]*/
/* Tests_SRS_INTERLOCKED_HL_01_008: [ If the value at address does not match, InterlockedHL_WaitForValue shall issue another call to WaitOnAddress. ]*/
TEST_FUNCTION(when_the_value_after_a_succesfull_wait_on_address_does_not_equal_target_a_new_wait_for_address_shall_be_issued)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    LONG value = 0x40;
    LONG intermediate_value = 0x41;
    LONG final_value = 0x42;

    STRICT_EXPECTED_CALL(InterlockedAdd(&value, 0));
    STRICT_EXPECTED_CALL(WaitOnAddress(&value, IGNORED_ARG, sizeof(LONG), INFINITE))
        .CopyOutArgumentBuffer_Address(&intermediate_value, sizeof(LONG));
    STRICT_EXPECTED_CALL(InterlockedAdd(&value, 0));
    STRICT_EXPECTED_CALL(WaitOnAddress(&value, IGNORED_ARG, sizeof(LONG), INFINITE))
        .CopyOutArgumentBuffer_Address(&final_value, sizeof(LONG));
    STRICT_EXPECTED_CALL(InterlockedAdd(&value, 0));

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, INFINITE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

/* Tests_SRS_INTERLOCKED_HL_01_006: [ If WaitOnAddress fails, InterlockedHL_WaitForValue shall fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(when_the_WaitOnAddress_fails_InterlockedHL_WaitForValue_also_fails)
{
    // arrange
    INTERLOCKED_HL_RESULT result;
    LONG value = 0x40;
    LONG intermediate_value = 0x41;

    STRICT_EXPECTED_CALL(InterlockedAdd(&value, 0));
    STRICT_EXPECTED_CALL(WaitOnAddress(&value, IGNORED_ARG, sizeof(LONG), INFINITE))
        .CopyOutArgumentBuffer_Address(&intermediate_value, sizeof(LONG))
        .SetReturn(FALSE);

    // act
    result = InterlockedHL_WaitForValue(&value, 0x42, INFINITE);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/*Tests_SRS_INTERLOCKED_HL_02_008: [ If target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedCompareExchange64If_with_target_NULL_fails)
{
    ///arrange
    LONG64 original_target;

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(NULL, 1, TEST_IS_GREATER, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_INTERLOCKED_HL_02_009: [ If compare is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedCompareExchange64If_with_compare_NULL_fails)
{
    ///arrange
    LONG64 original_target;
    volatile LONG64 target;
    (void)InterlockedExchange64(&target, 34);
    umock_c_reset_all_calls();

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 1, NULL, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_INTERLOCKED_HL_02_010: [ If original_target is NULL then InterlockedHL_CompareExchange64If shall return fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedCompareExchange64If_with_original_target_NULL_fails)
{
    ///arrange
    volatile LONG64 target;
    (void)InterlockedExchange64(&target, 34);
    umock_c_reset_all_calls();

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 1, TEST_IS_GREATER, NULL);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_012: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchange64If shall exchange target with exchange. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_014: [ If target did not change meanwhile then InterlockedHL_CompareExchange64If shall return return INTERLOCKED_HL_OK and shall peform the exchange of values. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ]*/
TEST_FUNCTION(InterlockedCompareExchange64If_with_compare_true_changed_false_succeeds)
{
    ///arrange
    LONG64 original_target;
    volatile LONG64 target;
    (void)InterlockedExchange64(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(InterlockedAdd64(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER(34, 99));
    STRICT_EXPECTED_CALL(InterlockedCompareExchange64(&target, 99, 34));

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 99, TEST_IS_GREATER, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    ASSERT_ARE_EQUAL(int64_t, 34, original_target);
    ASSERT_ARE_EQUAL(int64_t, 99, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_015: [ If compare returns false then InterlockedHL_CompareExchange64If shall not perform any exchanges and return INTERLOCKED_HL_OK. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_013: [ If target changed meanwhile then InterlockedHL_CompareExchange64If shall return return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ]*/
TEST_FUNCTION(InterlockedCompareExchange64If_with_compare_true_changed_true_succeeds)
{
    ///arrange
    LONG64 original_target;
    volatile LONG64 target;
    (void)InterlockedExchange64(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(InterlockedAdd64(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER(34, 99));
    STRICT_EXPECTED_CALL(InterlockedCompareExchange64(&target, 99, 34))
        .SetReturn(35);

    ///act
    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 99, TEST_IS_GREATER, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_CHANGED, result);
    ASSERT_ARE_EQUAL(int64_t, 34, original_target);
    ASSERT_ARE_EQUAL(int64_t, 34, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_INTERLOCKED_HL_02_011: [ InterlockedHL_CompareExchange64If shall acquire the initial value of target. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_012: [ If compare(target, exchange) returns true then InterlockedHL_CompareExchange64If shall exchange target with exchange. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_013: [ If target changed meanwhile then InterlockedHL_CompareExchange64If shall return return INTERLOCKED_HL_CHANGED and shall not peform any exchange of values. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_016: [ original_target shall be set to the original value of target. ]*/
TEST_FUNCTION(InterlockedCompareExchange64If_with_compare_false_succeeds)
{
    ///arrange
    LONG64 original_target;
    volatile LONG64 target;
    (void)InterlockedExchange64(&target, 34);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(InterlockedAdd64(&target, 0));
    STRICT_EXPECTED_CALL(TEST_IS_GREATER(34, 33)); /*33 is smaller, no exchanges happen*/

    ///act

    INTERLOCKED_HL_RESULT result = InterlockedHL_CompareExchange64If(&target, 33, TEST_IS_GREATER, &original_target);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
    ASSERT_ARE_EQUAL(int64_t, 34, original_target);
    ASSERT_ARE_EQUAL(int64_t, 34, target);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_INTERLOCKED_HL_02_020: [ If address is NULL then InterlockedHL_SetAndWake shall fail and return INTERLOCKED_HL_ERROR. ]*/
TEST_FUNCTION(InterlockedHL_SetAndWake_with_address_NULL_fails)
{
    ///arrange
    INTERLOCKED_HL_RESULT result;

    ///act
    result = InterlockedHL_SetAndWake(NULL, 3);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_ERROR, result);
}

/*Tests_SRS_INTERLOCKED_HL_02_017: [ InterlockedHL_SetAndWake shall set address to value. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_018: [ InterlockedHL_SetAndWake shall call WakeByAddressSingle. ]*/
/*Tests_SRS_INTERLOCKED_HL_02_019: [ InterlockedHL_SetAndWake shall succeed and return INTERLOCKED_HL_OK. ]*/
TEST_FUNCTION(InterlockedHL_SetAndWake_succeeds)
{
    ///arrange
    LONG hereLiesAThree = 3;
    LONG thatIsGoingToTurn4 = 4;
    INTERLOCKED_HL_RESULT result;

    STRICT_EXPECTED_CALL(InterlockedExchange(&hereLiesAThree, thatIsGoingToTurn4));
    STRICT_EXPECTED_CALL(WakeByAddressSingle(&hereLiesAThree));

    ///act
    result = InterlockedHL_SetAndWake(&hereLiesAThree, thatIsGoingToTurn4);

    ///assert
    ASSERT_ARE_EQUAL(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_OK, result);
}

END_TEST_SUITE(interlocked_hl_win32_ut)
