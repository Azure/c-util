// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_logging/logger.h"

#include "c_pal/thandle.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_util/buffer_.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_format.h"
#include "c_util/constbuffer_version.h"
#include "c_util/memory_data.h"
#include "c_util/constbuffer_thandle.h"

TEST_DEFINE_ENUM_TYPE(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_VALUES)

static const char* buffer1 = "le buffer no 1";

#define BUFFER1_HANDLE (BUFFER_HANDLE)1
#define BUFFER1_u_char ((unsigned char*)buffer1)
#define BUFFER1_length (uint32_t)strlen(buffer1)

static unsigned char* my_BUFFER_u_char(BUFFER_HANDLE handle)
{
    unsigned char* result;
    if (handle == BUFFER1_HANDLE)
    {
        result = BUFFER1_u_char;
    }
    else
    {
        result = NULL;
        ASSERT_FAIL("who am I?");
    }
    return result;
}

static size_t my_BUFFER_length(BUFFER_HANDLE handle)
{
    size_t result;
    if (handle == BUFFER1_HANDLE)
    {
        result = BUFFER1_length;
    }
    else
    {
        result = 0;
        ASSERT_FAIL("who am I?");
    }
    return result;
}

MOCK_FUNCTION_WITH_CODE(, void, test_free_func_with_free, void*, context)
    real_gballoc_hl_free(context);
MOCK_FUNCTION_END()

MOCK_FUNCTION_WITH_CODE(, void, test_free_func_no_free, void*, context)
    (void)context; // Mock function that does not actually free memory
MOCK_FUNCTION_END()

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(constbuffer_thandle_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result = real_gballoc_hl_init(NULL, NULL);
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_u_char, my_BUFFER_u_char);
    REGISTER_GLOBAL_MOCK_HOOK(BUFFER_length, my_BUFFER_length);
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

