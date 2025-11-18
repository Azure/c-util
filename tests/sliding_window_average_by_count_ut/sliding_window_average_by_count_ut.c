// Copyright (c) Microsoft. All rights reserved.


#include "sliding_window_average_by_count_ut_pch.h"

static THANDLE(SLIDING_WINDOW_AVERAGE) create_new_sliding_window_average(int32_t count)
{
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_create(false, "sliding_window_average"));
    THANDLE(SLIDING_WINDOW_AVERAGE) result = sliding_window_average_by_count_create(count);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    return result;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");

    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(SLIDING_WINDOW_AVERAGE), void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

//
// sliding_window_average_by_count_create
//

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_001: [ sliding_window_average_by_count_create shall return NULL is window_count is not >= 1. ]
TEST_FUNCTION(sliding_window_average_by_count_create_returns_null_on_negative_window)
{
    // arrange

    // act
    THANDLE(SLIDING_WINDOW_AVERAGE) result = sliding_window_average_by_count_create(-1);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_001: [ sliding_window_average_by_count_create shall return NULL is window_count is not >= 1. ]
TEST_FUNCTION(sliding_window_average_by_count_create_returns_null_on_window_too_small)
{
    // arrange

    // act
    THANDLE(SLIDING_WINDOW_AVERAGE) result = sliding_window_average_by_count_create(0);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_003: [ sliding_window_average_by_count_create shall return NULL if there are any errors. ]
TEST_FUNCTION(sliding_window_average_by_count_create_returns_null_on_thandle_malloc_fail)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG)).SetReturn(NULL);

    // act
    THANDLE(SLIDING_WINDOW_AVERAGE) result = sliding_window_average_by_count_create(2);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_026: [ sliding_window_average_by_count_create shall call srw_lock_create to create a lock for the SLIDING_WINDOW_AVERAGE struct. ]
