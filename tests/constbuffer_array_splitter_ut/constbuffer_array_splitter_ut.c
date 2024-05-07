// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <inttypes.h>


#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"
#include "c_util/constbuffer_array_tarray.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"


#include "../reals/real_constbuffer.h"
#include "../reals/real_constbuffer_array.h"
#include "../reals/real_constbuffer_array_tarray.h"

#include "c_util/constbuffer_array_splitter.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static CONSTBUFFER_HANDLE generate_test_buffer(uint32_t size, unsigned char data)
{
    unsigned char* memory = real_gballoc_hl_malloc(size);
    ASSERT_IS_NOT_NULL(memory);

    (void)memset(memory, data, size);

    CONSTBUFFER_HANDLE result = real_CONSTBUFFER_CreateWithMoveMemory(memory, size);
    ASSERT_IS_NOT_NULL(result);

    return result;
}

// Creates CONSTBUFFER_ARRAY where each buffer is of size buffer_size, buffer_size+size_increment, buffer_size+(size_increment*2), ... buffer_size+(size_increment*N)
// size_increment may be negative but test fails if it results in a negative buffer_size
static CONSTBUFFER_ARRAY_HANDLE generate_test_buffer_array_increasing_size(uint32_t buffer_count, uint32_t buffer_size, int32_t size_increment)
{
    CONSTBUFFER_HANDLE* buffers = real_gballoc_hl_malloc(sizeof(CONSTBUFFER_HANDLE) * buffer_count);
    ASSERT_IS_NOT_NULL(buffers);

    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        ASSERT_IS_TRUE(((int32_t)buffer_size + size_increment * i) >= 0, "Bad parameters to test, negative or zero buffer size!");
        uint32_t this_buffer_size = (uint32_t)((int32_t)buffer_size + size_increment * i);

        if (this_buffer_size == 0)
        {
            buffers[i] = real_CONSTBUFFER_Create(NULL, 0);
            ASSERT_IS_NOT_NULL(buffers[i]);
        }
        else
        {
            buffers[i] = generate_test_buffer(this_buffer_size, ('a' + i) % ('z' - 'a'));
        }
    }

    CONSTBUFFER_ARRAY_HANDLE buffer_array = real_constbuffer_array_create(buffers, buffer_count);
    ASSERT_IS_NOT_NULL(buffer_array);

    // cleanup

    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        real_CONSTBUFFER_DecRef(buffers[i]);
    }
    real_gballoc_hl_free(buffers);

    return buffer_array;
}

