// Copyright (c) Microsoft. All rights reserved.



#include "async_type_helper_ut_pch.h"

// {00000000-0000-0000-0000-000000000000}

static const UUID_T NIL_GUID = { 0 };

static UUID_T test_uuid = {
    0x64, 0x6F, 0x6E, 0x27, 0x74, 0x20, 0x70, 0x61,
    0x6E, 0x69, 0x63, 0x30, 0x30, 0x30, 0x30, 0x30
};

static const unsigned char test_payload_memory[TEST_PAYLOAD_SIZE] = { 0x42, 0x43 };


static CONSTBUFFER_HANDLE test_payload;
static CONSTBUFFER_ARRAY_HANDLE* test_payload_array;
static uint32_t test_payload_count = 2;

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
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc_2, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc_flex, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(charptr_t, char*);
    REGISTER_UMOCK_ALIAS_TYPE(const_charptr_t, const char*);
    REGISTER_UMOCK_ALIAS_TYPE(const const_charptr_t, const char*);

    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_ARRAY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_ARRAY_HANDLE*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(constbuffer_array_ptr, CONSTBUFFER_ARRAY_HANDLE*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    test_payload = real_CONSTBUFFER_Create(test_payload_memory, sizeof(test_payload_memory));
    ASSERT_IS_NOT_NULL(test_payload);
    test_payload_array = (CONSTBUFFER_ARRAY_HANDLE*)real_gballoc_hl_malloc(sizeof(CONSTBUFFER_ARRAY_HANDLE) * test_payload_count);
    ASSERT_IS_NOT_NULL(test_payload_array);
    for (uint32_t i = 0; i < test_payload_count; i++)
    {
        test_payload_array[i] = real_constbuffer_array_create(&test_payload, 1);
        ASSERT_IS_NOT_NULL(test_payload_array[i]);
    }
    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    for (uint32_t i = 0; i < test_payload_count; i++)
    {
        real_constbuffer_array_dec_ref(test_payload_array[i]);
    }
    real_gballoc_hl_free(test_payload_array);
    real_CONSTBUFFER_DecRef(test_payload);
    umock_c_negative_tests_deinit();
}

// The following functions are compile-time tests for macro behavior and are intentionally not called
MU_SUPPRESS_WARNING(4505) // unreferenced function with internal linkage has been removed

/*Tests_SRS_ASYNC_TYPE_HELPER_42_015: [ If ASYNC_TYPE_HELPER_HAS_CONST_{type} is defined as 1 then ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(type) shall expand to ASYNC_TYPE_HELPER_NON_CONST_TYPE_{type}. ]*/
#define ASYNC_TYPE_HELPER_HAS_CONST_my_test_type 1
#define ASYNC_TYPE_HELPER_NON_CONST_TYPE_my_test_type char*