TEST_FUNCTION(sliding_window_average_by_count_create_returns_null_on_srw_lock_create_fail)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_create(false, "sliding_window_average")).SetReturn(NULL);

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE(SLIDING_WINDOW_AVERAGE) result = sliding_window_average_by_count_create(2);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_002: [ sliding_window_average_by_count_create shall call THANDLE_MALLOC_FLEX to allocate SLIDING_WINDOW_AVERAGE struct and an array of int64_t the size of window_count. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_004: [ sliding_window_average_by_count_create shall initialize all counts in window to zero. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_005: [ sliding_window_average_by_count_create shall initialize the current sum, the current count, and the current average to zero. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_006: [ sliding_window_average_by_count_create shall initialize the next_available_slot to 0. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_025: [ sliding_window_average_by_count_create shall return a non-null THANDLE(SLIDING_WINDOW_AVERAGE) on success. ]
TEST_FUNCTION(sliding_window_average_by_count_create_returns_non_null_on_success)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_create(false, "sliding_window_average"));

    // act
    THANDLE(SLIDING_WINDOW_AVERAGE) result = sliding_window_average_by_count_create(2);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&result, NULL);
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_031: [sliding_window_average_by_count_dispose shall return if content is NULL.]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_033 : [sliding_window_average_by_count_dispose shall call srw_lock_destroy on the lock.]
TEST_FUNCTION(sliding_window_average_by_count_calls_lock_destroy_on_refcount_0)
{
    // arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_create(false, "sliding_window_average"));
    THANDLE(SLIDING_WINDOW_AVERAGE) result = sliding_window_average_by_count_create(2);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act

    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&result, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

//
// sliding_window_average_by_count_add
//

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_008: [ sliding_window_average_by_count_add shall return a non-zero value if handle is NULL. ]
TEST_FUNCTION(sliding_window_average_by_count_add_fails_when_handle_is_null)
{
    // arrange

    // act
    int result = sliding_window_average_by_count_add(NULL, 17);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_019: [ sliding_window_average_by_count_add shall return zero on success. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_027: [ sliding_window_average_by_count_add shall call srw_lock_acquire_exclusive on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_028: [ sliding_window_average_by_count_add shall call srw_lock_release_exclusive on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
TEST_FUNCTION(sliding_window_average_by_count_add_succeeds)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(2);

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    // act
    int result = sliding_window_average_by_count_add(object_under_test, 17);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

//
// sliding_window_average_by_count_get
//

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_021: [ sliding_window_average_by_count_get shall return a non-zero value if handle is NULL. ]
TEST_FUNCTION(sliding_window_average_by_count_get_fails_if_handle_is_null)
{
    // arrange

    // act
    double avg;
    int result = sliding_window_average_by_count_get(NULL, &avg);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_022: [ sliding_window_average_by_count_get shall return a non-zero value if average is NULL. ]
TEST_FUNCTION(sliding_window_average_by_count_get_fails_if_average_is_null)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(2);

    // act
    int result = sliding_window_average_by_count_get(object_under_test, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_005: [ sliding_window_average_by_count_create shall initialize the current sum, the current count, and the current average to zero. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_029: [ sliding_window_average_by_count_get shall call srw_lock_acquire_shared on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_023: [ sliding_window_average_by_count_get shall copy the current average into average. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_030: [ sliding_window_average_by_count_get shall call srw_lock_release_shared on the lock in the SLIDING_WINDOW_AVERAGE struct. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_024: [ sliding_window_average_by_count_get shall return zero on success. ]
TEST_FUNCTION(sliding_window_average_by_count_get_starts_with_0_average)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(2);

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));

    // act
    double avg;
    int result = sliding_window_average_by_count_get(object_under_test, &avg);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(double, 0.0, avg);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_009: [ sliding_window_average_by_count_add shall increment the next_available_slot. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_010: [ If next_available_slot is >= window_count ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_011: [ sliding_window_average_by_count_add shall subtract the value at next_available_slot%window_count from the current sum. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_012: [ sliding_window_average_by_count_add shall add the next_count value to the current sum. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_013: [ sliding_window_average_by_count_add shall assign the next_count value at next_available_slot%window_count index. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_014: [ If the next_available_slot < window_count ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_015: [ sliding_window_average_by_count_add shall add the next_count value to the current sum ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_016: [ sliding_window_average_by_count_add shall assign the next_slot value at next_available_slot index ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_017: [ sliding_window_average_by_count_add shall increment the current count. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_018: [ sliding_window_average_by_count_add shall assign the current average to the current sum/current count. ]
TEST_FUNCTION(sliding_window_average_by_count_get_average_windowsize_1_is_correct_after_increment)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(1);

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    int result = sliding_window_average_by_count_add(object_under_test, 3);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));

    // act
    double avg;
    result = sliding_window_average_by_count_get(object_under_test, &avg);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(double, 3.0, avg);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_009: [ sliding_window_average_by_count_add shall increment the next_available_slot. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_010: [ If next_available_slot is >= window_count ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_011: [ sliding_window_average_by_count_add shall subtract the value at next_available_slot%window_count from the current sum. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_012: [ sliding_window_average_by_count_add shall add the next_count value to the current sum. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_013: [ sliding_window_average_by_count_add shall assign the next_count value at next_available_slot%window_count index. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_014: [ If the next_available_slot < window_count ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_015: [ sliding_window_average_by_count_add shall add the next_count value to the current sum ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_016: [ sliding_window_average_by_count_add shall assign the next_slot value at next_available_slot index ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_017: [ sliding_window_average_by_count_add shall increment the current count. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_018: [ sliding_window_average_by_count_add shall assign the current average to the current sum/current count. ]
TEST_FUNCTION(sliding_window_average_by_count_get_average_windowsize_3_is_correct_after_increment)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(3);

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    int result = sliding_window_average_by_count_add(object_under_test, 3);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
    // act
    double avg;
    result = sliding_window_average_by_count_get(object_under_test, &avg);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(double, 3.0, avg);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

//
// Equivalent to the SlidingWindowAverageByCountTests in EventGrid.
//

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_009: [ sliding_window_average_by_count_add shall increment the next_available_slot. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_010: [ If next_available_slot is >= window_count ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_011: [ sliding_window_average_by_count_add shall subtract the value at next_available_slot%window_count from the current sum. ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_012: [ sliding_window_average_by_count_add shall add the next_count value to the current sum. ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_013: [ sliding_window_average_by_count_add shall assign the next_count value at next_available_slot%window_count index. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_018: [ sliding_window_average_by_count_add shall assign the current average to the current sum/current count. ]
TEST_FUNCTION(sliding_window_average_by_count_get_average_windowsize_1_is_correct_after_multiple_increments)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(1);
    double avg;

    int64_t increments[4] = { 1, 5, 3, 2 };
    double averages[4] = { 1.0, 5.0, 3.0, 2.0 };

    for (int i = 0; i < 4; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
        STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

        int result = sliding_window_average_by_count_add(object_under_test, increments[i]);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
        STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));

        // act

        result = sliding_window_average_by_count_get(object_under_test, &avg);

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(double, averages[i], avg);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_009: [ sliding_window_average_by_count_add shall increment the next_available_slot. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_010: [ If next_available_slot is >= window_count ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_011: [ sliding_window_average_by_count_add shall subtract the value at next_available_slot%window_count from the current sum. ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_012: [ sliding_window_average_by_count_add shall add the next_count value to the current sum. ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_013: [ sliding_window_average_by_count_add shall assign the next_count value at next_available_slot%window_count index. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_014: [ If the next_available_slot < window_count ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_015: [ sliding_window_average_by_count_add shall add the next_count value to the current sum ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_016: [ sliding_window_average_by_count_add shall assign the next_slot value at next_available_slot index ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_017: [ sliding_window_average_by_count_add shall increment the current count. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_018: [ sliding_window_average_by_count_add shall assign the current average to the current sum/current count. ]
TEST_FUNCTION(sliding_window_average_by_count_get_average_windowsize_2_is_correct_after_multiple_increments)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(2);

    double avg;

    int64_t increments[5] = { 5, 1, 3, 2, 0 };
    double averages[5] = { 5.0, 3.0, 2.0, 2.5, 1.0 };

    for (int i = 0; i < 5; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
        STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

        int result = sliding_window_average_by_count_add(object_under_test, increments[i]);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
        STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));

        // act

        result = sliding_window_average_by_count_get(object_under_test, &avg);

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(double, averages[i], avg);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_009: [ sliding_window_average_by_count_add shall increment the next_available_slot. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_010: [ If next_available_slot is >= window_count ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_011: [ sliding_window_average_by_count_add shall subtract the value at next_available_slot%window_count from the current sum. ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_012: [ sliding_window_average_by_count_add shall add the next_count value to the current sum. ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_013: [ sliding_window_average_by_count_add shall assign the next_count value at next_available_slot%window_count index. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_014: [ If the next_available_slot < window_count ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_015: [ sliding_window_average_by_count_add shall add the next_count value to the current sum ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_016: [ sliding_window_average_by_count_add shall assign the next_slot value at next_available_slot index ]
    // Tests_SRS_SLIDING_AVERAGE_WINDOW_45_017: [ sliding_window_average_by_count_add shall increment the current count. ]
// Tests_SRS_SLIDING_AVERAGE_WINDOW_45_018: [ sliding_window_average_by_count_add shall assign the current average to the current sum/current count. ]
TEST_FUNCTION(sliding_window_average_by_count_get_average_windowsize_3_is_correct_after_multiple_increments)
{
    // arrange
    THANDLE(SLIDING_WINDOW_AVERAGE) object_under_test = create_new_sliding_window_average(3);

    double avg;

    int64_t increments[6] = { 1, 5, 3, 4, 11, 0 };
    double averages[6] = { 1.0, 3.0, 3.0, 4.0, 6.0, 5.0 };

    for (int i = 0; i < 5; i++)
    {
        STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
        STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

        int result = sliding_window_average_by_count_add(object_under_test, increments[i]);
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
        STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));

        // act

        result = sliding_window_average_by_count_get(object_under_test, &avg);

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
        ASSERT_ARE_EQUAL(double, averages[i], avg);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    // ablution
    THANDLE_ASSIGN(SLIDING_WINDOW_AVERAGE)(&object_under_test, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
