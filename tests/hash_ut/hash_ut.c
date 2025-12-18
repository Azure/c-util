// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "hash_ut_pch.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#include "MurmurHash2.h"

MOCK_FUNCTION_WITH_CODE(, uint32_t, MurmurHash2, const void*, key, int, len, uint32_t, seed)
MOCK_FUNCTION_END(0x42)

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/* hash_compute_hash */

/* Tests_SRS_HASH_01_001: [ hash_compute_hash shall call MurmurHash2, while passing as arguments buffer, length and 0 as seed. ]*/
/* Tests_SRS_HASH_01_003: [ On success hash_compute_hash shall return 0 and fill in hash the computed hash value. ]*/
TEST_FUNCTION(hash_compute_hash_calls_the_underlying_Murmur_hash)
{
    // arrange
    uint32_t hash;
    uint8_t a = 0;
    int result;

    STRICT_EXPECTED_CALL(MurmurHash2(&a, sizeof(a), 0));

    // act
    result = hash_compute_hash(&a, sizeof(a), &hash);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 0x42, hash);
}

/* Tests_SRS_HASH_01_004: [ If buffer is NULL, hash_compute_hash shall fail and return a non-zero value. ]*/
TEST_FUNCTION(hash_compute_hash_with_NULL_key_fails)
{
    // arrange
    uint32_t hash;
    uint8_t a;
    int result;

    // act
    result = hash_compute_hash(NULL, sizeof(a), &hash);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_HASH_01_005: [ If length is 0, hash_compute_hash shall fail and return a non-zero value. ]*/
TEST_FUNCTION(hash_compute_hash_with_0_length_fails)
{
    // arrange
    uint32_t hash;
    uint8_t a = 0;
    int result;

    // act
    result = hash_compute_hash(&a, 0, &hash);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_HASH_01_006: [ If hash is NULL, hash_compute_hash shall fail and return a non-zero value. ]*/
TEST_FUNCTION(hash_compute_hash_with_NULL_hash_pointer_fails)
{
    // arrange
    uint8_t a = 0;
    int result;

    // act
    result = hash_compute_hash(&a, sizeof(a), NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_HASH_01_002: [ If length is greater than or equal to INT_MAX, hash_compute_hash shall fail and return a non-zero value. ]*/
TEST_FUNCTION(hash_compute_hash_with_INT_MAX_fails)
{
    // arrange
    uint8_t a = 0;
    int result;

    // act
    result = hash_compute_hash(&a, (size_t)INT_MAX, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

#if SIZE_MAX > INT_MAX
/* Tests_SRS_HASH_01_002: [ If length is greater than or equal to INT_MAX, hash_compute_hash shall fail and return a non-zero value. ]*/
TEST_FUNCTION(hash_compute_hash_with_INT_MAX_plus_1_fails)
{
    // arrange
    uint8_t a = 0;
    int result;

    // act
    result = hash_compute_hash(&a, (size_t)INT_MAX + 1, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}
#endif

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