static CONSTBUFFER_ARRAY_HANDLE generate_test_buffer_array(uint32_t buffer_count, uint32_t buffer_size)
{
    return generate_test_buffer_array_increasing_size(buffer_count, buffer_size, 0);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_2, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc_2, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc_flex, NULL);

    REGISTER_CONSTBUFFER_GLOBAL_MOCK_HOOK();
    REGISTER_CONSTBUFFER_ARRAY_GLOBAL_MOCK_HOOK();
    REGISTER_TARRAY_CONSTBUFFER_ARRAY_HANDLE_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(constbuffer_array_get_all_buffers_size, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(CONSTBUFFER_CreateWithMoveMemory, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(constbuffer_array_create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(constbuffer_array_create_empty, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(TARRAY_CREATE(CONSTBUFFER_ARRAY_HANDLE), NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(TARRAY_ENSURE_CAPACITY(CONSTBUFFER_ARRAY_HANDLE), MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(constbuffer_array_create_from_buffer_offset_and_count, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_ARRAY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(TARRAY(CONSTBUFFER_ARRAY_HANDLE), void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    int result = umock_c_negative_tests_init();
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_negative_tests_init failed");
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* constbuffer_array_splitter_split */

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_001: [ If buffers is NULL then constbuffer_array_splitter_split shall fail and return NULL. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_null_buffers_fails)
{
    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(NULL, 42);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_002: [ If max_buffer_size is 0 then constbuffer_array_splitter_split shall fail and return NULL. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_0_max_buffer_size_fails)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(2, 2);

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 0);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_019: [ If the buffer count is 0 then constbuffer_array_splitter_split shall call constbuffer_array_create_empty and return the result. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_empty_array_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_empty();

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, buffer_count);

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_020: [ If the remaining_buffer_size is 0 (all buffers are empty) then constbuffer_array_splitter_split shall call constbuffer_array_create_empty and return the result. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_all_empty_buffers_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(3, 0);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, buffer_count);

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_1_buffer_1_byte_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(1, 1);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(1, sizeof(CONSTBUFFER_HANDLE)));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0));

    STRICT_EXPECTED_CALL(malloc(1));
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1));

    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 1));

    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, buffer_count);

    const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result, 0);
    ASSERT_ARE_EQUAL(size_t, 1, buffer_temp->size);

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_multiple_buffers_1_byte_each_merge_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(100, 1);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(1, sizeof(CONSTBUFFER_HANDLE)));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0));

    STRICT_EXPECTED_CALL(malloc(100));
    for (uint32_t i = 1; i < 100; ++i)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, i));
    }
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 100));
    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 1));

    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, buffer_count);

    const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result, 0);
    ASSERT_ARE_EQUAL(size_t, 100, buffer_temp->size);

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_multiple_buffers_1000_bytes_each_merge_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(10, 1000);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(1, sizeof(CONSTBUFFER_HANDLE)));

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0));
    STRICT_EXPECTED_CALL(malloc(10 * 1000));
    for (uint32_t i = 1; i < 10; ++i)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, i));
    }
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 10 * 1000));
    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 1));

    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1024 * 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, buffer_count);

    const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result, 0);
    ASSERT_ARE_EQUAL(size_t, 10 * 1000, buffer_temp->size);

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_multiple_buffers_1000_bytes_each_merge_and_split_succeeds)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(10, 1000);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(7, sizeof(CONSTBUFFER_HANDLE)));

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 1));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 2));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 3));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 4));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 5));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 6));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 7));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 8));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 9));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1000));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1000));

    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 7));

    for (uint32_t i = 0; i < 7; ++i)
    {
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1500);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 7, buffer_count);

    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result, i);

        if (i != buffer_count - 1)
        {
            ASSERT_ARE_EQUAL(size_t, 1500, buffer_temp->size);
        }
        else
        {
            ASSERT_ARE_EQUAL(size_t, 1000, buffer_temp->size);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_large_buffers_to_100MB_chunks_succeeds)
{
    /// arrange
    // Buffer sizes 201MB, 202MB, 203MB
    // Split at 100MB
    // 6x buffers at 100MB and 1 small buffer
    // This simulates the actual size of splits for blob blocks and uses a split > UINT32_MAX
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array_increasing_size(3, 201 * 1024 * 1024, 1024 * 1024);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(7, sizeof(CONSTBUFFER_HANDLE)));

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0));

    STRICT_EXPECTED_CALL(malloc(100 * 1024 * 1024));
    // Read 100 MB, 101MB remain
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 100 * 1024 * 1024));

    STRICT_EXPECTED_CALL(malloc(100 * 1024 * 1024));
    // Read 100 MB, 1MB remain
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 100 * 1024 * 1024));

    STRICT_EXPECTED_CALL(malloc(100 * 1024 * 1024));
    // Read 1 MB
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 1));
    // Read 99 MB, 103MB remain
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 100 * 1024 * 1024));

    STRICT_EXPECTED_CALL(malloc(100 * 1024 * 1024));
    // Read 100 MB, 3MB remain
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 100 * 1024 * 1024));

    STRICT_EXPECTED_CALL(malloc(100 * 1024 * 1024));
    // Read 3 MB
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 2));
    // Read 97 MB, 106MB remain
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 100 * 1024 * 1024));

    STRICT_EXPECTED_CALL(malloc(100 * 1024 * 1024));
    // Read 100 MB, 6MB remain
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 100 * 1024 * 1024));

    STRICT_EXPECTED_CALL(malloc(6 * 1024 * 1024));
    // Read 6MB
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 6 * 1024 * 1024));

    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 7));

    for (uint32_t i = 0; i < 7; ++i)
    {
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 100 * 1024 * 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 7, buffer_count);

    for (uint32_t i = 0; i < buffer_count; ++i)
    {
        const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result, i);

        if (i != buffer_count - 1)
        {
            ASSERT_ARE_EQUAL(size_t, 100 * 1024 * 1024, buffer_temp->size);
        }
        else
        {
            ASSERT_ARE_EQUAL(size_t, 6 * 1024 * 1024, buffer_temp->size);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_empty_buffers_at_start_succeeds)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE empty = generate_test_buffer_array(3, 0);
    CONSTBUFFER_ARRAY_HANDLE buffers_temp = generate_test_buffer_array(10, 1000);

    CONSTBUFFER_ARRAY_HANDLE buffers_to_merge[2];
    buffers_to_merge[0] = empty;
    buffers_to_merge[1] = buffers_temp;
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_from_array_array(buffers_to_merge, sizeof(buffers_to_merge) / sizeof(buffers_to_merge[0]));

    real_constbuffer_array_dec_ref(buffers_temp);
    real_constbuffer_array_dec_ref(empty);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(7, sizeof(CONSTBUFFER_HANDLE)));

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 1));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 2));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 3));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 4));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 5));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 6));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 7));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 8));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 9));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 10));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 11));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 12));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1000));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1000));

    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 7));

    for (uint32_t i = 0; i < 7; ++i)
    {
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1500);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 7, buffer_count);

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_018: [ constbuffer_array_splitter_split shall call constbuffer_array_get_buffer_count. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_004: [ constbuffer_array_splitter_split shall call constbuffer_array_get_all_buffers_size for buffers and store the result as remaining_buffer_size. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_005: [ constbuffer_array_splitter_split shall allocate an array of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_006: [ constbuffer_array_splitter_split shall initialize the current buffer index to 0 and the current buffer offset to 0. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_007: [ constbuffer_array_splitter_split shall get the first buffer in buffers that is not empty. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_008: [ While the remaining_buffer_size is greater than 0: ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_009: [ constbuffer_array_splitter_split shall allocate memory of size min(max_buffer_size, remaining_buffer_size). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_010: [ constbuffer_array_splitter_split shall copy data from the buffers starting at the current buffer index and buffer offset until it has filled the allocated memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_011: [ constbuffer_array_splitter_split shall update the current buffer offset as it copies the memory. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_012: [ If the end of the current buffer in buffers is reached then constbuffer_array_splitter_split shall increment the buffer index and reset the buffer offset to 0, to read the next buffer in the original array (skipping over empty buffers). ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_013: [ constbuffer_array_splitter_split shall decrement the remaining_buffer_size by the amount of data copied. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_014: [ constbuffer_array_splitter_split shall call CONSTBUFFER_CreateWithMoveMemory for the allocated memory and store it in the allocated array. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_015: [ constbuffer_array_splitter_split shall call constbuffer_array_create with the allocated array of buffer handles as split_buffers. ]*/
/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_016: [ constbuffer_array_splitter_split shall succeed and return the split_buffers. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_empty_buffers_in_middle_succeeds)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE buffers_temp1 = generate_test_buffer_array(4, 1000);
    CONSTBUFFER_ARRAY_HANDLE empty = generate_test_buffer_array(3, 0);
    CONSTBUFFER_ARRAY_HANDLE buffers_temp2 = generate_test_buffer_array(6, 1000);

    CONSTBUFFER_ARRAY_HANDLE buffers_to_merge[4];
    buffers_to_merge[0] = buffers_temp1;
    buffers_to_merge[1] = empty;
    buffers_to_merge[2] = buffers_temp2;
    buffers_to_merge[3] = empty;
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_from_array_array(buffers_to_merge, sizeof(buffers_to_merge) / sizeof(buffers_to_merge[0]));

    real_constbuffer_array_dec_ref(buffers_temp1);
    real_constbuffer_array_dec_ref(empty);
    real_constbuffer_array_dec_ref(buffers_temp2);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(7, sizeof(CONSTBUFFER_HANDLE)));

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 1));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 2));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 3));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 4));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 5));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 6));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 7));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 8));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 9));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 10));
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 11));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 12));
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1000));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1000));

    // Doesn't check the last 3 empty buffers because all data is read

    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 7));

    for (uint32_t i = 0; i < 7; ++i)
    {
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    /// act
    CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1500);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result, &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 7, buffer_count);

    /// cleanup
    real_constbuffer_array_dec_ref(result);
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_multiple_buffers_1000_bytes_each_merge_and_split_fails_if_underlying_functions_fail)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(10, 1000);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc_2(7, sizeof(CONSTBUFFER_HANDLE)));

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 0))
        .CallCannotFail();

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 1))
        .CallCannotFail();
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 2))
        .CallCannotFail();
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 3))
        .CallCannotFail();
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 4))
        .CallCannotFail();
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 5))
        .CallCannotFail();
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 6))
        .CallCannotFail();
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 7))
        .CallCannotFail();
    // Read 500 bytes, 500 left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1500));
    // Read 500 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 8))
        .CallCannotFail();
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_content(buffers, 9))
        .CallCannotFail();
    // 1000 bytes left
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1500));

    STRICT_EXPECTED_CALL(malloc(1000));
    // Read 1000 bytes
    STRICT_EXPECTED_CALL(CONSTBUFFER_CreateWithMoveMemory(IGNORED_ARG, 1000));

    STRICT_EXPECTED_CALL(constbuffer_array_create(IGNORED_ARG, 7));

    for (uint32_t i = 0; i < 7; ++i)
    {
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1500);

            ///assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_empty_array_fails_if_underlying_functions_fail)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_empty();

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG))
        .CallCannotFail();

    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1024);

            ///assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

