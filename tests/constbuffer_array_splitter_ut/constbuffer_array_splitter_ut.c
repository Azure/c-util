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
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"


#include "../reals/real_constbuffer.h"
#include "../reals/real_constbuffer_array.h"

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

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(constbuffer_array_get_all_buffers_size, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(CONSTBUFFER_CreateWithMoveMemory, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(constbuffer_array_create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(constbuffer_array_create_empty, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_ARRAY_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(CONSTBUFFER_HANDLE, void*);
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

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