// This would be a warning if the macro didn't convert it
typedef const char* my_test_type;
static void test_strip_const(ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(my_test_type) non_const_char)
{
    non_const_char[0] = 'a';
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_017: [ Otherwise, ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(type) shall expand to type. ]*/

// This works without any extra defines
typedef const char* my_other_type;
static void test_with_const(ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(my_other_type) const_char)
{
    (void)const_char;
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_016: [ If ASYNC_TYPE_HELPER_USE_CONST_TYPE_{type} is defined as 1 then ASYNC_TYPE_HELPER_ADD_CONST_TYPE(type) shall expand to ASYNC_TYPE_HELPER_CONST_TYPE_{type}. ]*/
#define ASYNC_TYPE_HELPER_USE_CONST_TYPE_my_test_type_to_add 1
#define ASYNC_TYPE_HELPER_CONST_TYPE_my_test_type_to_add const char*

// This would be a warning if the macro didn't convert it
typedef char* my_test_type_to_add;
static void test_add_const(ASYNC_TYPE_HELPER_ADD_CONST_TYPE(my_test_type_to_add) const_char)
{
    (void)const_char;
    // The following issues a warning because macro adds const
    //const_char[0] = 'a';
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_018: [ Otherwise, ASYNC_TYPE_HELPER_ADD_CONST_TYPE(type) shall expand to type. ]*/
typedef char* my_test_type_dont_add;
static void test_no_add_const(ASYNC_TYPE_HELPER_ADD_CONST_TYPE(my_test_type_dont_add) const_char)
{
    // Macro does not add const
    const_char[0] = 'a';
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_019: [ If ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_{type} is defined as 1 then type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(type) shall expand to type. ]*/
#define ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_my_type_is_a_pointer 1
typedef char* my_type_is_a_pointer;
static void test_do_not_add_pointer(my_type_is_a_pointer ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(my_type_is_a_pointer) single_pointer)
{
    single_pointer[0] = 'a';
    // THe following issues a warning
    //single_pointer[0][0] = 'a';
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_020: [ Otherwise, type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(type) shall expand to type *. ]*/
typedef char my_type_gets_a_pointer;
static void test_add_pointer(my_type_gets_a_pointer ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(my_type_gets_a_pointer) becomes_a_pointer)
{
    becomes_a_pointer[0] = 'a';
}

MU_UNSUPPRESS_WARNING(4505)

//
// ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)
//

/*Tests_SRS_ASYNC_TYPE_HELPER_42_006: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
TEST_FUNCTION(const_charptr_t_copy_with_NULL_dest_fails)
{
    /// arrange

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(NULL, "foo");

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_007: [ If source is NULL then the copy handler will set the dest to NULL and return 0. ]*/
TEST_FUNCTION(const_charptr_t_copy_with_NULL_source_sets_dest_to_NULL)
{
    /// arrange
    char* dest;

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(&dest, NULL);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(dest);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_008: [ The copy handler shall allocate a string large enough to hold source, including the terminating NULL. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_42_009: [ The copy handler shall copy the string from source to dest. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_42_011: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(const_charptr_t_copy_succeeds)
{
    /// arrange
    char* dest;

    STRICT_EXPECTED_CALL(malloc(4));

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(&dest, "foo");

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, "foo", dest);

    /// cleanup
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)(dest);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_010: [ If there are any failures then the copy handler shall fail and return a non-zero value. ]*/
TEST_FUNCTION(const_charptr_t_copy_fails_when_malloc_fails)
{
    /// arrange
    char* dest;

    STRICT_EXPECTED_CALL(malloc(4))
        .SetReturn(NULL);

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(&dest, "foo");

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

//
// ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)
//

/*Tests_SRS_ASYNC_TYPE_HELPER_42_012: [ If value is NULL then the free handler shall return. ]*/
TEST_FUNCTION(const_charptr_t_free_with_NULL_value_fails)
{
    /// arrange

    /// act
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)(NULL);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_TYPE_HELPER_42_013: [ The free handler shall free value. ]*/
TEST_FUNCTION(const_charptr_t_free_works)
{
    /// arrange
    char* dest;
    STRICT_EXPECTED_CALL(malloc(4));
    ASSERT_ARE_EQUAL(int, 0, ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)(&dest, "foo"));

    STRICT_EXPECTED_CALL(free(dest));

    /// act
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)(dest);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)
//

/*Tests_SRS_ASYNC_TYPE_HELPER_04_001: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
TEST_FUNCTION(UUID_T_copy_with_NULL_dest_fails)
{
    /// arrange

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)(NULL, test_uuid);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_04_002: [ If source is NULL then the copy handler will set the dest to a zero UUID value. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_04_004: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(UUID_T_copy_with_NULL_src_produces_NIL_GUID)
{
    /// arrange
    UUID_T dest;

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)(&dest, NULL);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(0 == memcmp(NIL_GUID, dest, sizeof(UUID_T)));

    /// cleanup
    ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T)(dest);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_04_003: [ Otherwise, the copy handler shall copy the UUID_T from source to dest. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_04_004: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(UUID_T_copy_works)
{
    /// arrange
    UUID_T dest;

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)(&dest, test_uuid);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(0 == memcmp(test_uuid, dest, sizeof(UUID_T)));

    /// cleanup
    ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T)(dest);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_04_003: [ Otherwise, the copy handler shall copy the UUID_T from source to dest. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_04_004: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(UUID_T_copy_works_2)
{
    /// arrange
    UUID_T src1, src2;
    UUID_T dest1, dest2;
    int result;

    result = uuid_produce(src1);
    ASSERT_ARE_EQUAL(int, 0, result);

    result = uuid_produce(src2);
    ASSERT_ARE_EQUAL(int, 0, result);

    umock_c_reset_all_calls();

    /// act
    result = ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)(&dest1, src1);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)(&dest2, src2);
    ASSERT_ARE_EQUAL(int, 0, result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(0 == memcmp(src1, dest1, sizeof(UUID_T)));
    ASSERT_IS_TRUE(0 == memcmp(src2, dest2, sizeof(UUID_T)));

    /// cleanup
    ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T)(dest1);
    ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T)(dest2);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_04_005: [This handler shall do nothing.]*/
TEST_FUNCTION(UUID_T_free_does_nothing)
{
    /// arrange
    UUID_T dest;
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)(&dest, test_uuid);
    ASSERT_ARE_EQUAL(int, 0, result);

    /// act
    ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T)(dest);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)
//

/*Tests_SRS_ASYNC_TYPE_HELPER_01_001: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
TEST_FUNCTION(const_UUID_T_copy_with_NULL_dest_fails)
{
    /// arrange

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)(NULL, test_uuid);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_01_002: [ If source is NULL then the copy handler will set the dest to a zero UUID value. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_01_004: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(const_UUID_T_copy_with_NULL_src_produces_NIL_GUID)
{
    /// arrange
    UUID_T dest;

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)(&dest, NULL);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(0 == memcmp(NIL_GUID, dest, sizeof(const_UUID_T)));

    /// cleanup
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T)(dest);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_01_003: [ Otherwise, the copy handler shall copy the const UUID_T from source to dest. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_01_004: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(const_UUID_T_copy_works)
{
    /// arrange
    UUID_T dest;

    /// act
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)(&dest, test_uuid);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_TRUE(0 == memcmp(test_uuid, dest, sizeof(const_UUID_T)));

    /// cleanup
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T)(dest);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_01_003: [ Otherwise, the copy handler shall copy the const UUID_T from source to dest. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_01_004: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(const_UUID_T_copy_works_2)
{
    /// arrange
    UUID_T src1, src2;
    UUID_T dest1, dest2;
    int result;

    result = uuid_produce(src1);
    ASSERT_ARE_EQUAL(int, 0, result);

    result = uuid_produce(src2);
    ASSERT_ARE_EQUAL(int, 0, result);

    umock_c_reset_all_calls();

    /// act
    result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)(&dest1, src1);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)(&dest2, src2);
    ASSERT_ARE_EQUAL(int, 0, result);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_TRUE(0 == memcmp(src1, dest1, sizeof(const_UUID_T)));
    ASSERT_IS_TRUE(0 == memcmp(src2, dest2, sizeof(const_UUID_T)));

    /// cleanup
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T)(dest1);
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T)(dest2);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_01_005: [ This handler shall do nothing. ]*/
TEST_FUNCTION(const_UUID_T_free_does_nothing)
{
    /// arrange
    UUID_T dest;
    int result = ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)(&dest, test_uuid);
    ASSERT_ARE_EQUAL(int, 0, result);

    /// act
    ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T)(dest);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// constbuffer_array_ptr_copy
//

/*Tests_SRS_ASYNC_TYPE_HELPER_28_001: [ If dest is NULL then the copy handler will fail and return a non-zero value. ]*/
TEST_FUNCTION(constbuffer_array_ptr_copy_with_NULL_dest_fails)
{
    /// arrange

    /// act
    int result = constbuffer_array_ptr_copy(NULL, test_payload_array, test_payload_count);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_28_011: [ If item_count is 0, the copy handler will fail and return a non-zero value. ]*/
TEST_FUNCTION(constbuffer_array_ptr_copy_with_0_item_count_fails)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE* dest;

    /// act
    int result = constbuffer_array_ptr_copy(&dest, test_payload_array, 0);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_28_002: [ If src is NULL then the copy handler will set the dest to NULL and return 0. ]*/
TEST_FUNCTION(constbuffer_array_ptr_copy_with_NULL_source_sets_dest_to_NULL)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE* dest;

    /// act
    int result = constbuffer_array_ptr_copy(&dest, NULL, test_payload_count);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(dest);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_28_003: [ The copy handler shall allocate an array to store all the constbuffer_array in the src. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_28_005: [ The copy handler shall call constbuffer_array_inc_ref on each constbuffer array in src. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_28_006: [ The copy handler shall copy all the constbuffer_arrays from the src to the dest. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_28_007: [ The copy handler shall succeed and return 0. ]*/
TEST_FUNCTION(constbuffer_array_ptr_copy_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE* dest;

    STRICT_EXPECTED_CALL(malloc_2(test_payload_count, IGNORED_ARG));
    for (uint32_t i = 0; i < test_payload_count; i++)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_inc_ref(IGNORED_ARG));
    }

    /// act
    int result = constbuffer_array_ptr_copy(&dest, test_payload_array, test_payload_count);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(dest);
    for (uint32_t i = 0; i < test_payload_count; i++)
    {
        ASSERT_IS_TRUE(dest[i] == test_payload_array[i]);
    }

    /// cleanup
    constbuffer_array_ptr_free(dest, test_payload_count);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_28_004: [ If there are any failures then the copy handler shall fail and return a non-zero value. ]*/
TEST_FUNCTION(constbuffer_array_ptr_copy_fails_when_malloc_fails)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE* dest;

    STRICT_EXPECTED_CALL(malloc_2(test_payload_count, IGNORED_ARG))
        .SetReturn(NULL);

    /// act
    int result = constbuffer_array_ptr_copy(&dest, test_payload_array, test_payload_count);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

//
// constbuffer_array_ptr_free
//

/*Tests_SRS_ASYNC_TYPE_HELPER_28_008: [ If value is NULL then the free handler shall return. ]*/
TEST_FUNCTION(constbuffer_array_ptr_free_with_NULL_value_fails)
{
    /// arrange

    /// act
    constbuffer_array_ptr_free(NULL, test_payload_count);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_ASYNC_TYPE_HELPER_28_012: [ If item_count is 0, the free handler shall return. ]*/
TEST_FUNCTION(constbuffer_array_ptr_free_with_0_item_count_fails)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE* dest;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_ptr_copy(&dest, test_payload_array, test_payload_count));
    umock_c_reset_all_calls();

    /// act
    constbuffer_array_ptr_free(dest, 0);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /// cleanup
    constbuffer_array_ptr_free(dest, test_payload_count);
}

/*Tests_SRS_ASYNC_TYPE_HELPER_28_009: [ The free handler shall call constbuffer_array_dec_ref on each constbuffer array in value. ]*/
/*Tests_SRS_ASYNC_TYPE_HELPER_28_010: [ The free handler shall free value. ]*/
TEST_FUNCTION(constbuffer_array_ptr_free_works)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE* dest;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_ptr_copy(&dest, test_payload_array, test_payload_count));
    umock_c_reset_all_calls();

    for (uint32_t i = 0; i < test_payload_count; i++)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_dec_ref(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(dest));

    /// act
    constbuffer_array_ptr_free(dest, test_payload_count);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)