/*Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_42_017: [ If there are any other failures then constbuffer_array_splitter_split shall fail and return NULL. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_with_all_empty_buffers_fails_if_underlying_functions_fail)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(3, 0);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG))
        .CallCannotFail(); // technically this call can fail, but it is only due to overflow of the buffer sizes so not applicable to this case

    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            CONSTBUFFER_ARRAY_HANDLE result = constbuffer_array_splitter_split(buffers, 1500);

            ///assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

/* constbuffer_array_splitter_split_to_array_of_array */

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_001: [ If buffers is NULL then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_null_buffers_fails)
{
    ///arrange

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(NULL, 42);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_002: [ If max_buffer_size is 0 then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_0_max_buffer_size_fails)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(2, 2);

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 0);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NULL(result);

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_007: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_create_empty and store the created const buffer array in the first entry of the TARRAY that was created. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_024: [ If the total size for all buffers in buffers is 0 or buffer_count is 0: ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_021: [ constbuffer_array_splitter_split_to_array_of_array shall call TARRAY_CREATE_WITH_CAPACITY with size 1. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_empty_array_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_empty();

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1));
    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    //check buffer count in index 0
    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result->arr[0], &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, buffer_count);

    /// cleanup
    real_constbuffer_array_dec_ref(result->arr[0]);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
    real_constbuffer_array_dec_ref(buffers);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_024: [ If the total size for all buffers in buffers is 0 or buffer_count is 0: ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_021: [ constbuffer_array_splitter_split_to_array_of_array shall call TARRAY_CREATE_WITH_CAPACITY with size 1. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_007: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_create_empty and store the created const buffer array in the first entry of the TARRAY that was created. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_split_with_all_empty_buffers_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(3, 0);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1));
    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result->arr[0], &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 0, buffer_count);

    /// cleanup
    real_constbuffer_array_dec_ref(result->arr[0]);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
    real_constbuffer_array_dec_ref(buffers);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_022: [ If remaining_buffers_size is smaller or equal to max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall call TARRAY_CREATE_WITH_CAPACITY with size 1, inc ref the original buffer and return it. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_1_buffer_1_byte_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(1, 1);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1));
    STRICT_EXPECTED_CALL(constbuffer_array_inc_ref(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result->arr[0], &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 1, buffer_count);

    const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result->arr[0], 0);
    ASSERT_ARE_EQUAL(size_t, 1, buffer_temp->size);

    /// cleanup
    real_constbuffer_array_dec_ref(result->arr[0]);
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_022: [ If remaining_buffers_size is smaller or equal to max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall call TARRAY_CREATE_WITH_CAPACITY with size 1, inc ref the original buffer and return it. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_multiple_buffers_1_byte_each_merge_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(100, 1);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1));
    STRICT_EXPECTED_CALL(constbuffer_array_inc_ref(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result->arr[0], &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 100, buffer_count);

    for (uint32_t i = 0; i < 100; i++)
    {
        const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result->arr[0], i);
        ASSERT_ARE_EQUAL(size_t, 1, buffer_temp->size);
    }

    /// cleanup
    real_constbuffer_array_dec_ref(result->arr[0]);
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_022: [ If remaining_buffers_size is smaller or equal to max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall call TARRAY_CREATE_WITH_CAPACITY with size 1, inc ref the original buffer and return it. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_multiple_buffers_1000_bytes_each_merge_succeeds)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(10, 1000);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1));
    STRICT_EXPECTED_CALL(constbuffer_array_inc_ref(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1024 * 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t buffer_count;
    (void)real_constbuffer_array_get_buffer_count(result->arr[0], &buffer_count);
    ASSERT_ARE_EQUAL(uint32_t, 10, buffer_count);

    for (uint32_t i = 0; i < 10; i++)
    {
        const CONSTBUFFER* buffer_temp = real_constbuffer_array_get_buffer_content(result->arr[0], i);
        ASSERT_ARE_EQUAL(size_t, 1000, buffer_temp->size);
    }

    /// cleanup
    real_constbuffer_array_dec_ref(result->arr[0]);
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_023: [ constbuffer_array_splitter_split_to_array_of_array shall allocate a TARRAY of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_010: [ constbuffer_array_splitter_split_to_array_of_array shall initialize the start buffer index and offset to 0, current buffer count to 0 and end buffer size to 0. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_011: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer currently checking for the size by calling constbuffer_array_get_buffer. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_012: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer content by calling CONSTBUFFER_GetContent. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_016: [ If current buffer size added the current sub - tarray size is smaller than max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall include the current buffer to the current sub - tarray. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_018: [ If current buffer size added the current sub - tarray size is greater than max_buffer_size, then constbuffer_array_splitter_split_to_array_of_array shall get part of the current buffer as end buffer and added a new array into the result until the remaining size for the current buffer is smaller than max_buffer_size. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [ If current buffer is the last buffer in the original constbuffer_array, constbuffer_array_splitter_split_to_array_of_array shall store the sub - tarray with size smaller than max_buffer_size to result. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_multiple_buffers_1000_bytes_each_merge_and_split_succeeds)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(10, 1000);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(7));

    for (uint32_t i = 0; i < 7; i += 3)
    {
        //current_buffer_size == 1000, i == 0/3/6
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //get 500 in second buffer, i == 1/4/7 ,  =>first max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 1));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i, 2, 0, 500));

        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //current_buffer_size == 500 + 1000, i == 2/5/8 , => second max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 2));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 3));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i + 1, 2, 500, 1000));

        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    //for the last 1 x 1000 buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 9));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 9, 1, 0, 1000));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1500);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t count;
    uint32_t all_buffer_size;
    uint32_t buffer_count = 0;
    ASSERT_ARE_EQUAL(uint32_t, 7, result->capacity);
    for (uint32_t i = 0; i < 7; i++)
    {
        (void)real_constbuffer_array_get_buffer_count(result->arr[i], &count);
        (void)constbuffer_array_get_all_buffers_size(result->arr[i], &all_buffer_size);
        if (i != 6)
        {
            ASSERT_ARE_EQUAL(uint32_t, 2, count);
            ASSERT_ARE_EQUAL(uint32_t, 1500, all_buffer_size);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, 1, count);
            ASSERT_ARE_EQUAL(uint32_t, 1000, all_buffer_size);
        }
        buffer_count += count;
    }
    ASSERT_ARE_EQUAL(uint32_t, 13, buffer_count);

    /// cleanup
    for (uint32_t i = 0; i < 7; i++)
    {
        real_constbuffer_array_dec_ref(result->arr[i]);
    }
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_023: [ constbuffer_array_splitter_split_to_array_of_array shall allocate a TARRAY of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_010: [ constbuffer_array_splitter_split_to_array_of_array shall initialize the start buffer index and offset to 0, current buffer count to 0 and end buffer size to 0. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_011: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer currently checking for the size by calling constbuffer_array_get_buffer. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_012: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer content by calling CONSTBUFFER_GetContent. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_016: [ If current buffer size added the current sub - tarray size is smaller than max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall include the current buffer to the current sub - tarray. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_018: [ If current buffer size added the current sub - tarray size is greater than max_buffer_size, then constbuffer_array_splitter_split_to_array_of_array shall get part of the current buffer as end buffer and added a new array into the result until the remaining size for the current buffer is smaller than max_buffer_size. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [ If current buffer is the last buffer in the original constbuffer_array, constbuffer_array_splitter_split_to_array_of_array shall store the sub - tarray with size smaller than max_buffer_size to result. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_large_buffers_to_100MB_chunks_succeeds)
{
    /// arrange
    // Buffer sizes 201MB, 202MB, 203MB
    // Split at 100MB
    // 6x buffers at 100MB and 1 small buffer(6MB)
    // This simulates the actual size of splits for blob blocks and uses a split > UINT32_MAX
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array_increasing_size(3, 201 * 1024 * 1024, 1024 * 1024);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(7));

    //first buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 0));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    //get first 100MB
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 0, 1, 0, 100 * 1024 * 1024));

    //get second 100MB, 1MB left in first buffer
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 0, 1, 100 * 1024 * 1024, 100 * 1024 * 1024));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //second buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 1));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    //get first 100MB => 1MB from first buffer, 99MB from second buffer => 103MB left in second buffer
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 0, 2, 200 * 1024 * 1024, 99 * 1024 * 1024));
    //get second 100MB => 3 MB left in second buffer
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 1, 1, 99 * 1024 * 1024, 100 * 1024 * 1024));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //third buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 2));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    //get first 100MB => 3MB from second buffer, 97MB from third buffer => 106MB left in second buffer
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 1, 2, 199 * 1024 * 1024, 97 * 1024 * 1024));
    //get second 100MB => 6MB left in third buffer
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 2, 1, 97 * 1024 * 1024, 100 * 1024 * 1024));
    //last array get from third buffer with size 6MB
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 2, 1, 197 * 1024 * 1024, 6 * 1024 * 1024));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 100 * 1024 * 1024);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t count;
    uint32_t all_buffer_size;
    uint32_t buffer_count = 0;
    ASSERT_ARE_EQUAL(uint32_t, 7, result->capacity);
    for (uint32_t i = 0; i < 7; i++)
    {
        (void)real_constbuffer_array_get_buffer_count(result->arr[i], &count);
        (void)constbuffer_array_get_all_buffers_size(result->arr[i], &all_buffer_size);
        if (i == 2 || i == 4)
        {
            ASSERT_ARE_EQUAL(uint32_t, 2, count);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, 1, count);
        }

        if (i != 6)
        {
            ASSERT_ARE_EQUAL(size_t, 100 * 1024 * 1024, all_buffer_size);
        }
        else
        {
            ASSERT_ARE_EQUAL(size_t, 6 * 1024 * 1024, all_buffer_size);
        }
        buffer_count += count;
    }
    ASSERT_ARE_EQUAL(uint32_t, 9, buffer_count);

    /// cleanup
    for (uint32_t i = 0; i < 7; i++)
    {
        real_constbuffer_array_dec_ref(result->arr[i]);
    }
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_023: [ constbuffer_array_splitter_split_to_array_of_array shall allocate a TARRAY of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_010: [ constbuffer_array_splitter_split_to_array_of_array shall initialize the start buffer index and offset to 0, current buffer count to 0 and end buffer size to 0. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_011: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer currently checking for the size by calling constbuffer_array_get_buffer. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_012: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer content by calling CONSTBUFFER_GetContent. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_016: [ If current buffer size added the current sub - tarray size is smaller than max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall include the current buffer to the current sub - tarray. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_018: [ If current buffer size added the current sub - tarray size is greater than max_buffer_size, then constbuffer_array_splitter_split_to_array_of_array shall get part of the current buffer as end buffer and added a new array into the result until the remaining size for the current buffer is smaller than max_buffer_size. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [ If current buffer is the last buffer in the original constbuffer_array, constbuffer_array_splitter_split_to_array_of_array shall store the sub - tarray with size smaller than max_buffer_size to result. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_empty_buffers_at_start_succeeds)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE empty = generate_test_buffer_array(3, 0);
    CONSTBUFFER_ARRAY_HANDLE buffers_temp = generate_test_buffer_array(10, 1000);

    CONSTBUFFER_ARRAY_HANDLE buffers_to_merge[2];
    buffers_to_merge[0] = empty;
    buffers_to_merge[1] = buffers_temp;
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_from_array_array(buffers_to_merge, sizeof(buffers_to_merge) / sizeof(buffers_to_merge[0]));
    ASSERT_IS_NOT_NULL(buffers);

    real_constbuffer_array_dec_ref(buffers_temp);
    real_constbuffer_array_dec_ref(empty);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(7));

    //first three empty buffers
    for (uint32_t i = 0; i < 3; i++)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }

    for (uint32_t i = 3; i < 10; i += 3)
    {
        //current_buffer_size == 1000, i == 3/6/9
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //get 500 in second buffer, i == 4/7/10 ,  =>first max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 1));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        if (i == 3)
        {
            STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 0, 5, 0, 500));
        }
        else
        {
            STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i, 2, 0, 500));
        }


        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //current_buffer_size == 500 + 1000, i == 5/8/13 , => second max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 2));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 3));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i + 1, 2, 500, 1000));

        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    //for the last 1 x 1000 buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 12));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 12, 1, 0, 1000));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1500);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t count;
    uint32_t all_buffer_size;
    uint32_t buffer_count = 0;
    ASSERT_ARE_EQUAL(uint32_t, 7, result->capacity);
    for (uint32_t i = 0; i < 7; i++)
    {
        (void)real_constbuffer_array_get_buffer_count(result->arr[i], &count);
        (void)constbuffer_array_get_all_buffers_size(result->arr[i], &all_buffer_size);
        if (i == 0)
        {
            ASSERT_ARE_EQUAL(uint32_t, 5, count);
            ASSERT_ARE_EQUAL(uint32_t, 1500, all_buffer_size);
        }
        else if (i == 6)
        {
            ASSERT_ARE_EQUAL(uint32_t, 1, count);
            ASSERT_ARE_EQUAL(uint32_t, 1000, all_buffer_size);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, 2, count);
            ASSERT_ARE_EQUAL(uint32_t, 1500, all_buffer_size);
        }
        buffer_count += count;
    }
    ASSERT_ARE_EQUAL(uint32_t, 16, buffer_count);

    /// cleanup
    for (uint32_t i = 0; i < 7; i++)
    {
        real_constbuffer_array_dec_ref(result->arr[i]);
    }
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_023: [ constbuffer_array_splitter_split_to_array_of_array shall allocate a TARRAY of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_010: [ constbuffer_array_splitter_split_to_array_of_array shall initialize the start buffer index and offset to 0, current buffer count to 0 and end buffer size to 0. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_011: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer currently checking for the size by calling constbuffer_array_get_buffer. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_012: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer content by calling CONSTBUFFER_GetContent. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_016: [ If current buffer size added the current sub - tarray size is smaller than max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall include the current buffer to the current sub - tarray. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_018: [ If current buffer size added the current sub - tarray size is greater than max_buffer_size, then constbuffer_array_splitter_split_to_array_of_array shall get part of the current buffer as end buffer and added a new array into the result until the remaining size for the current buffer is smaller than max_buffer_size. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [ If current buffer is the last buffer in the original constbuffer_array, constbuffer_array_splitter_split_to_array_of_array shall store the sub - tarray with size smaller than max_buffer_size to result. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_empty_buffers_in_middle_succeeds)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE buffers_temp1 = generate_test_buffer_array(4, 1000);
    CONSTBUFFER_ARRAY_HANDLE empty = generate_test_buffer_array(3, 0);
    CONSTBUFFER_ARRAY_HANDLE buffers_temp2 = generate_test_buffer_array(6, 1000);

    CONSTBUFFER_ARRAY_HANDLE buffers_to_merge[4];
    buffers_to_merge[0] = buffers_temp1;
    buffers_to_merge[1] = empty;
    buffers_to_merge[2] = buffers_temp2;
    buffers_to_merge[3] = empty;
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_from_array_array(buffers_to_merge, sizeof(buffers_to_merge) / sizeof(buffers_to_merge[0]));
    ASSERT_IS_NOT_NULL(buffers);

    real_constbuffer_array_dec_ref(buffers_temp1);
    real_constbuffer_array_dec_ref(empty);
    real_constbuffer_array_dec_ref(buffers_temp2);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(7));

    //get first 1000 in index 0
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 0));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //get 500 in index 1 => first max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 1));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 0, 2, 0, 500));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //current_buffer_size == 500 + 1000, from buffer[1] and [2] , => second max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 2));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 3));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 1, 2, 500, 1000));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //get first 1000 in index 3 for third max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 3));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    //3 empty buffers in between
    for (uint32_t i = 4; i < 7; i++)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }

    //get 500 from buffer index 7 => 3th max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 7));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 3, 5, 0, 500));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //current_buffer_size == 500 + 1000, from buffer[7] and [8] , => 4th max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 8));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 9));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 7, 2, 500, 1000));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //get 1000 from buffer index 9
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 9));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //current_buffer_size == 1000+500, from buffer[9] and [10] , => 5th max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 10));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 9, 2, 0, 500));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //current_buffer_size == 500+1000, from buffer[10] and [11] , => 5th max_buffer => 6th max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 11));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 12));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 10, 2, 500, 1000));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //index 12 get 1000, index 13/14 is empty buffer
    for (uint32_t i = 12; i < 15; i++)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }

    //index 15 is empty and last buffer, need to result a sub-array from index 12
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 15));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 12, 4, 0, 0));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1500);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t count;
    uint32_t all_buffer_size;
    uint32_t buffer_count = 0;
    ASSERT_ARE_EQUAL(uint32_t, 7, result->capacity);
    for (uint32_t i = 0; i < 7; i++)
    {
        (void)real_constbuffer_array_get_buffer_count(result->arr[i], &count);
        (void)constbuffer_array_get_all_buffers_size(result->arr[i], &all_buffer_size);
        if (i == 2)
        {
            ASSERT_ARE_EQUAL(uint32_t, 5, count);
            ASSERT_ARE_EQUAL(uint32_t, 1500, all_buffer_size);
        }
        else if (i == 6)
        {
            ASSERT_ARE_EQUAL(uint32_t, 4, count);
            ASSERT_ARE_EQUAL(uint32_t, 1000, all_buffer_size);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, 2, count);
            ASSERT_ARE_EQUAL(uint32_t, 1500, all_buffer_size);

        }
        buffer_count += count;
    }
    ASSERT_ARE_EQUAL(uint32_t, 19, buffer_count);

    /// cleanup
    for (uint32_t i = 0; i < 7; i++)
    {
        real_constbuffer_array_dec_ref(result->arr[i]);
    }
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_005: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_buffer_count to get the total number of buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_008: [ constbuffer_array_splitter_split_to_array_of_array shall call constbuffer_array_get_all_buffers_size for buffers to obtain the total size of all buffers in buffers. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_023: [ constbuffer_array_splitter_split_to_array_of_array shall allocate a TARRAY of CONSTBUFFER_HANDLE of size remaining_buffer_size / max_buffer_size (rounded up). ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_010: [ constbuffer_array_splitter_split_to_array_of_array shall initialize the start buffer index and offset to 0, current buffer count to 0 and end buffer size to 0. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_011: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer currently checking for the size by calling constbuffer_array_get_buffer. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_012: [ constbuffer_array_splitter_split_to_array_of_array shall get the buffer content by calling CONSTBUFFER_GetContent. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_016: [ If current buffer size added the current sub - tarray size is smaller than max_buffer_size, constbuffer_array_splitter_split_to_array_of_array shall include the current buffer to the current sub - tarray. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_018: [ If current buffer size added the current sub - tarray size is greater than max_buffer_size, then constbuffer_array_splitter_split_to_array_of_array shall get part of the current buffer as end buffer and added a new array into the result until the remaining size for the current buffer is smaller than max_buffer_size. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_025: [ If current buffer size added the current sub - tarray size is equal to max_buffers_size, then constbuffer_array_splitter_split_to_array_of_array shall include any consecutive empty buffers right after the current buffer to the new array which will be added to the result. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_014: [ If current buffer is the last buffer in the original constbuffer_array, constbuffer_array_splitter_split_to_array_of_array shall store the sub - tarray with size smaller than max_buffer_size to result. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_020: [ constbuffer_array_splitter_split_to_array_of_array shall succeed and return the new TARRAY(CONSTBUFFER_ARRAY_HANDLE). ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_empty_buffers_in_middle_and_right_after_end_of_a_new_sub_tarray_succeeds)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (2 x 1500 and 4 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE buffers_temp1 = generate_test_buffer_array(3, 1000);
    CONSTBUFFER_ARRAY_HANDLE empty = generate_test_buffer_array(3, 0);
    CONSTBUFFER_ARRAY_HANDLE buffers_temp2 = generate_test_buffer_array(7, 1000);

    CONSTBUFFER_ARRAY_HANDLE buffers_to_merge[4];
    buffers_to_merge[0] = buffers_temp1;
    buffers_to_merge[1] = empty;
    buffers_to_merge[2] = buffers_temp2;
    buffers_to_merge[3] = empty;
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_from_array_array(buffers_to_merge, sizeof(buffers_to_merge) / sizeof(buffers_to_merge[0]));
    ASSERT_IS_NOT_NULL(buffers);

    real_constbuffer_array_dec_ref(buffers_temp1);
    real_constbuffer_array_dec_ref(empty);
    real_constbuffer_array_dec_ref(buffers_temp2);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(7));

    //get first 1000 in index 0
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 0));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //get 500 in index 1 => first max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 1));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 0, 2, 0, 500));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    //current_buffer_size == 500 + 1000, from buffer[1] and [2] , => second max_buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 2));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    //index 3/4/5 empty buffers, should be included in this constbuffer array
    for (uint32_t i = 3; i < 7; i++)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i)); //6
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 1, 5, 500, 0));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    for (uint32_t i = 6; i < 11; i+=3)
    {
        //get first 1000 in index 6/9
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //get 500 in index 7/10 => 3/5th max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 1));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i, 2, 0, 500));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //500 in index 7/10 + 1000 in index 8/11 => 4/6th max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 2));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 3));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i + 1, 2, 500, 1000));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }

    //index 12 get 1000, index 13/14 is empty buffer
    for (uint32_t i = 12; i < 15; i++)
    {
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i));
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }

    //index 15 is empty and last buffer, need to result a sub-array from index 12
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 15));
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG));
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 12, 4, 0, 0));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    /// act
    TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1500);

    /// assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(result);

    uint32_t count;
    uint32_t all_buffer_size;
    uint32_t buffer_count = 0;
    ASSERT_ARE_EQUAL(uint32_t, 7, result->capacity);
    for (uint32_t i = 0; i < 7; i++)
    {
        (void)real_constbuffer_array_get_buffer_count(result->arr[i], &count);
        (void)constbuffer_array_get_all_buffers_size(result->arr[i], &all_buffer_size);
        if (i == 1)
        {
            ASSERT_ARE_EQUAL(uint32_t, 5, count);
            ASSERT_ARE_EQUAL(uint32_t, 1500, all_buffer_size);
        }
        else if (i == 6)
        {
            ASSERT_ARE_EQUAL(uint32_t, 4, count);
            ASSERT_ARE_EQUAL(uint32_t, 1000, all_buffer_size);
        }
        else
        {
            ASSERT_ARE_EQUAL(uint32_t, 2, count);
            ASSERT_ARE_EQUAL(uint32_t, 1500, all_buffer_size);
        }
        buffer_count += count;
    }
    ASSERT_ARE_EQUAL(uint32_t, 19, buffer_count);

    /// cleanup
    for (uint32_t i = 0; i < 7; i++)
    {
        real_constbuffer_array_dec_ref(result->arr[i]);
    }
    real_constbuffer_array_dec_ref(buffers);
    TARRAY_ASSIGN(CONSTBUFFER_ARRAY_HANDLE)(&result, NULL);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_019: [ On any failure, constbuffer_array_splitter_split_to_array_of_array dec ref the sub - tarrays by calling constbuffer_array_dec_ref. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_multiple_buffers_1000_bytes_each_merge_and_split_fails_if_underlying_functions_fail)
{
    /// arrange
    // 10 x 1000 bytes
    // Split into 1500 bytes each (6 x 1500 and 1 x 1000)
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(10, 1000);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(7));

    for (uint32_t i = 0; i < 7; i += 3)
    {
        //current_buffer_size == 1000, i == 0/3/6
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //get 500 in second buffer, i == 1/4/7 ,  =>first max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 1))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i, 2, 0, 500));

        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

        //current_buffer_size == 500 + 1000, i == 2/5/8 , => second max_buffer
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 2))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, i + 3))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
        STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, i + 1, 2, 500, 1000));

        STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));
    }
    //for the last 1 x 1000 buffer
    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer(buffers, 9))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(CONSTBUFFER_GetContent(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(constbuffer_array_create_from_buffer_offset_and_count(buffers, 9, 1, 0, 1000));
    STRICT_EXPECTED_CALL(CONSTBUFFER_DecRef(IGNORED_ARG));

    STRICT_EXPECTED_CALL(TARRAY_INITIALIZE_MOVE(CONSTBUFFER_ARRAY_HANDLE)(IGNORED_ARG, IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1500);

            ///assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_019: [ On any failure, constbuffer_array_splitter_split_to_array_of_array dec ref the sub - tarrays by calling constbuffer_array_dec_ref. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_empty_array_fails_if_underlying_functions_fail)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = real_constbuffer_array_create_empty();

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1));
    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1024);

            ///assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_004: [ If there are any other failures then constbuffer_array_splitter_split_to_array_of_array shall fail and return NULL. ]*/
/* Tests_SRS_CONSTBUFFER_ARRAY_SPLITTER_07_019: [ On any failure, constbuffer_array_splitter_split_to_array_of_array dec ref the sub - tarrays by calling constbuffer_array_dec_ref. ]*/
TEST_FUNCTION(constbuffer_array_splitter_split_to_array_of_array_with_all_empty_buffers_fails_if_underlying_functions_fail)
{
    /// arrange
    CONSTBUFFER_ARRAY_HANDLE buffers = generate_test_buffer_array(3, 0);

    STRICT_EXPECTED_CALL(constbuffer_array_get_buffer_count(buffers, IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(constbuffer_array_get_all_buffers_size(buffers, IGNORED_ARG));
    STRICT_EXPECTED_CALL(TARRAY_CREATE_WITH_CAPACITY(CONSTBUFFER_ARRAY_HANDLE)(1));
    STRICT_EXPECTED_CALL(constbuffer_array_create_empty());

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            TARRAY(CONSTBUFFER_ARRAY_HANDLE) result = constbuffer_array_splitter_split_to_array_of_array(buffers, 1500);

            ///assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }

    /// cleanup
    real_constbuffer_array_dec_ref(buffers);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