/* CONSTBUFFER_THANDLE_Create */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_001: [ If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_Create shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_with_source_NULL_and_size_not_0_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_Create(NULL, 1);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_002: [ Otherwise, CONSTBUFFER_THANDLE_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_004: [ Otherwise CONSTBUFFER_THANDLE_Create shall return a non-NULL handle.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_005: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_Create shall have its ref count set to "1".]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_with_valid_args_succeeds)
{
    ///arrange
    unsigned char source[] = { 1, 2, 3 };

    STRICT_EXPECTED_CALL(gballoc_hl_malloc_flex(IGNORED_ARG, sizeof(source), 1));

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_Create(source, sizeof(source));

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    /*Tests_SRS_CONSTBUFFER_THANDLE_88_013: [ CONSTBUFFER_dispose shall free the memory used by the const buffer.]*/
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_002: [ Otherwise, CONSTBUFFER_THANDLE_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_004: [ Otherwise CONSTBUFFER_THANDLE_Create shall return a non-NULL handle.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_005: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_Create shall have its ref count set to "1".]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_with_source_NULL_and_size_0_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_hl_malloc_flex(IGNORED_ARG, 0, 1));

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_Create(NULL, 0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_102: [ If source is non-NULL and size is 0 then CONSTBUFFER_THANDLE_Create shall create an empty buffer.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_004: [ Otherwise CONSTBUFFER_THANDLE_Create shall return a non-NULL handle.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_005: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_Create shall have its ref count set to "1".]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_with_source_non_NULL_and_size_0_succeeds)
{
    ///arrange
    unsigned char source[] = { 1, 2, 3 };

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_Create(source, 0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_IS_NULL(content->buffer);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_003: [ If creating the copy fails then CONSTBUFFER_THANDLE_Create shall return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_fails_when_malloc_fails)
{
    ///arrange
    unsigned char source[] = { 1, 2, 3 };
    
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1))
        .SetReturn(NULL);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_Create(source, sizeof(source));

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/* CONSTBUFFER_THANDLE_GetContent */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_011: [ If constbufferHandle is NULL then CONSTBUFFER_THANDLE_GetContent shall return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetContent_with_NULL_fails)
{
    ///arrange

    ///act
    const CONSTBUFFER_THANDLE* result = CONSTBUFFER_THANDLE_GetContent(NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_012: [ Otherwise, CONSTBUFFER_THANDLE_GetContent shall return a pointer to a CONSTBUFFER_THANDLE structure.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetContent_succeeds)
{
    ///arrange
    unsigned char source[] = { 1, 2, 3 };
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(source, sizeof(source));
    ASSERT_IS_NOT_NULL(handle);
    umock_c_reset_all_calls();

    ///act
    const CONSTBUFFER_THANDLE* result = CONSTBUFFER_THANDLE_GetContent(handle);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    /*testing the "copy"*/
    ASSERT_ARE_EQUAL(uint32_t, sizeof(source), result->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(source, result->buffer, sizeof(source)));
    /*testing that it is a copy and not a pointer assignment*/
    ASSERT_ARE_NOT_EQUAL(void_ptr, source, result->buffer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_012: [ Otherwise, CONSTBUFFER_THANDLE_GetContent shall return a pointer to a CONSTBUFFER_THANDLE structure.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetContent_with_empty_buffer_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(handle);
    umock_c_reset_all_calls();

    ///act
    const CONSTBUFFER_THANDLE* result = CONSTBUFFER_THANDLE_GetContent(handle);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 0, result->size);
    ASSERT_IS_NULL(result->buffer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_012: [ Otherwise, CONSTBUFFER_THANDLE_GetContent shall return a pointer to a CONSTBUFFER_THANDLE structure.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetContent_with_single_byte_succeeds)
{
    ///arrange
    unsigned char source[] = { 0x42 };
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(source, sizeof(source));
    ASSERT_IS_NOT_NULL(handle);
    umock_c_reset_all_calls();

    ///act
    const CONSTBUFFER_THANDLE* result = CONSTBUFFER_THANDLE_GetContent(handle);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, 1, result->size);
    ASSERT_ARE_EQUAL(uint8_t, 0x42, result->buffer[0]);
    ASSERT_ARE_NOT_EQUAL(void_ptr, source, result->buffer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/* CONSTBUFFER_THANDLE_CreateWithMoveMemory */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_015: [ If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_with_NULL_source_and_size_not_0_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateWithMoveMemory(NULL, 1);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_016: [ CONSTBUFFER_THANDLE_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_019: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithMoveMemory shall have its ref count set to "1".]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_succeeds)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 42;
    test_buffer[1] = 43;
    umock_c_reset_all_calls();

    ///act
    STRICT_EXPECTED_CALL(gballoc_hl_malloc(IGNORED_ARG));
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithMoveMemory(test_buffer, 2);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "storage"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 2, content->size);
    ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer, "same buffer should be returned");
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_016: [ CONSTBUFFER_THANDLE_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_018: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_with_0_size_succeeds)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 42;
    test_buffer[1] = 43;
    umock_c_reset_all_calls();

    ///act
    STRICT_EXPECTED_CALL(gballoc_hl_malloc(IGNORED_ARG));
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithMoveMemory(test_buffer, 0);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "storage"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_016: [ CONSTBUFFER_THANDLE_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_with_NULL_source_and_0_size_succeeds)
{
    ///arrange
    umock_c_reset_all_calls();

    ///act
    STRICT_EXPECTED_CALL(gballoc_hl_malloc(IGNORED_ARG));
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithMoveMemory(NULL, 0);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "storage"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_IS_NULL(content->buffer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_014: [ If the buffer was created by calling CONSTBUFFER_THANDLE_CreateWithMoveMemory, the memory pointed to by the buffer pointer shall be freed.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_frees_moved_memory_on_dispose)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 0xAA;
    test_buffer[1] = 0xBB;
    test_buffer[2] = 0xCC;
    
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithMoveMemory(test_buffer, 3);
    ASSERT_IS_NOT_NULL(handle);
    
    // Verify the buffer content is accessible
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 3, content->size);
    ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer);
    ASSERT_ARE_EQUAL(uint8_t, 0xAA, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 0xBB, content->buffer[1]);
    ASSERT_ARE_EQUAL(uint8_t, 0xCC, content->buffer[2]);

    umock_c_reset_all_calls();

    // Expect the free calls that will happen during disposal
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG)); // free the moved memory
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG)); // free the THANDLE structure

    ///act & cleanup
    // When we release the handle, it should free the moved memory
    // We can't directly test that the memory was freed, but we can verify
    // that the disposal completes without error
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);

    ///assert
    // If we get here without crash/error, the disposal worked correctly
    // The memory pointed to by test_buffer is now freed and should not be accessed
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_017: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWithMoveMemory shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_fails_when_malloc_fails)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 43;
    test_buffer[1] = 44;

    STRICT_EXPECTED_CALL(gballoc_hl_malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateWithMoveMemory(test_buffer, 2);

    ///assert
    ASSERT_IS_NULL(result);

    ///cleanup
    real_gballoc_hl_free(test_buffer);
}

/* CONSTBUFFER_THANDLE_CreateFromBuffer */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_006: [ If buffer is NULL then CONSTBUFFER_THANDLE_CreateFromBuffer shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromBuffer_with_NULL_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromBuffer(NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_007: [ Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall copy the content of buffer.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_009: [ Otherwise, CONSTBUFFER_THANDLE_CreateFromBuffer shall return a non-NULL handle.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_010: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateFromBuffer shall have its ref count set to "1".]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromBuffer_succeeds)
{
    ///arrange
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(BUFFER_length(BUFFER1_HANDLE));
    STRICT_EXPECTED_CALL(BUFFER_u_char(BUFFER1_HANDLE));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1));

    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateFromBuffer(BUFFER1_HANDLE);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "copy"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, BUFFER1_length, content->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER1_u_char, content->buffer, BUFFER1_length));
    /*testing that it is a copy and not a pointer assignment*/
    ASSERT_ARE_NOT_EQUAL(void_ptr, BUFFER1_u_char, content->buffer);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_008: [ If copying the content fails, then CONSTBUFFER_THANDLE_CreateFromBuffer shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromBuffer_fails_when_malloc_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(BUFFER_length(BUFFER1_HANDLE));
    STRICT_EXPECTED_CALL(BUFFER_u_char(BUFFER1_HANDLE));
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromBuffer(BUFFER1_HANDLE);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/* CONSTBUFFER_THANDLE_contain_same */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_020: [ If left is NULL and right is NULL then CONSTBUFFER_THANDLE_contain_same shall return true.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_with_left_NULL_and_right_NULL_returns_true)
{
    ///arrange

    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(NULL, NULL);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_021: [ If left is NULL and right is not NULL then CONSTBUFFER_THANDLE_contain_same shall return false.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_with_left_NULL_and_right_not_NULL_returns_false)
{
    ///arrange
    unsigned char rightSource = 'r';
    THANDLE(CONSTBUFFER) right = CONSTBUFFER_THANDLE_Create(&rightSource, sizeof(rightSource));
    ASSERT_IS_NOT_NULL(right);
    umock_c_reset_all_calls();

    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(NULL, right);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&right, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_022: [ If left is not NULL and right is NULL then CONSTBUFFER_THANDLE_contain_same shall return false.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_with_left_not_NULL_and_right_NULL_returns_false)
{
    ///arrange
    unsigned char leftSource = 'l';
    THANDLE(CONSTBUFFER) left = CONSTBUFFER_THANDLE_Create(&leftSource, sizeof(leftSource));
    ASSERT_IS_NOT_NULL(left);
    umock_c_reset_all_calls();

    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(left, NULL);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&left, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_023: [ If left's size is different than right's size then CONSTBUFFER_THANDLE_contain_same shall return false.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_with_left_and_right_sizes_not_equal_returns_false)
{
    ///arrange
    unsigned char leftSource = 'l';
    THANDLE(CONSTBUFFER) left = CONSTBUFFER_THANDLE_Create(&leftSource, sizeof(leftSource));
    ASSERT_IS_NOT_NULL(left);

    unsigned char rightSource[2] = { 'r', 'r' };
    THANDLE(CONSTBUFFER) right = CONSTBUFFER_THANDLE_Create(rightSource, sizeof(rightSource));
    ASSERT_IS_NOT_NULL(right);
    umock_c_reset_all_calls();

    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(left, right);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&left, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&right, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_024: [ If left's buffer contains different bytes than right's buffer then CONSTBUFFER_THANDLE_contain_same shall return false.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_with_left_and_right_content_not_equal_returns_false)
{
    ///arrange
    unsigned char leftSource[2] = { 'l', 'l' };
    THANDLE(CONSTBUFFER) left = CONSTBUFFER_THANDLE_Create(leftSource, sizeof(leftSource));
    ASSERT_IS_NOT_NULL(left);

    unsigned char rightSource[2] = { 'r', 'r' };
    THANDLE(CONSTBUFFER) right = CONSTBUFFER_THANDLE_Create(rightSource, sizeof(rightSource));
    ASSERT_IS_NOT_NULL(right);
    umock_c_reset_all_calls();

    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(left, right);

    ///assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&left, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&right, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_025: [ Otherwise, CONSTBUFFER_THANDLE_contain_same shall return true.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_with_left_and_right_same_returns_true)
{
    ///arrange
    unsigned char leftSource[2] = { '1', '2' };
    THANDLE(CONSTBUFFER) left = CONSTBUFFER_THANDLE_Create(leftSource, sizeof(leftSource));
    ASSERT_IS_NOT_NULL(left);

    unsigned char rightSource[2] = { '1', '2' };
    THANDLE(CONSTBUFFER) right = CONSTBUFFER_THANDLE_Create(rightSource, sizeof(rightSource));
    ASSERT_IS_NOT_NULL(right);
    umock_c_reset_all_calls();

    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(left, right);

    ///assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&left, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&right, NULL);
}

/* CONSTBUFFER_THANDLE_CreateFromOffsetAndSize */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_035: [ If handle is NULL then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_NULL_handle_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(NULL, 0, 0);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_037: [ If offset is greater than handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_offset_too_large_fails)
{
    ///arrange
    unsigned char test_buffer[4] = { 1, 2, 3, 4 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    umock_c_reset_all_calls();

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 5, 1);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_039: [ If offset + size exceed handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_offset_plus_size_too_large_fails)
{
    ///arrange
    unsigned char test_buffer[4] = { 1, 2, 3, 4 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    umock_c_reset_all_calls();

    ///act
    THANDLE(CONSTBUFFER) result1 = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 0, 5);
    THANDLE(CONSTBUFFER) result2 = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 4, 1);
    THANDLE(CONSTBUFFER) result3 = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 2, 3);

    ///assert
    ASSERT_IS_NULL(result1);
    ASSERT_IS_NULL(result2);
    ASSERT_IS_NULL(result3);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_038: [ If offset + size would overflow then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_overflow_fails)
{
    ///arrange
    unsigned char test_buffer[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 5, UINT32_MAX - 2);

    ///assert
    ASSERT_IS_NULL(result);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_040: [ If offset is 0 and size is equal to handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall increment the reference count of handle and return handle.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_full_buffer_returns_same_handle)
{
    ///arrange
    unsigned char test_buffer[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 0, 6);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    // Should return the same handle for full buffer
    ASSERT_IS_TRUE(result == source);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_041: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_043: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the buffer pointer to point to handle's buffer + offset.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_044: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the size to the provided size.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_046: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER to 1.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_047: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall succeed and return a non-NULL value.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_offset_and_size_succeeds)
{
    ///arrange
    unsigned char test_buffer[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);
    const CONSTBUFFER_THANDLE* source_content = CONSTBUFFER_THANDLE_GetContent(source);
    ASSERT_IS_NOT_NULL(source_content);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 2, 3);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    // Should be a different handle since it's not the full buffer
    ASSERT_IS_TRUE(result != source);
    
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 3, content->size);
    // Buffer pointer should point to source buffer + offset (no copy, just pointer arithmetic)
    ASSERT_IS_TRUE(source_content->buffer + 2 == content->buffer);
    // Verify the data is as expected (pointing to the right location)
    ASSERT_ARE_EQUAL(uint8_t, 0x33, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 0x44, content->buffer[1]);
    ASSERT_ARE_EQUAL(uint8_t, 0x55, content->buffer[2]);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    /*Tests_SRS_CONSTBUFFER_THANDLE_88_048: [ If the buffer was created by calling CONSTBUFFER_THANDLE_CreateFromOffsetAndSize, the original handle shall be assigned to NULL.]*/
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_043: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the buffer pointer to point to handle's buffer + offset.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_044: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the size to the provided size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_zero_size_succeeds)
{
    ///arrange
    unsigned char test_buffer[4] = { 1, 2, 3, 4 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 2, 0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    // For zero size, buffer pointer behavior may vary but should be consistent

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_043: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the buffer pointer to point to handle's buffer + offset.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_044: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall set the size to the provided size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_single_byte_succeeds)
{
    ///arrange
    unsigned char test_buffer[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);
    const CONSTBUFFER_THANDLE* source_content = CONSTBUFFER_THANDLE_GetContent(source);
    ASSERT_IS_NOT_NULL(source_content);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 3, 1);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 1, content->size);
    ASSERT_IS_TRUE(source_content->buffer + 3 == content->buffer);
    ASSERT_ARE_EQUAL(uint8_t, 0xDD, content->buffer[0]);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_045: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall increment the reference count of handle and store it.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_keeps_original_alive)
{
    ///arrange
    unsigned char test_buffer[4] = { 1, 2, 3, 4 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);
    const CONSTBUFFER_THANDLE* source_content = CONSTBUFFER_THANDLE_GetContent(source);
    ASSERT_IS_NOT_NULL(source_content);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 1, 2);
    ASSERT_IS_NOT_NULL(result);
    
    // Release the original source handle
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    
    // The offset buffer should still be valid because it holds a reference to the original
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 2, content->size);
    ASSERT_ARE_EQUAL(uint8_t, 2, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 3, content->buffer[1]);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_047: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall succeed and return a non-NULL value.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_empty_source_buffer_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(source, 0, 0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    // Should return the same handle for full buffer (0,0 of empty buffer)
    ASSERT_IS_TRUE(result == source);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_042: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSize shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_fails_when_error_occurs)
{
    ///arrange
    // Test basic error condition with invalid input
    
    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(NULL, 1, 2);

    ///assert
    ASSERT_IS_NULL(result);

    ///cleanup
    // No cleanup needed
}

/* CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_049: [ If handle is NULL, CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_NULL_handle_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(NULL, 0, 0);

    ///assert
    ASSERT_IS_NULL(result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_056: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall succeed and return a non-NULL value.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_succeeds)
{
    ///arrange
    unsigned char test_buffer[4] = { 42, 43, 44, 45 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 1, 2);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 2, content->size);
    ASSERT_ARE_EQUAL(uint8_t, 43, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 44, content->buffer[1]);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_051: [ If offset is greater than handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_offset_too_large_fails)
{
    ///arrange
    unsigned char test_buffer[2] = { 42, 43 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 3, 1);

    ///assert
    ASSERT_IS_NULL(result);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_053: [ If offset + size exceed handle's size then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_offset_plus_size_too_large_fails)
{
    ///arrange
    unsigned char test_buffer[2] = { 42, 43 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 1, 2);

    ///assert
    ASSERT_IS_NULL(result);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_052: [ If offset + size would overflow then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_overflow_fails)
{
    ///arrange
    unsigned char test_buffer[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 5, UINT32_MAX - 2);

    ///assert
    ASSERT_IS_NULL(result);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_zero_size_succeeds)
{
    ///arrange
    unsigned char test_buffer[4] = { 42, 43, 44, 45 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 2, 0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_IS_NULL(content->buffer);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_single_byte_succeeds)
{
    ///arrange
    unsigned char test_buffer[5] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 2, 1);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 1, content->size);
    ASSERT_ARE_EQUAL(uint8_t, 0xCC, content->buffer[0]);
    // Verify it's a copy, not the same pointer
    ASSERT_IS_TRUE(test_buffer + 2 != content->buffer);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_multiple_bytes_from_middle_succeeds)
{
    ///arrange
    unsigned char test_buffer[6] = { 1, 2, 3, 4, 5, 6 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 2, 3);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 3, content->size);
    ASSERT_ARE_EQUAL(uint8_t, 3, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 4, content->buffer[1]);
    ASSERT_ARE_EQUAL(uint8_t, 5, content->buffer[2]);
    // Verify it's a copy, not the same pointer
    ASSERT_IS_TRUE(test_buffer + 2 != content->buffer);
    // Verify the data is the same as the original
    ASSERT_ARE_EQUAL(int, 0, memcmp(content->buffer, test_buffer + 2, 3));

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_full_buffer_copy_succeeds)
{
    ///arrange
    unsigned char test_buffer[3] = { 0x11, 0x22, 0x33 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 0, 3);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 3, content->size);
    ASSERT_ARE_EQUAL(uint8_t, 0x11, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 0x22, content->buffer[1]);
    ASSERT_ARE_EQUAL(uint8_t, 0x33, content->buffer[2]);
    // Verify it's a copy, not the same pointer
    ASSERT_IS_TRUE(test_buffer != content->buffer);
    // Verify the data is the same as the original
    ASSERT_ARE_EQUAL(int, 0, memcmp(content->buffer, test_buffer, 3));

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_054: [ CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall create a new const buffer by copying data from handle's buffer starting at offset and with the given size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_with_empty_source_buffer_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(source);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source, 0, 0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(result);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_IS_NULL(content->buffer);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&result, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_055: [ If there are any failures then CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_fails_when_malloc_fails)
{
    ///arrange
    unsigned char source[] = { 1, 2, 3, 4, 5 };
    THANDLE(CONSTBUFFER) source_handle = CONSTBUFFER_THANDLE_Create(source, sizeof(source));
    ASSERT_IS_NOT_NULL(source_handle);
    umock_c_reset_all_calls();

    // Mock malloc_flex to fail - this will cause the copy operation to fail
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, 1))
        .SetReturn(NULL);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(source_handle, 1, 2);

    ///assert
    /*Tests_SRS_CONSTBUFFER_THANDLE_88_055: [ Function should return NULL on failures (malloc failure)]*/
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source_handle, NULL);
}

/* CONSTBUFFER_THANDLE_get_serialization_size */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_057: [ If source is NULL then CONSTBUFFER_THANDLE_get_serialization_size shall fail and return 0.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_get_serialization_size_with_source_NULL_fails)
{
    ///arrange

    ///act
    uint32_t result = CONSTBUFFER_THANDLE_get_serialization_size(NULL);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_058: [ If sizeof(uint8_t) + sizeof(uint32_t) + source's size exceed UINT32_MAX then CONSTBUFFER_THANDLE_get_serialization_size shall fail and return 0.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_get_serialization_size_with_too_big_size_fails)
{
    ///arrange
    // Create a normal buffer for testing (overflow condition is hard to test in practice)
    unsigned char test_buffer[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    THANDLE(CONSTBUFFER) normal_h = CONSTBUFFER_THANDLE_Create(test_buffer, sizeof(test_buffer));
    ASSERT_IS_NOT_NULL(normal_h);

    umock_c_reset_all_calls();

    ///act
    uint32_t result = CONSTBUFFER_THANDLE_get_serialization_size(normal_h);

    ///assert
    // For normal sizes, should succeed
    ASSERT_ARE_EQUAL(uint32_t, CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + 10, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&normal_h, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_059: [ Otherwise CONSTBUFFER_THANDLE_get_serialization_size shall succeed and return sizeof(uint8_t) + sizeof(uint32_t) + source's size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_get_serialization_size_with_0_size_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER) smallest = CONSTBUFFER_THANDLE_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(smallest);

    umock_c_reset_all_calls();

    ///act
    uint32_t result = CONSTBUFFER_THANDLE_get_serialization_size(smallest);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&smallest, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_059: [ Otherwise CONSTBUFFER_THANDLE_get_serialization_size shall succeed and return sizeof(uint8_t) + sizeof(uint32_t) + source's size.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_get_serialization_size_with_2_size_succeeds)
{
    ///arrange
    unsigned char test_data[2] = { 42, 43 };
    THANDLE(CONSTBUFFER) small_buffer = CONSTBUFFER_THANDLE_Create(test_data, sizeof(test_data));
    ASSERT_IS_NOT_NULL(small_buffer);

    umock_c_reset_all_calls();

    ///act
    uint32_t result = CONSTBUFFER_THANDLE_get_serialization_size(small_buffer);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + 2, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&small_buffer, NULL);
}

/* CONSTBUFFER_THANDLE_to_buffer */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_060: [ If source is NULL then CONSTBUFFER_THANDLE_to_buffer shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_buffer_with_source_NULL_fails)
{
    ///arrange
    uint32_t size;

    ///act
    unsigned char* result = CONSTBUFFER_THANDLE_to_buffer(NULL, NULL, NULL, &size);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_061: [ If serialized_size is NULL then CONSTBUFFER_THANDLE_to_buffer shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_buffer_with_size_NULL_fails)
{
    ///arrange
    unsigned char test_data[1] = { 42 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_data, sizeof(test_data));
    ASSERT_IS_NOT_NULL(source);

    umock_c_reset_all_calls();

    ///act
    unsigned char* result = CONSTBUFFER_THANDLE_to_buffer(source, NULL, NULL, NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_062: [ If alloc is NULL then CONSTBUFFER_THANDLE_to_buffer shall use malloc as provided by gballoc_hl_redirect.h.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_063: [ CONSTBUFFER_THANDLE_to_buffer shall allocate memory using alloc for holding the complete serialization.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_064: [ CONSTBUFFER_THANDLE_to_buffer shall write at offset 0 of the allocated memory the version of the serialization (currently 1).]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_065: [ CONSTBUFFER_THANDLE_to_buffer shall write at offsets 1-4 of the allocated memory the value of source's size in network byte order.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_066: [ CONSTBUFFER_THANDLE_to_buffer shall write starting at offset 5 of the allocated memory the bytes of source's buffer.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_067: [ CONSTBUFFER_THANDLE_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_buffer_with_size_1_with_malloc_succeeds)
{
    ///arrange
    unsigned char test_data[1] = { 42 };
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(test_data, sizeof(test_data));
    ASSERT_IS_NOT_NULL(source);
    uint32_t size;

    ///act
    unsigned char* result = CONSTBUFFER_THANDLE_to_buffer(source, NULL, NULL, &size);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE + 1, size);
    
    // Check version byte
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, result[0]);
    
    // Check size bytes (network byte order)
    uint32_t read_size;
    read_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, &read_size);
    ASSERT_ARE_EQUAL(uint32_t, 1, read_size);
    
    // Check content
    ASSERT_ARE_EQUAL(uint8_t, 42, result[CONSTBUFFER_CONTENT_OFFSET]);

    ///cleanup
    free(result);
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_067: [ CONSTBUFFER_THANDLE_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_buffer_with_0_size_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(source);
    uint32_t size;

    ///act
    unsigned char* result = CONSTBUFFER_THANDLE_to_buffer(source, NULL, NULL, &size);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, CONSTBUFFER_VERSION_SIZE + CONSTBUFFER_SIZE_SIZE, size);
    
    // Check version byte
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, result[0]);
    
    // Check size bytes (network byte order)
    uint32_t read_size;
    read_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, &read_size);
    ASSERT_ARE_EQUAL(uint32_t, 0, read_size);

    ///cleanup
    free(result);
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/* CONSTBUFFER_THANDLE_to_fixed_size_buffer */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_069: [ If source is NULL then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_fixed_size_buffer_with_source_NULL_fails)
{
    ///arrange
    unsigned char destination[10];
    uint32_t serialized_size;

    ///act
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result = CONSTBUFFER_THANDLE_to_fixed_size_buffer(NULL, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_070: [ If destination is NULL then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_fixed_size_buffer_with_destination_NULL_fails)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create((const unsigned char*)"a", 1);
    ASSERT_IS_NOT_NULL(source);
    uint32_t serialized_size;

    ///act
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result = CONSTBUFFER_THANDLE_to_fixed_size_buffer(source, NULL, 10, &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG, result);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_071: [ If serialized_size is NULL then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_fixed_size_buffer_with_serialized_size_NULL_fails)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create((const unsigned char*)"a", 1);
    ASSERT_IS_NOT_NULL(source);
    unsigned char destination[10];

    ///act
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result = CONSTBUFFER_THANDLE_to_fixed_size_buffer(source, destination, sizeof(destination), NULL);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG, result);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_072: [ If the size of serialization exceeds destination_size then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail, write in serialized_size how much it would need and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_fixed_size_buffer_with_insufficient_buffer_fails)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create((const unsigned char*)"abc", 3);
    ASSERT_IS_NOT_NULL(source);
    unsigned char destination[5]; // Need 8 bytes (1 + 4 + 3) but only have 5
    uint32_t serialized_size;

    ///act
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result = CONSTBUFFER_THANDLE_to_fixed_size_buffer(source, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER, result);
    ASSERT_ARE_EQUAL(uint32_t, 8, serialized_size); // Should report how much is needed

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_073: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall write at offset 0 of destination the version of serialization (currently 1).]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_074: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall write at offset 1 of destination the value of source's size in network byte order.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_075: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall copy all the bytes of source's buffer in destination starting at offset 5.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_076: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall succeed, write in serialized_size how much it used and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_fixed_size_buffer_with_size_1_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create((const unsigned char*)"a", 1);
    ASSERT_IS_NOT_NULL(source);
    unsigned char destination[10];
    uint32_t serialized_size;

    ///act
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result = CONSTBUFFER_THANDLE_to_fixed_size_buffer(source, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, 6, serialized_size); // 1 + 4 + 1
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, destination[0]);
    
    uint32_t read_size;
    read_uint32_t(destination + CONSTBUFFER_SIZE_OFFSET, &read_size);
    ASSERT_ARE_EQUAL(uint32_t, 1, read_size);
    ASSERT_ARE_EQUAL(char, 'a', destination[CONSTBUFFER_CONTENT_OFFSET]);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_076: [ CONSTBUFFER_THANDLE_to_fixed_size_buffer shall succeed, write in serialized_size how much it used and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_fixed_size_buffer_with_0_size_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER) source = CONSTBUFFER_THANDLE_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(source);
    unsigned char destination[10];
    uint32_t serialized_size;

    ///act
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result = CONSTBUFFER_THANDLE_to_fixed_size_buffer(source, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, 5, serialized_size); // 1 + 4 + 0
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, destination[0]);
    
    uint32_t read_size;
    read_uint32_t(destination + CONSTBUFFER_SIZE_OFFSET, &read_size);
    ASSERT_ARE_EQUAL(uint32_t, 0, read_size);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&source, NULL);
}

/* CONSTBUFFER_THANDLE_from_buffer */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_078: [ If source is NULL then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_source_NULL_fails)
{
    ///arrange
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(NULL, 10, &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_079: [ If consumed is NULL then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_consumed_NULL_fails)
{
    ///arrange
    unsigned char buffer[10] = { 0 };
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, sizeof(buffer), NULL, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_080: [ If destination is NULL then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_destination_NULL_fails)
{
    ///arrange
    unsigned char buffer[10] = { 0 };
    uint32_t consumed;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, sizeof(buffer), &consumed, NULL);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_081: [ If size is 0 then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_size_0_fails)
{
    ///arrange
    unsigned char buffer[10] = { 0 };
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, 0, &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_ARG, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_082: [ If source byte at offset 0 is not 1 (current version) then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_invalid_version_fails)
{
    ///arrange
    unsigned char buffer[10] = { 2 }; // Invalid version
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, sizeof(buffer), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_083: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_too_small_buffer_fails)
{
    ///arrange
    unsigned char buffer[4] = { CONSTBUFFER_VERSION_V1 }; // Only 4 bytes, need at least 5
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, sizeof(buffer), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_085: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) + number of content bytes then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_insufficient_content_bytes_fails)
{
    ///arrange
    unsigned char buffer[6]; // Version + size(2) + only 1 content byte
    buffer[0] = CONSTBUFFER_VERSION_V1;
    write_uint32_t(buffer + CONSTBUFFER_SIZE_OFFSET, 2); // Claims 2 content bytes but only has 1
    buffer[5] = 'a';
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, sizeof(buffer), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_INVALID_DATA, result);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_084: [ CONSTBUFFER_THANDLE_from_buffer shall read the number of serialized content bytes from offset 1 of source.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_086: [ CONSTBUFFER_THANDLE_from_buffer shall create a THANDLE(CONSTBUFFER) from the bytes at offset 5 of source.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_087: [ CONSTBUFFER_THANDLE_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed THANDLE(CONSTBUFFER) and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_valid_buffer_succeeds)
{
    ///arrange
    unsigned char buffer[6]; // Version + size(1) + 1 content byte
    buffer[0] = CONSTBUFFER_VERSION_V1;
    write_uint32_t(buffer + CONSTBUFFER_SIZE_OFFSET, 1);
    buffer[5] = 'a';
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, sizeof(buffer), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, 6, consumed); // 1 + 4 + 1
    ASSERT_IS_NOT_NULL(destination);
    
    // Verify the content
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(destination);
    ASSERT_ARE_EQUAL(uint32_t, 1, content->size);
    ASSERT_ARE_EQUAL(char, 'a', content->buffer[0]);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&destination, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_087: [ CONSTBUFFER_THANDLE_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed THANDLE(CONSTBUFFER) and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_with_zero_content_succeeds)
{
    ///arrange
    unsigned char buffer[5]; // Version + size(0) + 0 content bytes
    buffer[0] = CONSTBUFFER_VERSION_V1;
    write_uint32_t(buffer + CONSTBUFFER_SIZE_OFFSET, 0);
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(buffer, sizeof(buffer), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, 5, consumed); // 1 + 4 + 0
    ASSERT_IS_NOT_NULL(destination);
    
    // Verify the content
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(destination);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&destination, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_068: [ If there are any failures then CONSTBUFFER_THANDLE_get_serialized_size shall fail and return CONSTBUFFER_THANDLE_GET_SERIALIZED_SIZE_RESULT_ERROR.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_get_serialized_size_fails_when_error_occurs)
{
    ///arrange
    // Test with NULL source which is a valid error scenario

    ///act
    uint32_t result = CONSTBUFFER_THANDLE_get_serialization_size(NULL);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, 0, result);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_077: [ If there are any failures then CONSTBUFFER_THANDLE_to_fixed_size_buffer shall fail and return CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_ERROR.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_to_fixed_size_buffer_fails_when_error_occurs)
{
    ///arrange
    unsigned char destination[10];
    uint32_t serialized_size;

    ///act
    CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT result = CONSTBUFFER_THANDLE_to_fixed_size_buffer(NULL, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_THANDLE_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG, result);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_088: [ If there are any failures then CONSTBUFFER_THANDLE_from_buffer shall fail and return CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_from_buffer_fails_when_error_occurs)
{
    ///arrange
    // Create a valid serialized buffer to deserialize from
    unsigned char source_buffer[] = { 
        CONSTBUFFER_VERSION_V1,                    // version = 1
        0x00, 0x00, 0x00, 0x02,                   // size = 2 (big endian)
        0x42, 0x43                                // content = [0x42, 0x43]
    };
    uint32_t consumed;
    THANDLE(CONSTBUFFER) destination = NULL;

    // Inject malloc failure in the CONSTBUFFER_THANDLE_Create call inside from_buffer
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, 1))
        .SetReturn(NULL);

    ///act
    CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT result = CONSTBUFFER_THANDLE_from_buffer(source_buffer, sizeof(source_buffer), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT, CONSTBUFFER_THANDLE_FROM_BUFFER_RESULT_ERROR, result);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&destination, NULL);
}

/* CONSTBUFFER_THANDLE_CreateWithCustomFree */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_026: [ If source is NULL and size is different than 0 then CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_with_NULL_source_and_non_zero_size_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(NULL, 1, test_free_func_no_free, (void*)0x4242);

    ///assert
    ASSERT_IS_NULL(handle);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_029: [ If customFreeFunc is NULL, CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_with_NULL_customFreeFunc_fails)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 42;
    test_buffer[1] = 43;

    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(test_buffer, 2, NULL, test_buffer);

    ///assert
    ASSERT_IS_NULL(handle);

    ///cleanup
    real_gballoc_hl_free(test_buffer);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_028: [ CONSTBUFFER_THANDLE_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_030: [ The non-NULL handle returned by CONSTBUFFER_THANDLE_CreateWithCustomFree shall have its ref count set to "1".]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_succeeds)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 42;
    test_buffer[1] = 43;

    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(test_buffer, 2, test_free_func_with_free, test_buffer);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "storage"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 2, content->size);
    ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer, "same buffer should be returned");

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_032: [ customFreeFuncContext shall be allowed to be NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_with_NULL_customFreeFuncContext_succeeds)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 42;
    test_buffer[1] = 43;

    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(test_buffer, 2, test_free_func_no_free, NULL);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "storage"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 2, content->size);
    ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer, "same buffer should be returned");

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
    real_gballoc_hl_free(test_buffer);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_027: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_028: [ CONSTBUFFER_THANDLE_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_with_0_size_succeeds)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 42;
    test_buffer[1] = 43;

    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(test_buffer, 0, test_free_func_with_free, test_buffer);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "storage"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_028: [ CONSTBUFFER_THANDLE_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_with_NULL_source_and_0_size_succeeds)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(NULL, 0, test_free_func_no_free, (void*)0x4242);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    /*testing the "storage"*/
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_IS_NULL(content->buffer);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_034: [ CONSTBUFFER_THANDLE_CreateWithCustomFree shall store customFreeFunc and customFreeFuncContext in order to use them to free the memory when the const buffer resources are freed.]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_031: [ If the buffer was created by calling CONSTBUFFER_THANDLE_CreateWithCustomFree, the customFreeFunc function shall be called to free the memory, while passed customFreeFuncContext as argument.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_calls_custom_free_func_on_dispose)
{
    ///arrange
    unsigned char* test_buffer = real_gballoc_hl_malloc(2);
    ASSERT_IS_NOT_NULL(test_buffer);
    test_buffer[0] = 42;
    test_buffer[1] = 43;
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(test_buffer, 2, test_free_func_no_free, (void*)0x4242);
    ASSERT_IS_NOT_NULL(handle);
    umock_c_reset_all_calls();

    ///act
    STRICT_EXPECTED_CALL(test_free_func_no_free((void*)0x4242));
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    real_gballoc_hl_free(test_buffer);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_033: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWithCustomFree shall fail and return NULL.]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_fails_when_malloc_fails)
{
    ///arrange
    unsigned char test_buffer[] = { 1, 2, 3 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_CreateWithCustomFree(test_buffer, sizeof(test_buffer), test_free_func_with_free, test_buffer);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/* CONSTBUFFER_THANDLE_CreateWritableHandle */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_089: [ If size is 0, then CONSTBUFFER_THANDLE_CreateWritableHandle shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWritableHandle_with_zero_size_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) handle = CONSTBUFFER_THANDLE_CreateWritableHandle(0);

    ///assert
    ASSERT_IS_NULL(handle);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_091: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWritableHandle shall fail and return NULL. ]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_091: [ If any error occurs, CONSTBUFFER_THANDLE_CreateWritableHandle shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWritableHandle_fails_when_malloc_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1))
        .SetReturn(NULL);

    ///act
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) result = CONSTBUFFER_THANDLE_CreateWritableHandle(BUFFER1_length);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_090: [ CONSTBUFFER_THANDLE_CreateWritableHandle shall allocate memory for the THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA). ]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_092: [ CONSTBUFFER_THANDLE_CreateWritableHandle shall set the ref count of the newly created THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) to 1. ]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_093: [ CONSTBUFFER_THANDLE_CreateWritableHandle shall succeed and return a non-NULL THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA). ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWritableHandle_succeeds)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) handle = CONSTBUFFER_THANDLE_CreateWritableHandle(BUFFER1_length);

    ///assert
    ASSERT_IS_NOT_NULL(handle);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(&handle, NULL);
}

/* CONSTBUFFER_THANDLE_GetWritableBuffer */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_094: [ If constbufferWritableHandle is NULL, then CONSTBUFFER_THANDLE_GetWritableBuffer shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetWritableBuffer_with_NULL_handle_fails)
{
    ///arrange

    ///act
    unsigned char* buffer = CONSTBUFFER_THANDLE_GetWritableBuffer(NULL);

    ///assert
    ASSERT_IS_NULL(buffer);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_095: [ CONSTBUFFER_THANDLE_GetWritableBuffer shall succeed and return a pointer to the non-CONST buffer of constbufferWritableHandle. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetWritableBuffer_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) handle = CONSTBUFFER_THANDLE_CreateWritableHandle(BUFFER1_length);
    ASSERT_IS_NOT_NULL(handle);

    ///act
    unsigned char* buffer = CONSTBUFFER_THANDLE_GetWritableBuffer(handle);

    ///assert
    ASSERT_IS_NOT_NULL(buffer);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(&handle, NULL);
}

/* CONSTBUFFER_THANDLE_SealWritableHandle */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_096: [ If constbufferWritableHandle is NULL then CONSTBUFFER_THANDLE_SealWritableHandle shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_SealWritableHandle_with_NULL_handle_fails)
{
    ///arrange

    ///act
    THANDLE(CONSTBUFFER) sealed = CONSTBUFFER_THANDLE_SealWritableHandle(NULL);

    ///assert
    ASSERT_IS_NULL(sealed);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_098: [ If there are any failures then CONSTBUFFER_THANDLE_SealWritableHandle shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_SealWritableHandle_fails_when_Create_fails)
{
    ///arrange
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) handle = CONSTBUFFER_THANDLE_CreateWritableHandle(BUFFER1_length);
    ASSERT_IS_NOT_NULL(handle);
    
    // Get buffer and fill it
    unsigned char* buffer = CONSTBUFFER_THANDLE_GetWritableBuffer(handle);
    ASSERT_IS_NOT_NULL(buffer);
    (void)memcpy(buffer, BUFFER1_u_char, BUFFER1_length);
    
    umock_c_reset_all_calls();
    
    // Mock failure in the internal Create call used by SealWritableHandle
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1))
        .SetReturn(NULL);

    ///act
    THANDLE(CONSTBUFFER) result = CONSTBUFFER_THANDLE_SealWritableHandle(handle);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(&handle, NULL);
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_097: [ CONSTBUFFER_THANDLE_SealWritableHandle shall create a new THANDLE(CONSTBUFFER) from the contents of constbufferWritableHandle. ]*/
/*Tests_SRS_CONSTBUFFER_THANDLE_88_099: [ CONSTBUFFER_THANDLE_SealWritableHandle shall succeed and return a non-NULL THANDLE(CONSTBUFFER). ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_SealWritableHandle_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) handle = CONSTBUFFER_THANDLE_CreateWritableHandle(BUFFER1_length);
    ASSERT_IS_NOT_NULL(handle);
    
    // Write some data to the writable buffer
    unsigned char* buffer = CONSTBUFFER_THANDLE_GetWritableBuffer(handle);
    ASSERT_IS_NOT_NULL(buffer);
    memcpy(buffer, BUFFER1_u_char, BUFFER1_length);

    ///act
    THANDLE(CONSTBUFFER) sealed = CONSTBUFFER_THANDLE_SealWritableHandle(handle);

    ///assert
    ASSERT_IS_NOT_NULL(sealed);
    
    // Verify the content matches
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(sealed);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, BUFFER1_length, content->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(content->buffer, BUFFER1_u_char, BUFFER1_length));

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(&handle, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&sealed, NULL);
}

/* CONSTBUFFER_THANDLE_GetWritableBufferSize */

/*Tests_SRS_CONSTBUFFER_THANDLE_88_100: [ If constbufferWritableHandle is NULL, then CONSTBUFFER_THANDLE_GetWritableBufferSize shall return 0. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetWritableBufferSize_with_NULL_handle_returns_0)
{
    ///arrange

    ///act
    uint32_t size = CONSTBUFFER_THANDLE_GetWritableBufferSize(NULL);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, 0, size);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_THANDLE_88_101: [ CONSTBUFFER_THANDLE_GetWritableBufferSize shall succeed and return the size of the writable buffer of constbufferWritableHandle. ]*/
TEST_FUNCTION(CONSTBUFFER_THANDLE_GetWritableBufferSize_succeeds)
{
    ///arrange
    THANDLE(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA) handle = CONSTBUFFER_THANDLE_CreateWritableHandle(BUFFER1_length);
    ASSERT_IS_NOT_NULL(handle);

    ///act
    uint32_t size = CONSTBUFFER_THANDLE_GetWritableBufferSize(handle);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, BUFFER1_length, size);

    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER_THANDLE_WRITABLE_HANDLE_DATA)(&handle, NULL);
}

END_TEST_SUITE(constbuffer_thandle_ut)
