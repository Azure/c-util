// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include <limits.h>

#include "umock_c/umocktypes_stdint.h"

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c.h"
#include "c_util/buffer_.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

MOCKABLE_FUNCTION(, void*, test_alloc, size_t, size, void*, context);

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

static void* my_gballoc_malloc_with_context(size_t size, void* context)
{
    (void)context;
    return real_gballoc_hl_malloc(size);
}

#include "c_util/memory_data.h"

#include "c_util/constbuffer_format.h"
#include "c_util/constbuffer_version.h"
#include "c_util/constbuffer.h"

static TEST_MUTEX_HANDLE g_testByTest;

static const char* buffer1 = "le buffer no 1";
static const char* buffer2 = NULL;
static const char* buffer3 = "three";

#define BUFFER1_HANDLE (BUFFER_HANDLE)1
#define BUFFER1_u_char ((unsigned char*)buffer1)
#define BUFFER1_length (uint32_t)strlen(buffer1)

#define BUFFER2_HANDLE (BUFFER_HANDLE)2
#define BUFFER2_u_char ((unsigned char*)buffer2)
#define BUFFER2_length ((uint32_t)0)

#define BUFFER3_HANDLE (BUFFER_HANDLE)3
#define BUFFER3_u_char ((unsigned char*)buffer3)
#define BUFFER3_length ((uint32_t)0)

unsigned char* my_BUFFER_u_char(BUFFER_HANDLE handle)
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

MOCK_FUNCTION_WITH_CODE(, void, test_free_func, void*, context)
MOCK_FUNCTION_END()

TEST_DEFINE_ENUM_TYPE(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_VALUES)

TEST_DEFINE_ENUM_TYPE(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_VALUES)
IMPLEMENT_UMOCK_C_ENUM_TYPE(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_VALUES)

#define TEST_ALLOC_CONTEXT (void*)0x9874387 /*random typing*/
static void* test_alloc_impl(size_t size, void* context)
{
    ASSERT_ARE_EQUAL(void_ptr, TEST_ALLOC_CONTEXT, context);
    return real_gballoc_hl_malloc(size);
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

    TEST_SUITE_INITIALIZE(setsBufferTempSize)
    {
        ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

        ASSERT_IS_NOT_NULL(g_testByTest = TEST_MUTEX_CREATE());

        ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
        ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

        REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);

        REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_u_char, my_BUFFER_u_char);
        REGISTER_GLOBAL_MOCK_HOOK(BUFFER_length, my_BUFFER_length);

        REGISTER_TYPE(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT);
        REGISTER_TYPE(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT);
        REGISTER_GLOBAL_MOCK_HOOK(test_alloc, test_alloc_impl);
}

    TEST_SUITE_CLEANUP(TestClassCleanup)
    {
        umock_c_deinit();

        TEST_MUTEX_DESTROY(g_testByTest);

        real_gballoc_hl_deinit();
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

    /* CONSTBUFFER_Create */

    /*Tests_SRS_CONSTBUFFER_02_001: [If source is NULL and size is different than 0 then CONSTBUFFER_Create shall fail and return NULL.]*/
    TEST_FUNCTION(CONSTBUFFER_Create_with_invalid_args_fails)
    {
        ///arrange

        ///act
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_Create(NULL, 1);

        ///assert
        ASSERT_IS_NULL(handle);

        ///cleanup
    }

    /*Tests_SRS_CONSTBUFFER_02_002: [Otherwise, CONSTBUFFER_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
    /*Tests_SRS_CONSTBUFFER_02_004: [Otherwise CONSTBUFFER_Create shall return a non-NULL handle.]*/
    TEST_FUNCTION(CONSTBUFFER_Create_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;

        ///act
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1));

        handle = CONSTBUFFER_Create(BUFFER1_u_char, BUFFER1_length);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "copy"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, BUFFER1_length, content->size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER1_u_char, content->buffer, BUFFER1_length));
        /*testing that it is a copy and not a pointer assignment*/
        ASSERT_ARE_NOT_EQUAL(void_ptr, BUFFER1_u_char, content->buffer);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* CONSTBUFFER_CreateFromBuffer */

    /*Tests_SRS_CONSTBUFFER_02_009: [Otherwise, CONSTBUFFER_CreateFromBuffer shall return a non-NULL handle.]*/
    /*Tests_SRS_CONSTBUFFER_02_007: [Otherwise, CONSTBUFFER_CreateFromBuffer shall copy the content of buffer.]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromBuffer_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;

        ///act
        STRICT_EXPECTED_CALL(BUFFER_length(BUFFER1_HANDLE));
        STRICT_EXPECTED_CALL(BUFFER_u_char(BUFFER1_HANDLE));
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1));

        handle = CONSTBUFFER_CreateFromBuffer(BUFFER1_HANDLE);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "copy"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, BUFFER1_length, content->size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER1_u_char, content->buffer, BUFFER1_length));
        /*testing that it is a copy and not a pointer assignment*/
        ASSERT_ARE_NOT_EQUAL(void_ptr, BUFFER1_u_char, content->buffer);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_008: [If copying the content fails, then CONSTBUFFER_CreateFromBuffer shall fail and return NULL.]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromBuffer_fails_when_malloc_fails)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;

        ///act
        STRICT_EXPECTED_CALL(BUFFER_length(BUFFER1_HANDLE));
        STRICT_EXPECTED_CALL(BUFFER_u_char(BUFFER1_HANDLE));

        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1))
            .SetReturn(NULL);


        handle = CONSTBUFFER_CreateFromBuffer(BUFFER1_HANDLE);

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_006: [If buffer is NULL then CONSTBUFFER_CreateFromBuffer shall fail and return NULL.]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromBuffer_with_NULL_fails)
    {
        ///arrange

        ///act
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_CreateFromBuffer(NULL);

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_003: [If creating the copy fails then CONSTBUFFER_Create shall return NULL.]*/
    TEST_FUNCTION(CONSTBUFFER_Create_fails_when_malloc_fails)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;

        ///act
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1))
            .SetReturn(NULL);

        handle = CONSTBUFFER_Create(BUFFER1_u_char, BUFFER1_length);

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_CONSTBUFFER_02_005: [The non-NULL handle returned by CONSTBUFFER_Create shall have its ref count set to "1".]*/
    /*Tests_SRS_CONSTBUFFER_02_017: [If the refcount reaches zero, then CONSTBUFFER_DecRef shall deallocate all resources used by the CONSTBUFFER_HANDLE.]*/
    TEST_FUNCTION(CONSTBUFFER_Create_is_ref_counted_1)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_Create(BUFFER1_u_char, BUFFER1_length);
        umock_c_reset_all_calls();
        ///act

        STRICT_EXPECTED_CALL(free(IGNORED_ARG));

        CONSTBUFFER_DecRef(handle);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_CONSTBUFFER_02_002: [Otherwise, CONSTBUFFER_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
    TEST_FUNCTION(CONSTBUFFER_Create_from_0_size_succeeds_1)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;

        ///act
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER2_length, 1));

        handle = CONSTBUFFER_Create(BUFFER2_u_char, BUFFER2_length);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "copy"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, BUFFER2_length, content->size);
        /*testing that it is a copy and not a pointer assignment*/
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_002: [Otherwise, CONSTBUFFER_Create shall create a copy of the memory area pointed to by source having size bytes.]*/
    /*Tests_SRS_CONSTBUFFER_02_009: [Otherwise, CONSTBUFFER_CreateFromBuffer shall return a non-NULL handle.]*/
    TEST_FUNCTION(CONSTBUFFER_Create_from_0_size_succeeds_2)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;

        ///act
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER3_length, 1));

        handle = CONSTBUFFER_Create(BUFFER3_u_char, BUFFER3_length);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "copy"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, BUFFER3_length, content->size);
        /*testing that it is a copy and not a pointer assignment*/
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* CONSTBUFFER_CreateWithMoveMemory */

    /* Tests_SRS_CONSTBUFFER_01_001: [ If source is NULL and size is different than 0 then CONSTBUFFER_CreateWithMoveMemory shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithMoveMemory_with_invalid_args_fails)
    {
        ///arrange

        ///act
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_CreateWithMoveMemory(NULL, 1);

        ///assert
        ASSERT_IS_NULL(handle);

        ///cleanup
    }

    /* Tests_SRS_CONSTBUFFER_01_002: [ CONSTBUFFER_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithMoveMemory_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;
        unsigned char* test_buffer = (unsigned char* )real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        handle = CONSTBUFFER_CreateWithMoveMemory(test_buffer, 2);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "storage"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, 2, content->size);
        ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer, "same buffer should be returned");
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* Tests_SRS_CONSTBUFFER_01_002: [ CONSTBUFFER_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
    /* Tests_SRS_CONSTBUFFER_01_004: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithMoveMemory_with_0_size_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;
        unsigned char* test_buffer = (unsigned char* )real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        handle = CONSTBUFFER_CreateWithMoveMemory(test_buffer, 0);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "storage"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, 0, content->size);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* Tests_SRS_CONSTBUFFER_01_002: [ CONSTBUFFER_CreateWithMoveMemory shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithMoveMemory_with_NULL_source_and_0_size_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        handle = CONSTBUFFER_CreateWithMoveMemory(NULL, 0);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "storage"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, 0, content->size);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* Tests_SRS_CONSTBUFFER_01_005: [ If any error occurs, CONSTBUFFER_CreateWithMoveMemory shall fail and return NULL. ]*/
    TEST_FUNCTION(when_malloc_fails_CONSTBUFFER_CreateWithMoveMemory_fails)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        unsigned char* test_buffer = (unsigned char* )real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
            .SetReturn(NULL);

        ///act
        handle = CONSTBUFFER_CreateWithMoveMemory(test_buffer, 2);

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        free(test_buffer);
    }

    /* CONSTBUFFER_CreateWithCustomFree */

    /* Tests_SRS_CONSTBUFFER_01_006: [ If source is NULL and size is different than 0 then CONSTBUFFER_CreateWithCustomFree shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_with_invalid_args_fails)
    {
        ///arrange

        ///act
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_CreateWithCustomFree(NULL, 1, test_free_func, (void*)0x4242);

        ///assert
        ASSERT_IS_NULL(handle);
    }

    /* Tests_SRS_CONSTBUFFER_01_013: [ If customFreeFunc is NULL, CONSTBUFFER_CreateWithCustomFree shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_with_NULL_customFreeFunc_fails)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        unsigned char* test_buffer = (unsigned char*)real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        ///act
        handle = CONSTBUFFER_CreateWithCustomFree(test_buffer, 2, NULL, test_buffer);

        ///assert
        ASSERT_IS_NULL(handle);

        /// cleanup
        real_gballoc_hl_free(test_buffer);
    }

    /* Tests_SRS_CONSTBUFFER_01_008: [ CONSTBUFFER_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;
        unsigned char* test_buffer = (unsigned char*)real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        handle = CONSTBUFFER_CreateWithCustomFree(test_buffer, 2, free, test_buffer);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "storage"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, 2, content->size);
        ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer, "same buffer should be returned");
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* Tests_SRS_CONSTBUFFER_01_014: [ customFreeFuncContext shall be allowed to be NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_succeedswith_NULL_free_function_context)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;
        unsigned char* test_buffer = (unsigned char*)real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        handle = CONSTBUFFER_CreateWithCustomFree(test_buffer, 2, test_free_func, NULL);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "storage"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, 2, content->size);
        ASSERT_ARE_EQUAL(void_ptr, test_buffer, content->buffer, "same buffer should be returned");
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
        real_gballoc_hl_free(test_buffer);
    }

    /* Tests_SRS_CONSTBUFFER_01_008: [ CONSTBUFFER_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
    /* Tests_SRS_CONSTBUFFER_01_007: [ If source is non-NULL and size is 0, the source pointer shall be owned (and freed) by the newly created instance of const buffer. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_with_0_size_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;
        unsigned char* test_buffer = (unsigned char*)real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        handle = CONSTBUFFER_CreateWithCustomFree(test_buffer, 0, free, test_buffer);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "storage"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, 0, content->size);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* Tests_SRS_CONSTBUFFER_01_008: [ CONSTBUFFER_CreateWithCustomFree shall store the source and size and return a non-NULL handle to the newly created const buffer. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_with_NULL_source_and_0_size_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        handle = CONSTBUFFER_CreateWithCustomFree(NULL, 0, free, NULL);

        ///assert
        ASSERT_IS_NOT_NULL(handle);
        /*testing the "storage"*/
        content = CONSTBUFFER_GetContent(handle);
        ASSERT_ARE_EQUAL(size_t, 0, content->size);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /* Tests_SRS_CONSTBUFFER_01_011: [ If any error occurs, CONSTBUFFER_CreateWithMoveMemory shall fail and return NULL. ]*/
    TEST_FUNCTION(when_malloc_fails_CONSTBUFFER_CreateWithCustomFree_fails)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        unsigned char* test_buffer = (unsigned char*)real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
            .SetReturn(NULL);

        ///act
        handle = CONSTBUFFER_CreateWithCustomFree(test_buffer, 2, free, test_buffer);

        ///assert
        ASSERT_IS_NULL(handle);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        free(test_buffer);
    }

    /* CONSTBUFFER_GetContent */

    /*Tests_SRS_CONSTBUFFER_02_011: [If constbufferHandle is NULL then CONSTBUFFER_GetContent shall return NULL.]*/
    TEST_FUNCTION(CONSTBUFFER_GetContent_with_NULL_returns_NULL)
    {
        ///arrange

        ///act
        const CONSTBUFFER* content = CONSTBUFFER_GetContent(NULL);

        ///assert
        ASSERT_IS_NULL(content);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_CONSTBUFFER_02_012: [Otherwise, CONSTBUFFER_GetContent shall return a const CONSTBUFFER* that matches byte by byte the original bytes used to created the const buffer and has the same length.]*/
    TEST_FUNCTION(CONSTBUFFER_GetContent_succeeds_1)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;

        handle = CONSTBUFFER_Create(BUFFER1_u_char, BUFFER1_length);
        umock_c_reset_all_calls();

        ///act
        content = CONSTBUFFER_GetContent(handle);

        ///assert
        ASSERT_IS_NOT_NULL(content);
        /*testing the "copy"*/
        ASSERT_ARE_EQUAL(uint32_t, BUFFER1_length, content->size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(BUFFER1_u_char, content->buffer, BUFFER1_length));
        /*testing that it is a copy and not a pointer assignment*/
        ASSERT_ARE_NOT_EQUAL(void_ptr, BUFFER1_u_char, content->buffer);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_012: [Otherwise, CONSTBUFFER_GetContent shall return a const CONSTBUFFER* that matches byte by byte the original bytes used to created the const buffer and has the same length.]*/
    TEST_FUNCTION(CONSTBUFFER_GetContent_succeeds_2)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        const CONSTBUFFER* content;
        handle = CONSTBUFFER_Create(NULL, 0);
        umock_c_reset_all_calls();

        ///act
        content = CONSTBUFFER_GetContent(handle);

        ///assert
        ASSERT_IS_NOT_NULL(content);
        /*testing the "copy"*/
        ASSERT_ARE_EQUAL(size_t, 0, content->size);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_025: [ If handle is NULL then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_with_handle_NULL_fails)
    {
        ///arrange

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(NULL, 0, 0);

        ///assert
        ASSERT_IS_NULL(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_033: [ If offset is greater than handles's size then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    /*Tests_SRS_CONSTBUFFER_02_027: [ If offset + size exceed handles's size then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_with_offset_greater_than_handle_s_size_fails)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        ///act
        CONSTBUFFER_HANDLE result;
        result = CONSTBUFFER_CreateFromOffsetAndSize(origin, sizeof(source) + 1, 0);
        ASSERT_IS_NULL(result);
        result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 0, sizeof(source) + 1);
        ASSERT_IS_NULL(result);
        result = CONSTBUFFER_CreateFromOffsetAndSize(origin, sizeof(source), 1);
        ASSERT_IS_NULL(result);
        result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 1, sizeof(source));
        ASSERT_IS_NULL(result);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
    }

    /*Tests_SRS_CONSTBUFFER_02_032: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_with_offset_plus_size_equal_to_UINT_MAX_fail)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, (uint32_t)(sizeof(source) - 1), (uint32_t)(UINT_MAX- sizeof(source) + 2));

        ///assert
        ASSERT_IS_NULL(result);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
    }

    /*Tests_SRS_CONSTBUFFER_02_027: [ If offset + size exceed handles's size then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_with_offset_plus_size_exceed_handle_size_fails_2)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, (uint32_t)(sizeof(source)-1), 2);

        ///assert
        ASSERT_IS_NULL(result);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
    }

    /*Tests_SRS_CONSTBUFFER_02_028: [ CONSTBUFFER_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER_HANDLE's content. ]*/
    /*Tests_SRS_CONSTBUFFER_02_029: [ CONSTBUFFER_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER_HANDLE to the initial value. ]*/
    /*Tests_SRS_CONSTBUFFER_02_030: [ CONSTBUFFER_CreateFromOffsetAndSize shall increment the reference count of handle. ]*/
    /*Tests_SRS_CONSTBUFFER_02_031: [ CONSTBUFFER_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_succeeds_1)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 0, (uint32_t)(sizeof(source)));

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        const CONSTBUFFER* content = CONSTBUFFER_GetContent(result);
        ASSERT_ARE_EQUAL(size_t, sizeof(source), content->size);
        ASSERT_IS_TRUE(memcmp(content->buffer, source, sizeof(source)) == 0);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
        CONSTBUFFER_DecRef(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_028: [ CONSTBUFFER_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER_HANDLE's content. ]*/
    /*Tests_SRS_CONSTBUFFER_02_029: [ CONSTBUFFER_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER_HANDLE to the initial value. ]*/
    /*Tests_SRS_CONSTBUFFER_02_030: [ CONSTBUFFER_CreateFromOffsetAndSize shall increment the reference count of handle. ]*/
    /*Tests_SRS_CONSTBUFFER_02_031: [ CONSTBUFFER_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_succeeds_2)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 0, 0);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        const CONSTBUFFER* content = CONSTBUFFER_GetContent(result);
        ASSERT_ARE_EQUAL(uint32_t, 0, content->size);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
        CONSTBUFFER_DecRef(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_028: [ CONSTBUFFER_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER_HANDLE's content. ]*/
    /*Tests_SRS_CONSTBUFFER_02_029: [ CONSTBUFFER_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER_HANDLE to the initial value. ]*/
    /*Tests_SRS_CONSTBUFFER_02_030: [ CONSTBUFFER_CreateFromOffsetAndSize shall increment the reference count of handle. ]*/
    /*Tests_SRS_CONSTBUFFER_02_031: [ CONSTBUFFER_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_succeeds_3)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, (uint32_t)(sizeof(source) - 1), 1);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        const CONSTBUFFER* content = CONSTBUFFER_GetContent(result);
        ASSERT_ARE_EQUAL(uint32_t, 1, content->size);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
        CONSTBUFFER_DecRef(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_028: [ CONSTBUFFER_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER_HANDLE's content. ]*/
    /*Tests_SRS_CONSTBUFFER_02_029: [ CONSTBUFFER_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER_HANDLE to the initial value. ]*/
    /*Tests_SRS_CONSTBUFFER_02_030: [ CONSTBUFFER_CreateFromOffsetAndSize shall increment the reference count of handle. ]*/
    /*Tests_SRS_CONSTBUFFER_02_031: [ CONSTBUFFER_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_succeeds_4)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 1, (uint32_t)(sizeof(source) - 1));

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        const CONSTBUFFER* content = CONSTBUFFER_GetContent(result);
        ASSERT_ARE_EQUAL(size_t, sizeof(source)-1, content->size);
        ASSERT_IS_TRUE(memcmp(content->buffer, source+1, sizeof(source)-1) == 0);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
        CONSTBUFFER_DecRef(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_028: [ CONSTBUFFER_CreateFromOffsetAndSize shall allocate memory for a new CONSTBUFFER_HANDLE's content. ]*/
    /*Tests_SRS_CONSTBUFFER_02_029: [ CONSTBUFFER_CreateFromOffsetAndSize shall set the ref count of the newly created CONSTBUFFER_HANDLE to the initial value. ]*/
    /*Tests_SRS_CONSTBUFFER_02_030: [ CONSTBUFFER_CreateFromOffsetAndSize shall increment the reference count of handle. ]*/
    /*Tests_SRS_CONSTBUFFER_02_031: [ CONSTBUFFER_CreateFromOffsetAndSize shall succeed and return a non-NULL value. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_succeeds_5)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, (uint32_t)(sizeof(source)), 0);

        ///assert
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        const CONSTBUFFER* content = CONSTBUFFER_GetContent(result);
        ASSERT_ARE_EQUAL(uint32_t, 0, content->size);

        ///cleanup
        CONSTBUFFER_DecRef(origin);
        CONSTBUFFER_DecRef(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_032: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSize_when_malloc_fails_it_fails)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
            .SetReturn(NULL);

        ///act
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 1, (uint32_t)(sizeof(source) - 1));

        ///assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(origin);
    }

    /*Tests_SRS_CONSTBUFFER_02_024: [ If the constbufferHandle was created by calling CONSTBUFFER_CreateFromOffsetAndSize then CONSTBUFFER_DecRef shall decrement the ref count of the original handle passed to CONSTBUFFER_CreateFromOffsetAndSize. ]*/
    /*Tests_SRS_CONSTBUFFER_02_032: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_DecRef_for_CONSTBUFFER_CreateFromOffsetAndSize_succeeds)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 1, (uint32_t)(sizeof(source) - 1));
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(free(result));

        ///act
        CONSTBUFFER_DecRef(result);

        ///assert - origin should pretty much be still accesible
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        const CONSTBUFFER* content = CONSTBUFFER_GetContent(origin);
        ASSERT_ARE_EQUAL(size_t, sizeof(source), content->size);
        ASSERT_IS_TRUE(memcmp(content->buffer, source, sizeof(source)) == 0);
        
        ///cleanup
        CONSTBUFFER_DecRef(origin);
    }

    /*Tests_SRS_CONSTBUFFER_02_024: [ If the constbufferHandle was created by calling CONSTBUFFER_CreateFromOffsetAndSize then CONSTBUFFER_DecRef shall decrement the ref count of the original handle passed to CONSTBUFFER_CreateFromOffsetAndSize. ]*/
    /*Tests_SRS_CONSTBUFFER_02_032: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSize shall fail and return NULL. ]*/
    TEST_FUNCTION(CONSTBUFFER_DecRef_for_CONSTBUFFER_CreateFromOffsetAndSize_succeeds_2)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSize(origin, 1, (uint32_t)(sizeof(source) - 1));
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        /*note: no expected calls - after the below call both buffers are at ref count == 1*/

        ///act
        CONSTBUFFER_DecRef(origin);

        ///assert - origin should pretty much be still accesible
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        const CONSTBUFFER * content = CONSTBUFFER_GetContent(origin);
        ASSERT_ARE_EQUAL(size_t, sizeof(source), content->size);
        ASSERT_IS_TRUE(memcmp(content->buffer, source, sizeof(source)) == 0);

        const CONSTBUFFER* contentResult = CONSTBUFFER_GetContent(result);
        ASSERT_ARE_EQUAL(size_t, sizeof(source)-1, contentResult->size);
        ASSERT_IS_TRUE(memcmp(contentResult->buffer, source+ 1, sizeof(source)-1) == 0);

        ///cleanup
        CONSTBUFFER_DecRef(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_024: [ If the constbufferHandle was created by calling CONSTBUFFER_CreateFromOffsetAndSize then CONSTBUFFER_DecRef shall decrement the ref count of the original handle passed to CONSTBUFFER_CreateFromOffsetAndSize. ]*/
    TEST_FUNCTION(CONSTBUFFER_DecRef_for_CONSTBUFFER_CreateFromOffsetAndSize_succeeds_3)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE result1 = CONSTBUFFER_CreateFromOffsetAndSize(origin, 0, 2);
        ASSERT_IS_NOT_NULL(result1);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE result2 = CONSTBUFFER_CreateFromOffsetAndSize(result1, 1, 1);
        ASSERT_IS_NOT_NULL(result2);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        CONSTBUFFER_DecRef(origin);
        umock_c_reset_all_calls();

        /*note: no expected calls - after the below call all buffers are at ref count == 1*/

        ///act
        CONSTBUFFER_DecRef(result1); /*at this time result 2 has a ref to result1, which has a ref to origin. Nothing is freed*/

        ///assert 
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(result2); /*triggers the release of result1, which triggers the release of origin*/
    }

    /*Tests_SRS_CONSTBUFFER_02_024: [ If the constbufferHandle was created by calling CONSTBUFFER_CreateFromOffsetAndSize then CONSTBUFFER_DecRef shall decrement the ref count of the original handle passed to CONSTBUFFER_CreateFromOffsetAndSize. ]*/
    TEST_FUNCTION(CONSTBUFFER_DecRef_for_CONSTBUFFER_CreateFromOffsetAndSize_succeeds_4)
    {
        ///arrange
        CONSTBUFFER_HANDLE origin;
        const char source[] = "source";
        STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
        origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
        ASSERT_IS_NOT_NULL(origin);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE result1 = CONSTBUFFER_CreateFromOffsetAndSize(origin, 0, 2);
        ASSERT_IS_NOT_NULL(result1);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE result2 = CONSTBUFFER_CreateFromOffsetAndSize(result1, 1, 1);
        ASSERT_IS_NOT_NULL(result2);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();

        CONSTBUFFER_DecRef(origin);
        CONSTBUFFER_DecRef(result1); /*at this time result 2 has a ref to result1, which has a ref to origin. Nothing is freed*/
        umock_c_reset_all_calls();

        /*note: no expected calls - after the below call all buffers are at ref count == 1*/

        STRICT_EXPECTED_CALL(free(origin));
        STRICT_EXPECTED_CALL(free(result1));
        STRICT_EXPECTED_CALL(free(result2));

        ///act
        CONSTBUFFER_DecRef(result2); /*triggers the release of result1, which triggers the release of origin*/

        ///assert 
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup - nothing left
        
    }

    /* CONSTBUFFER_IncRef */

    /*Tests_SRS_CONSTBUFFER_02_013: [If constbufferHandle is NULL then CONSTBUFFER_IncRef shall return.]*/
    TEST_FUNCTION(CONSTBUFFER_IncRef_with_NULL_returns_NULL)
    {
        ///arrange

        ///act
        CONSTBUFFER_IncRef(NULL);

        ///assert
    }

    /*Tests_SRS_CONSTBUFFER_02_014: [Otherwise, CONSTBUFFER_IncRef shall increment the reference count.]*/
    TEST_FUNCTION(CONSTBUFFER_IncRef_increments_ref_count_1)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_Create(BUFFER1_u_char, BUFFER1_length);
        umock_c_reset_all_calls();

        ///act
        CONSTBUFFER_IncRef(handle);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_014: [Otherwise, CONSTBUFFER_IncRef shall increment the reference count and return constbufferHandle.]*/
    /*Tests_SRS_CONSTBUFFER_02_016: [Otherwise, CONSTBUFFER_DecRef shall decrement the refcount on the constbufferHandle handle.]*/
    /*Tests_SRS_CONSTBUFFER_02_017: [If the refcount reaches zero, then CONSTBUFFER_DecRef shall deallocate all resources used by the CONSTBUFFER_HANDLE.]*/
    TEST_FUNCTION(CONSTBUFFER_IncRef_increments_ref_count_2)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_Create(BUFFER1_u_char, BUFFER1_length);
        CONSTBUFFER_IncRef(handle);
        umock_c_reset_all_calls();

        ///act
        CONSTBUFFER_DecRef(handle); /*only a dec_Ref is expected here, so no effects*/

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
        CONSTBUFFER_DecRef(handle);
    }

    /*Tests_SRS_CONSTBUFFER_02_014: [Otherwise, CONSTBUFFER_IncRef shall increment the reference count and return constbufferHandle.]*/
    /*Tests_SRS_CONSTBUFFER_02_016: [Otherwise, CONSTBUFFER_DecRef shall decrement the refcount on the constbufferHandle handle.]*/
    /*Tests_SRS_CONSTBUFFER_02_017: [If the refcount reaches zero, then CONSTBUFFER_DecRef shall deallocate all resources used by the CONSTBUFFER_HANDLE.]*/
    TEST_FUNCTION(CONSTBUFFER_IncRef_increments_ref_count_3)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_Create(BUFFER1_u_char, BUFFER1_length);
        CONSTBUFFER_IncRef(handle);
        CONSTBUFFER_DecRef(handle); /*only a dec_Ref is expected here, so no effects*/
        umock_c_reset_all_calls();

        ///act
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        CONSTBUFFER_DecRef(handle);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /* CONSTBUFFER_DecRef */

    /*Tests_SRS_CONSTBUFFER_02_015: [If constbufferHandle is NULL then CONSTBUFFER_DecRef shall do nothing.]*/
    TEST_FUNCTION(CONSTBUFFER_DecRef_with_NULL_argument_does_nothing)
    {
        ///arrange

        ///act
        CONSTBUFFER_DecRef(NULL);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /*Tests_SRS_CONSTBUFFER_02_010: [The non-NULL handle returned by CONSTBUFFER_CreateFromBuffer shall have its ref count set to "1".]*/
    /*Tests_SRS_CONSTBUFFER_02_005: [The non-NULL handle returned by CONSTBUFFER_Create shall have its ref count set to "1".]*/
    /*Tests_SRS_CONSTBUFFER_02_017: [If the refcount reaches zero, then CONSTBUFFER_DecRef shall deallocate all resources used by the CONSTBUFFER_HANDLE.]*/
    TEST_FUNCTION(CONSTBUFFER_CreateFromBuffer_is_ref_counted_1)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_CreateFromBuffer(BUFFER1_HANDLE);
        umock_c_reset_all_calls();
        ///act

        STRICT_EXPECTED_CALL(free(IGNORED_ARG));

        CONSTBUFFER_DecRef(handle);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///cleanup
    }

    /* Tests_SRS_CONSTBUFFER_01_010: [ The non-NULL handle returned by CONSTBUFFER_CreateWithCustomFree shall have its ref count set to 1. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_is_ref_counted_1)
    {
        ///arrange
        unsigned char* test_buffer = (unsigned char*)real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_CreateWithCustomFree(test_buffer, 2, free, test_buffer);
        umock_c_reset_all_calls();
        ///act

        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));

        CONSTBUFFER_DecRef(handle);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /* Tests_SRS_CONSTBUFFER_01_009: [ CONSTBUFFER_CreateWithCustomFree shall store customFreeFunc and customFreeFuncContext in order to use them to free the memory when the CONST buffer resources are freed. ]*/
    /* Tests_SRS_CONSTBUFFER_01_012: [ If the buffer was created by calling CONSTBUFFER_CreateWithCustomFree, the customFreeFunc function shall be called to free the memory, while passed customFreeFuncContext as argument. ]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithCustomFree_with_custom_free_function_calls_the_custom_free_func)
    {
        ///arrange
        unsigned char* test_buffer = (unsigned char*)real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;
        CONSTBUFFER_HANDLE handle = CONSTBUFFER_CreateWithCustomFree(test_buffer, 2, test_free_func, (void*)0x4242);
        umock_c_reset_all_calls();
        ///act

        STRICT_EXPECTED_CALL(test_free_func((void*)0x4242));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));

        CONSTBUFFER_DecRef(handle);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup
        real_gballoc_hl_free(test_buffer);
    }

    /* Tests_SRS_CONSTBUFFER_01_003: [ The non-NULL handle returned by CONSTBUFFER_CreateWithMoveMemory shall have its ref count set to "1". ]*/
    /*Tests_SRS_CONSTBUFFER_02_017: [If the refcount reaches zero, then CONSTBUFFER_DecRef shall deallocate all resources used by the CONSTBUFFER_HANDLE.]*/
    TEST_FUNCTION(CONSTBUFFER_CreateWithMoveMemory_is_ref_counted_1)
    {
        ///arrange
        CONSTBUFFER_HANDLE handle;
        unsigned char* test_buffer = (unsigned char* )real_gballoc_hl_malloc(2);
        test_buffer[0] = 42;
        test_buffer[1] = 43;
        handle = CONSTBUFFER_CreateWithMoveMemory(test_buffer, 2);
        umock_c_reset_all_calls();

        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));

        ///act
        CONSTBUFFER_DecRef(handle);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    /*Tests_SRS_CONSTBUFFER_02_018: [ If left is NULL and right is NULL then CONSTBUFFER_HANDLE_contain_same shall return true. ]*/
    TEST_FUNCTION(CONSTBUFFER_HANDLE_contain_same_with_left_NULL_and_right_NULL_returns_true)
    {
        ///arrange
        bool result;

        ///act
        result = CONSTBUFFER_HANDLE_contain_same(NULL, NULL);

        ///assert
        ASSERT_IS_TRUE(result);
    }

    /*Tests_SRS_CONSTBUFFER_02_019: [ If left is NULL and right is not NULL then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
    TEST_FUNCTION(CONSTBUFFER_HANDLE_contain_same_with_left_NULL_and_right_not_NULL_returns_false)
    {
        ///arrange
        bool result;
        unsigned char rightSource = 'r';
        CONSTBUFFER_HANDLE right = CONSTBUFFER_Create(&rightSource, sizeof(rightSource));
        ASSERT_IS_NOT_NULL(right);

        ///act
        result = CONSTBUFFER_HANDLE_contain_same(NULL, right);

        ///assert
        ASSERT_IS_FALSE(result);

        ///clean
        CONSTBUFFER_DecRef(right);
    }

    /*Tests_SRS_CONSTBUFFER_02_020: [ If left is not NULL and right is NULL then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
    TEST_FUNCTION(CONSTBUFFER_HANDLE_contain_same_with_left_not_NULL_and_right_NULL_returns_false)
    {
        ///arrange
        bool result;
        unsigned char leftSource = 'l';
        CONSTBUFFER_HANDLE left = CONSTBUFFER_Create(&leftSource, sizeof(leftSource));
        ASSERT_IS_NOT_NULL(left);

        ///act
        result = CONSTBUFFER_HANDLE_contain_same(left, NULL);

        ///assert
        ASSERT_IS_FALSE(result);

        ///clean
        CONSTBUFFER_DecRef(left);
    }

    /*Tests_SRS_CONSTBUFFER_02_021: [ If left's size is different than right's size then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
    TEST_FUNCTION(CONSTBUFFER_HANDLE_contain_same_with_left_and_right_sizes_not_equal_returns_false)
    {
        ///arrange
        bool result;
        unsigned char leftSource = 'l';
        CONSTBUFFER_HANDLE left = CONSTBUFFER_Create(&leftSource, sizeof(leftSource));
        ASSERT_IS_NOT_NULL(left);

        unsigned char rightSource[2] = { 'r', 'r' };
        CONSTBUFFER_HANDLE right = CONSTBUFFER_Create(rightSource, sizeof(rightSource));
        ASSERT_IS_NOT_NULL(right);

        ///act
        result = CONSTBUFFER_HANDLE_contain_same(left, right);

        ///assert
        ASSERT_IS_FALSE(result);

        ///clean
        CONSTBUFFER_DecRef(left);
        CONSTBUFFER_DecRef(right);
    }

    /*Tests_SRS_CONSTBUFFER_02_022: [ If left's buffer is contains different bytes than rights's buffer then CONSTBUFFER_HANDLE_contain_same shall return false. ]*/
    TEST_FUNCTION(CONSTBUFFER_HANDLE_contain_same_with_left_and_right_content_not_equal_returns_false)
    {
        ///arrange
        bool result;
        unsigned char leftSource[2] = { 'l', 'l' };
        CONSTBUFFER_HANDLE left = CONSTBUFFER_Create(leftSource, sizeof(leftSource));
        ASSERT_IS_NOT_NULL(left);

        unsigned char rightSource[2] = { 'r', 'r' };
        CONSTBUFFER_HANDLE right = CONSTBUFFER_Create(rightSource, sizeof(rightSource));
        ASSERT_IS_NOT_NULL(right);

        ///act
        result = CONSTBUFFER_HANDLE_contain_same(left, right);

        ///assert
        ASSERT_IS_FALSE(result);

        ///clean
        CONSTBUFFER_DecRef(left);
        CONSTBUFFER_DecRef(right);
    }

    /*Tests_SRS_CONSTBUFFER_02_023: [ CONSTBUFFER_HANDLE_contain_same shall return true. ]*/
    TEST_FUNCTION(CONSTBUFFER_HANDLE_contain_same_with_left_and_right_same_returns_true)
    {
        ///arrange
        bool result;
        unsigned char leftSource[2] = { '1', '2' };
        CONSTBUFFER_HANDLE left = CONSTBUFFER_Create(leftSource, sizeof(leftSource));
        ASSERT_IS_NOT_NULL(left);

        unsigned char rightSource[2] = { '1', '2' };
        CONSTBUFFER_HANDLE right = CONSTBUFFER_Create(rightSource, sizeof(rightSource));
        ASSERT_IS_NOT_NULL(right);

        ///act
        result = CONSTBUFFER_HANDLE_contain_same(left, right);

        ///assert
        ASSERT_IS_TRUE(result);

        ///clean
        CONSTBUFFER_DecRef(left);
        CONSTBUFFER_DecRef(right);
    }


/*Tests_SRS_CONSTBUFFER_02_034: [ If handle is NULL then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSizeWithCopy_with_handle_NULL_fails)
{
    ///arrange

    ///act
    CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(NULL, 0, 0);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_CONSTBUFFER_02_035: [ If offset exceeds the capacity of handle then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
/*Tests_SRS_CONSTBUFFER_02_036: [ If offset + size exceed the capacity of handle then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSizeWithCopy_with_offset_exceeding_capacity_fails)
{
    ///arrange
    CONSTBUFFER_HANDLE origin;
    const char source[] = "source";
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
    origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    ///act
    CONSTBUFFER_HANDLE result;
    result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, sizeof(source) + 1, 0);
    ASSERT_IS_NULL(result);
    result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, 0, sizeof(source) + 1);
    ASSERT_IS_NULL(result);
    result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, sizeof(source), 1);
    ASSERT_IS_NULL(result);
    result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, 1, sizeof(source));
    ASSERT_IS_NULL(result);

    ///cleanup
    CONSTBUFFER_DecRef(origin);
}

/*Tests_SRS_CONSTBUFFER_02_040: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSizeWithCopy_with_offset_plus_size_equal_to_UINT_MAX_fail)
{
    ///arrange
    CONSTBUFFER_HANDLE origin;
    const char source[] = "source";
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
    origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    ///act
    CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, sizeof(source) - 1, UINT32_MAX - sizeof(source) + 2);

    ///assert
    ASSERT_IS_NULL(result);

    ///cleanup
    CONSTBUFFER_DecRef(origin);
}

/*Tests_SRS_CONSTBUFFER_02_037: [ CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall allocate enough memory to hold CONSTBUFFER_HANDLE and size bytes. ]*/
/*Tests_SRS_CONSTBUFFER_02_038: [ If size is 0 then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall set the pointed to buffer to NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSizeWithCopy_succeeds_1) /*this has size == 0*/
{
    ///arrange
    CONSTBUFFER_HANDLE origin;
    const char source[] = "source";
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
    origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 1));

    ///act
    CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, 0, 0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    const CONSTBUFFER* content = CONSTBUFFER_GetContent(result);
    ASSERT_IS_NULL(content->buffer);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);

    ///cleanup
    CONSTBUFFER_DecRef(origin);
    CONSTBUFFER_DecRef(result);
}

/*Tests_SRS_CONSTBUFFER_02_037: [ CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall allocate enough memory to hold CONSTBUFFER_HANDLE and size bytes. ]*/
/*Tests_SRS_CONSTBUFFER_02_039: [ CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall set the pointed to a non-NULL value that contains the same bytes as offset...offset+size-1 of handle. ]*/
TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSizeWithCopy_succeeds_2) /*this has size == 1*/
{
    ///arrange
    CONSTBUFFER_HANDLE origin;
    const char source[] = "source";
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
    origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, 1));

    ///act
    CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, 0, 1);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    const CONSTBUFFER* content = CONSTBUFFER_GetContent(result);
    ASSERT_IS_NOT_NULL(content->buffer);
    ASSERT_ARE_EQUAL(uint32_t, 1, content->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(content->buffer, source, 1));
    
    ///cleanup
    CONSTBUFFER_DecRef(origin);
    CONSTBUFFER_DecRef(result);
}

/*Tests_SRS_CONSTBUFFER_02_040: [ If there are any failures then CONSTBUFFER_CreateFromOffsetAndSizeWithCopy shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_CreateFromOffsetAndSizeWithCopy_unhappy_path) /*this fails when malloc fails*/
{
    ///arrange
    CONSTBUFFER_HANDLE origin;
    const char source[] = "source";
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
    origin = CONSTBUFFER_Create((const unsigned char*)source, sizeof(source));
    ASSERT_IS_NOT_NULL(origin);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, 1))
        .SetReturn(NULL);

    ///act
    CONSTBUFFER_HANDLE result = CONSTBUFFER_CreateFromOffsetAndSizeWithCopy(origin, 0, 1);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    CONSTBUFFER_DecRef(origin);
}

/*small tests that check code integrity*/
TEST_FUNCTION(CONSBUFFER_HANDLE_serialization_constants)
{
    ///arrange

    ///act

    ///assert
    ASSERT_ARE_EQUAL(size_t, 1, CONSTBUFFER_VERSION_SIZE);
    ASSERT_ARE_EQUAL(size_t, sizeof(uint32_t), CONSTBUFFER_SIZE_SIZE);
}

/* CONSTBUFFER_get_serialization_size */

/*Tests_SRS_CONSTBUFFER_02_041: [ If source is NULL then CONSTBUFFER_get_serialization_size shall fail and return 0. ]*/
TEST_FUNCTION(CONSTBUFFER_get_serialization_size_with_source_NULL_fails)
{
    ///arrange
    uint32_t result;

    ///act
    result = CONSTBUFFER_get_serialization_size(NULL);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_042: [ If sizeof(uint8_t) + sizeof(uint32_t) + source's size exceed UINT32_MAX then CONSTBUFFER_get_serialization_size shall fail and return 0. ]*/
TEST_FUNCTION(CONSTBUFFER_get_serialization_size_with_too_big_size_fails)
{
    ///arrange
    uint32_t almost4GB = UINT32_MAX - 4; /*this is 4 bytes shy of 4GB. It would not fit.*/
    unsigned char* toobig = real_gballoc_hl_malloc(almost4GB);
    if (toobig == NULL)
    {
        /*do nothing, let the test pass - not a fault in this module*/
    }
    else
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE toobig_h = CONSTBUFFER_CreateWithMoveMemory(toobig, almost4GB);
        ASSERT_IS_NOT_NULL(toobig_h);
        uint32_t result;

        ///act
        result = CONSTBUFFER_get_serialization_size(toobig_h);

        ///assert
        ASSERT_ARE_EQUAL(uint32_t, 0, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///clean
        CONSTBUFFER_DecRef(toobig_h);
    }
}

/*Tests_SRS_CONSTBUFFER_02_042: [ If sizeof(uint8_t) + sizeof(uint32_t) + source's size exceed UINT32_MAX then CONSTBUFFER_get_serialization_size shall fail and return 0. ]*/
/*Tests_SRS_CONSTBUFFER_02_043: [ Otherwise CONSTBUFFER_get_serialization_size shall succeed and return sizeof(uint8_t) + sizeof(uint32_t) + source's size. ]*/
TEST_FUNCTION(CONSTBUFFER_get_serialization_size_with_biggest_size_succeeds)
{
    ///arrange
    uint32_t almost4GB = UINT32_MAX - 5; /*this is 5 bytes shy of 4GB. This is the greatest size for which serialization still works*/
    unsigned char* toobig = real_gballoc_hl_malloc(almost4GB);
    if (toobig == NULL)
    {
        /*do nothing, let the test pass - not a fault in this module*/
    }
    else
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE toobig_h = CONSTBUFFER_CreateWithMoveMemory(toobig, almost4GB);
        ASSERT_IS_NOT_NULL(toobig_h);
        uint32_t result;

        ///act
        result = CONSTBUFFER_get_serialization_size(toobig_h);

        ///assert
        ASSERT_ARE_EQUAL(uint32_t, UINT32_MAX, result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        ///clean
        CONSTBUFFER_DecRef(toobig_h);
    }
}

/*Tests_SRS_CONSTBUFFER_02_042: [ If sizeof(uint8_t) + sizeof(uint32_t) + source's size exceed UINT32_MAX then CONSTBUFFER_get_serialization_size shall fail and return 0. ]*/
/*Tests_SRS_CONSTBUFFER_02_043: [ Otherwise CONSTBUFFER_get_serialization_size shall succeed and return sizeof(uint8_t) + sizeof(uint32_t) + source's size. ]*/
TEST_FUNCTION(CONSTBUFFER_get_serialization_size_with_0_size_succeeds)
{
    ///arrange
    
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 1));
    CONSTBUFFER_HANDLE smallest = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(smallest);
    uint32_t result;

    ///act
    result = CONSTBUFFER_get_serialization_size(smallest);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t), result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(smallest);
    
}

/*Tests_SRS_CONSTBUFFER_02_042: [ If sizeof(uint8_t) + sizeof(uint32_t) + source's size exceed UINT32_MAX then CONSTBUFFER_get_serialization_size shall fail and return 0. ]*/
/*Tests_SRS_CONSTBUFFER_02_043: [ Otherwise CONSTBUFFER_get_serialization_size shall succeed and return sizeof(uint8_t) + sizeof(uint32_t) + source's size. ]*/
TEST_FUNCTION(CONSTBUFFER_get_serialization_size_with_2_size_succeeds)
{
    ///arrange
    unsigned char source[2] = { 1,2 };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source), 1));
    CONSTBUFFER_HANDLE smallest = CONSTBUFFER_Create(source, sizeof(source));
    ASSERT_IS_NOT_NULL(smallest);
    uint32_t result;

    ///act
    result = CONSTBUFFER_get_serialization_size(smallest);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(source), result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(smallest);

}

/* CONSTBUFFER_to_buffer */

/*Tests_SRS_CONSTBUFFER_02_044: [ If source is NULL then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_source_NULL_fails)
{
    ///arrange
    uint32_t size;
    unsigned char* result;

    ///act
    result = CONSTBUFFER_to_buffer(NULL, my_gballoc_malloc_with_context, NULL, &size);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_045: [ If serialized_size is NULL then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_size_NULL_fails)
{
    ///arrange
    unsigned char s[] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(s), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(s, sizeof(s));
    ASSERT_IS_NOT_NULL(source);
    unsigned char* result;

    ///act
    result = CONSTBUFFER_to_buffer(source, my_gballoc_malloc_with_context, NULL, NULL);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/*Tests_SRS_CONSTBUFFER_02_054: [ If there are any failures then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_size_exceeding_UINT32_MAX_fails)
{
    ///arrange
    uint32_t almost4GB = UINT32_MAX - 4; /*this is 4 bytes shy of 4GB. Serialization_size is UINT32_MAX + 1 for almost4GB, which cannot be represented as a uint32_t type*/
    unsigned char* toobig = real_gballoc_hl_malloc(almost4GB);
    if (toobig == NULL)
    {
        /*do nothing, let the test pass - not a fault in this module*/
    }
    else
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE toobig_h = CONSTBUFFER_CreateWithMoveMemory(toobig, almost4GB);
        ASSERT_IS_NOT_NULL(toobig_h);

        ///arrange
        uint32_t serialized_size;

        ///act
        unsigned char* result;

        ///act
        result = CONSTBUFFER_to_buffer(toobig_h, NULL, NULL, &serialized_size);

        ///assert
        ASSERT_IS_NULL(result);

        ///clean

        CONSTBUFFER_DecRef(toobig_h);
    }
}

/*Tests_SRS_CONSTBUFFER_02_046: [ If alloc is NULL then CONSTBUFFER_to_buffer shall use malloc as provided by gballoc_hl_redirect.h. ]*/
/*Tests_SRS_CONSTBUFFER_02_049: [ CONSTBUFFER_to_buffer shall allocate memory using alloc for holding the complete serialization. ]*/
/*Tests_SRS_CONSTBUFFER_02_050: [ CONSTBUFFER_to_buffer shall write at offset 0 of the allocated memory the version of the serialization (currently 1). ]*/
/*Tests_SRS_CONSTBUFFER_02_051: [ CONSTBUFFER_to_buffer shall write at offsets 1-4 of the allocated memory the value of source->alias.size in network byte order. ]*/
/*Tests_SRS_CONSTBUFFER_02_052: [ CONSTBUFFER_to_buffer shall write starting at offset 5 of the allocated memory the bytes of source->alias.buffer. ]*/
/*Tests_SRS_CONSTBUFFER_02_053: [ CONSTBUFFER_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_size_1_with_malloc_succeeds)
{
    ///arrange
    unsigned char s[] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG,sizeof(s), 1));
    
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(s, sizeof(s));
    ASSERT_IS_NOT_NULL(source);
    unsigned char* result;
    uint32_t size;

    STRICT_EXPECTED_CALL(malloc(sizeof(uint8_t)+sizeof(uint32_t) + sizeof(uint8_t))); /*we only serialize a CONSTBUFFER of 1 character*/

    ///act
    result = CONSTBUFFER_to_buffer(source, NULL, NULL, &size);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t), size);

    /*version*/
    uint8_t version_from_serialization;
    read_uint8_t(result + CONSTBUFFER_VERSION_OFFSET, &version_from_serialization);
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, version_from_serialization);

    /*size*/
    uint32_t size_from_serialization;
    read_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, &size_from_serialization);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(s), size_from_serialization);

    /*content*/
    ASSERT_IS_TRUE(memcmp(s, result + CONSTBUFFER_CONTENT_OFFSET, sizeof(s)) == 0);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
    free(result);
}

/*Tests_SRS_CONSTBUFFER_02_046: [ If alloc is NULL then CONSTBUFFER_to_buffer shall use malloc as provided by gballoc_hl_redirect.h. ]*/
/*Tests_SRS_CONSTBUFFER_02_049: [ CONSTBUFFER_to_buffer shall allocate memory using alloc for holding the complete serialization. ]*/
/*Tests_SRS_CONSTBUFFER_02_050: [ CONSTBUFFER_to_buffer shall write at offset 0 of the allocated memory the version of the serialization (currently 1). ]*/
/*Tests_SRS_CONSTBUFFER_02_051: [ CONSTBUFFER_to_buffer shall write at offsets 1-4 of the allocated memory the value of source->alias.size in network byte order. ]*/
/*Tests_SRS_CONSTBUFFER_02_052: [ CONSTBUFFER_to_buffer shall write starting at offset 5 of the allocated memory the bytes of source->alias.buffer. ]*/
/*Tests_SRS_CONSTBUFFER_02_053: [ CONSTBUFFER_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_size_0_with_malloc_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(source);
    unsigned char* result;
    uint32_t size;

    STRICT_EXPECTED_CALL(malloc(sizeof(uint8_t) + sizeof(uint32_t))); /*we only serialize an empty CONSTBUFFER*/

    ///act
    result = CONSTBUFFER_to_buffer(source, NULL, NULL, &size);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t), size);

    /*version*/
    uint8_t version_from_serialization;
    read_uint8_t(result + CONSTBUFFER_VERSION_OFFSET, &version_from_serialization);
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, version_from_serialization);

    /*size*/
    uint32_t size_from_serialization;
    read_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, &size_from_serialization);
    ASSERT_ARE_EQUAL(uint32_t, 0, size_from_serialization);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
    free(result);
}

/*Tests_SRS_CONSTBUFFER_02_046: [ If alloc is NULL then CONSTBUFFER_to_buffer shall use malloc as provided by gballoc_hl_redirect.h. ]*/
/*Tests_SRS_CONSTBUFFER_02_049: [ CONSTBUFFER_to_buffer shall allocate memory using alloc for holding the complete serialization. ]*/
/*Tests_SRS_CONSTBUFFER_02_050: [ CONSTBUFFER_to_buffer shall write at offset 0 of the allocated memory the version of the serialization (currently 1). ]*/
/*Tests_SRS_CONSTBUFFER_02_051: [ CONSTBUFFER_to_buffer shall write at offsets 1-4 of the allocated memory the value of source->alias.size in network byte order. ]*/
/*Tests_SRS_CONSTBUFFER_02_052: [ CONSTBUFFER_to_buffer shall write starting at offset 5 of the allocated memory the bytes of source->alias.buffer. ]*/
/*Tests_SRS_CONSTBUFFER_02_053: [ CONSTBUFFER_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_size_1_with_custom_alloc_succeeds)
{
    ///arrange
    unsigned char s[] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(s), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(s, sizeof(s));
    ASSERT_IS_NOT_NULL(source);
    unsigned char* result;
    uint32_t size;

    STRICT_EXPECTED_CALL(test_alloc(sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t), TEST_ALLOC_CONTEXT)); /*we only serialize a CONSTBUFFER of 1 character*/

    ///act
    result = CONSTBUFFER_to_buffer(source, test_alloc, TEST_ALLOC_CONTEXT, &size);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t), size);

    /*version*/
    uint8_t version_from_serialization;
    read_uint8_t(result + CONSTBUFFER_VERSION_OFFSET, &version_from_serialization);
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, version_from_serialization);

    /*size*/
    uint32_t size_from_serialization;
    read_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, &size_from_serialization);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(s), size_from_serialization);

    /*content*/
    ASSERT_IS_TRUE(memcmp(s, result + CONSTBUFFER_CONTENT_OFFSET, sizeof(s)) == 0);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
    free(result);
}

/*Tests_SRS_CONSTBUFFER_02_046: [ If alloc is NULL then CONSTBUFFER_to_buffer shall use malloc as provided by gballoc_hl_redirect.h. ]*/
/*Tests_SRS_CONSTBUFFER_02_049: [ CONSTBUFFER_to_buffer shall allocate memory using alloc for holding the complete serialization. ]*/
/*Tests_SRS_CONSTBUFFER_02_050: [ CONSTBUFFER_to_buffer shall write at offset 0 of the allocated memory the version of the serialization (currently 1). ]*/
/*Tests_SRS_CONSTBUFFER_02_051: [ CONSTBUFFER_to_buffer shall write at offsets 1-4 of the allocated memory the value of source->alias.size in network byte order. ]*/
/*Tests_SRS_CONSTBUFFER_02_052: [ CONSTBUFFER_to_buffer shall write starting at offset 5 of the allocated memory the bytes of source->alias.buffer. ]*/
/*Tests_SRS_CONSTBUFFER_02_053: [ CONSTBUFFER_to_buffer shall succeed, write in serialized_size the size of the serialization and return the allocated memory. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_size_2_with_custom_alloc_succeeds)
{
    ///arrange
    unsigned char s[] = { 1, 2 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(s), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(s, sizeof(s));
    ASSERT_IS_NOT_NULL(source);
    unsigned char* result;
    uint32_t size;

    STRICT_EXPECTED_CALL(test_alloc(sizeof(uint8_t) + sizeof(uint32_t) + 2*sizeof(uint8_t), TEST_ALLOC_CONTEXT)); /*we only serialize a CONSTBUFFER of 2 characters*/

    ///act
    result = CONSTBUFFER_to_buffer(source, test_alloc, TEST_ALLOC_CONTEXT, &size);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t) + 2*sizeof(uint8_t), size);

    /*version*/
    uint8_t version_from_serialization;
    read_uint8_t(result + CONSTBUFFER_VERSION_OFFSET, &version_from_serialization);
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, version_from_serialization);

    /*size*/
    uint32_t size_from_serialization;
    read_uint32_t(result + CONSTBUFFER_SIZE_OFFSET, &size_from_serialization);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(s), size_from_serialization);

    /*content*/
    ASSERT_IS_TRUE(memcmp(s, result + CONSTBUFFER_CONTENT_OFFSET, sizeof(s)) == 0);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
    free(result);
}

/*Tests_SRS_CONSTBUFFER_02_054: [ If there are any failures then CONSTBUFFER_to_buffer shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_to_buffer_with_size_2_with_custom_alloc_unhappy_path)
{
    ///arrange
    unsigned char s[] = { 1, 2 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(s), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(s, sizeof(s));
    ASSERT_IS_NOT_NULL(source);
    unsigned char* result;
    uint32_t size;

    STRICT_EXPECTED_CALL(test_alloc(sizeof(uint8_t) + sizeof(uint32_t) + 2 * sizeof(uint8_t), TEST_ALLOC_CONTEXT)) /*we only serialize a CONSTBUFFER of 2 characters*/
        .SetReturn(NULL);

    ///act
    result = CONSTBUFFER_to_buffer(source, test_alloc, TEST_ALLOC_CONTEXT, &size);

    ///assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/* CONSTBUFFER_to_fixed_size_buffer */

/*Tests_SRS_CONSTBUFFER_02_055: [ If source is NULL then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_with_source_NULL_fails)
{
    ///arrange
    unsigned char destination[100];
    uint32_t size;
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_to_fixed_size_buffer(NULL, destination, sizeof(destination), &size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/*Tests_SRS_CONSTBUFFER_02_056: [ If destination is NULL then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_with_destination_NULL_fails)
{
    ///arrange
    unsigned char source_bytes[1] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source_bytes), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(source_bytes, sizeof(source_bytes));
    ASSERT_IS_NOT_NULL(source);

    uint32_t size;
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_to_fixed_size_buffer(source, NULL, 0, &size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/*Tests_SRS_CONSTBUFFER_02_057: [ If serialized_size is NULL then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_with_serialized_size_NULL_fails)
{
    ///arrange
    unsigned char source_bytes[1] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source_bytes), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(source_bytes, sizeof(source_bytes));
    ASSERT_IS_NOT_NULL(source);

    unsigned char destination[100];

    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_to_fixed_size_buffer(source, destination, sizeof(destination), NULL);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/*Tests_SRS_CONSTBUFFER_02_074: [ If there are any failures then CONSTBUFFER_to_fixed_size_buffer shall fail and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_ERROR. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_when_serialized_size_overflows_fails)
{
    ///arrange
    uint32_t almost4GB = UINT32_MAX - 4; /*this is 4 bytes shy of 4GB. Serialization_size is UINT32_MAX + 1 for almost4GB, which cannot be represented as a uint32_t type*/
    unsigned char* toobig = real_gballoc_hl_malloc(almost4GB);
    if (toobig == NULL)
    {
        /*do nothing, let the test pass - not a fault in this module*/
    }
    else
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        CONSTBUFFER_HANDLE toobig_h = CONSTBUFFER_CreateWithMoveMemory(toobig, almost4GB);
        ASSERT_IS_NOT_NULL(toobig_h);

        unsigned char* destination = real_gballoc_hl_malloc(UINT32_MAX);
        if (destination == NULL)
        {
            /*not really a fault of this test, let the test pass*/
        }
        else
        {
            ///arrange
            uint32_t serialized_size;

            ///act
            CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

            ///act
            result = CONSTBUFFER_to_fixed_size_buffer(toobig_h, destination, UINT32_MAX, &serialized_size);

            ///assert
            ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_ERROR, result);
            
            ///clean
            real_gballoc_hl_free(destination);
        }
        CONSTBUFFER_DecRef(toobig_h);
    }
}

/*Tests_SRS_CONSTBUFFER_02_058: [ If the size of serialization exceeds destination_size then CONSTBUFFER_to_fixed_size_buffer shall fail, write in serialized_size how much it would need and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_that_would_overflow_destination_fails)
{
    ///arrange
    unsigned char source_bytes[1] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source_bytes), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(source_bytes, sizeof(source_bytes));
    ASSERT_IS_NOT_NULL(source);

    unsigned char destination[5]; /*the above CONSTBUFFER_HANDLE requires 6 bytes...*/
    uint32_t serialized_size;
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_to_fixed_size_buffer(source, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_INSUFFICIENT_BUFFER, result);
    ASSERT_ARE_EQUAL(uint32_t, 6, serialized_size);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/*Tests_SRS_CONSTBUFFER_02_059: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 0 of destination the version of serialization (currently 1). ]*/
/*Tests_SRS_CONSTBUFFER_02_060: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 1 of destination the value of source->alias.size in network byte order. ]*/
/*Tests_SRS_CONSTBUFFER_02_061: [ CONSTBUFFER_to_fixed_size_buffer shall copy all the bytes of source->alias.buffer in destination starting at offset 5. ]*/
/*Tests_SRS_CONSTBUFFER_02_062: [ CONSTBUFFER_to_fixed_size_buffer shall succeed, write in serialized_size how much it used and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_succeeds_1) /*in this case, the size of the buffer matches perfectly the serialization size*/
{
    ///arrange
    unsigned char source_bytes[1] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source_bytes), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(source_bytes, sizeof(source_bytes));
    ASSERT_IS_NOT_NULL(source);

    unsigned char destination[6]; /*the above CONSTBUFFER_HANDLE requires 6 bytes...*/
    uint32_t serialized_size;
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_to_fixed_size_buffer(source, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, 6, serialized_size);

    /*version*/
    uint8_t serialized_version;
    read_uint8_t(destination + CONSTBUFFER_VERSION_OFFSET, &serialized_version);
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, serialized_version);

    /*size*/
    uint32_t size;
    read_uint32_t(destination + CONSTBUFFER_SIZE_OFFSET, &size);
    ASSERT_ARE_EQUAL(uint32_t, 1, size);

    /*content*/
    ASSERT_IS_TRUE(memcmp(source_bytes, destination + CONSTBUFFER_CONTENT_OFFSET, sizeof(source_bytes)) == 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/*Tests_SRS_CONSTBUFFER_02_059: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 0 of destination the version of serialization (currently 1). ]*/
/*Tests_SRS_CONSTBUFFER_02_060: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 1 of destination the value of source->alias.size in network byte order. ]*/
/*Tests_SRS_CONSTBUFFER_02_061: [ CONSTBUFFER_to_fixed_size_buffer shall copy all the bytes of source->alias.buffer in destination starting at offset 5. ]*/
/*Tests_SRS_CONSTBUFFER_02_062: [ CONSTBUFFER_to_fixed_size_buffer shall succeed, write in serialized_size how much it used and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_succeeds_2) /*in this case, the size of the buffer exceeds the serialization size*/
{
    ///arrange
    unsigned char source_bytes[1] = { 1 };
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, sizeof(source_bytes), 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(source_bytes, sizeof(source_bytes));
    ASSERT_IS_NOT_NULL(source);

    unsigned char destination[7]; /*the above CONSTBUFFER_HANDLE requires 6 bytes...*/
    uint32_t serialized_size;
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_to_fixed_size_buffer(source, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, 6, serialized_size);

    /*version*/
    uint8_t serialized_version;
    read_uint8_t(destination + CONSTBUFFER_VERSION_OFFSET, &serialized_version);
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, serialized_version);

    /*size*/
    uint32_t size;
    read_uint32_t(destination + CONSTBUFFER_SIZE_OFFSET, &size);
    ASSERT_ARE_EQUAL(uint32_t, 1, size);

    /*content*/
    ASSERT_IS_TRUE(memcmp(source_bytes, destination + CONSTBUFFER_CONTENT_OFFSET, sizeof(source_bytes)) == 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/*Tests_SRS_CONSTBUFFER_02_059: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 0 of destination the version of serialization (currently 1). ]*/
/*Tests_SRS_CONSTBUFFER_02_060: [ CONSTBUFFER_to_fixed_size_buffer shall write at offset 1 of destination the value of source->alias.size in network byte order. ]*/
/*Tests_SRS_CONSTBUFFER_02_061: [ CONSTBUFFER_to_fixed_size_buffer shall copy all the bytes of source->alias.buffer in destination starting at offset 5. ]*/
/*Tests_SRS_CONSTBUFFER_02_062: [ CONSTBUFFER_to_fixed_size_buffer shall succeed, write in serialized_size how much it used and return CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_to_fixed_size_buffer_succeeds_3) /*in this case, an empty constbuffer_handle is serialized*/
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 1));
    CONSTBUFFER_HANDLE source = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(source);

    unsigned char destination[7]; /*the above CONSTBUFFER_HANDLE requires 5 bytes...*/
    uint32_t serialized_size;
    CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_to_fixed_size_buffer(source, destination, sizeof(destination), &serialized_size);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT, CONSTBUFFER_TO_FIXED_SIZE_BUFFER_RESULT_OK, result);
    ASSERT_ARE_EQUAL(uint32_t, 5, serialized_size);

    /*version*/
    uint8_t serialized_version;
    read_uint8_t(destination + CONSTBUFFER_VERSION_OFFSET, &serialized_version);
    ASSERT_ARE_EQUAL(uint8_t, CONSTBUFFER_VERSION_V1, serialized_version);

    /*size*/
    uint32_t size;
    read_uint32_t(destination + CONSTBUFFER_SIZE_OFFSET, &size);
    ASSERT_ARE_EQUAL(uint32_t, 0, size);

    /*content*/
    /*doesn't have*/

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(source);
}

/* CONSTBUFFER_from_buffer */

/*Tests_SRS_CONSTBUFFER_02_063: [ If source is NULL then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_source_NULL_fails)
{
    ///arrange
    uint32_t consumed;
    CONSTBUFFER_HANDLE destination;
    CONSTBUFFER_FROM_BUFFER_RESULT result;

    ///act
    result = CONSTBUFFER_from_buffer(NULL, 6, &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_064: [ If consumed is NULL then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_consumed_NULL_fails)
{
    ///arrange
    CONSTBUFFER_HANDLE destination;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    unsigned char source[10] = { 0 };
    ///act
    result = CONSTBUFFER_from_buffer(source, 6, NULL, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_065: [ If destination is NULL then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_destination_NULL_fails)
{
    ///arrange
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[10] = { 0 };
    ///act
    result = CONSTBUFFER_from_buffer(source, 6, &consumed, NULL);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_066: [ If size is 0 then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_size_0_fails)
{
    ///arrange
    CONSTBUFFER_HANDLE destination;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[10] = { 0 };
    ///act
    result = CONSTBUFFER_from_buffer(source, 0, &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_ARG, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_067: [ If source byte at offset 0 is not 1 (current version) then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_version_0_fails)
{
    ///arrange
    CONSTBUFFER_HANDLE destination;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[10] = { 0 }; /*note first byte to be 0*/
    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_068: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_insufficient_bytes_fails_1) /*in this case there are not enough bytes for version and size*/
{
    ///arrange
    CONSTBUFFER_HANDLE destination;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[10] = { 1 }; /*note first byte to be 1, rest are 0*/
    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(uint8_t) + sizeof(uint32_t) - 1, &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONSTBUFFER_02_069: [ CONSTBUFFER_from_buffer shall read the number of serialized content bytes from offset 1 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_071: [ CONSTBUFFER_from_buffer shall create a CONSTBUFFER_HANDLE from the bytes at offset 5 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_072: [ CONSTBUFFER_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed CONSTBUFFER_HANDLE and return CONSTBUFFER_FROM_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_0_size_succeeds)
{
    ///arrange
    CONSTBUFFER_HANDLE destination = NULL;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[] = { 
        1, /*version*/
        0,0,0,0 /*size = 0*/
    };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 1));

    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(destination);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t), consumed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /*size*/
    const CONSTBUFFER* content= CONSTBUFFER_GetContent(destination);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);

    ///clean
    CONSTBUFFER_DecRef(destination);
}

/*Tests_SRS_CONSTBUFFER_02_069: [ CONSTBUFFER_from_buffer shall read the number of serialized content bytes from offset 1 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_071: [ CONSTBUFFER_from_buffer shall create a CONSTBUFFER_HANDLE from the bytes at offset 5 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_072: [ CONSTBUFFER_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed CONSTBUFFER_HANDLE and return CONSTBUFFER_FROM_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_0_size_from_bigger_buffer_succeeds)
{
    ///arrange
    CONSTBUFFER_HANDLE destination = NULL;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[] = {
        1, /*version*/
        0,0,0,0, /*size = 0*/
        8 /*extra byte*/
    };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 0, 1));

    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(destination);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t), consumed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /*size*/
    const CONSTBUFFER* content = CONSTBUFFER_GetContent(destination);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);

    ///clean
    CONSTBUFFER_DecRef(destination);
}

/*Tests_SRS_CONSTBUFFER_02_070: [ If source's size is less than sizeof(uint8_t) + sizeof(uint32_t) + number of content bytes then CONSTBUFFER_from_buffer shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_1_size_from_insufficient_buffer_fails)
{
    ///arrange
    CONSTBUFFER_HANDLE destination = NULL;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[] = {
        1, /*version*/
        0,0,0,1 /*size = 1*/
    };

    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_INVALID_DATA, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    CONSTBUFFER_DecRef(destination);
}

/*Tests_SRS_CONSTBUFFER_02_069: [ CONSTBUFFER_from_buffer shall read the number of serialized content bytes from offset 1 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_071: [ CONSTBUFFER_from_buffer shall create a CONSTBUFFER_HANDLE from the bytes at offset 5 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_072: [ CONSTBUFFER_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed CONSTBUFFER_HANDLE and return CONSTBUFFER_FROM_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_1_size_succeeds)
{
    ///arrange
    CONSTBUFFER_HANDLE destination = NULL;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[] = {
        1, /*version*/
        0,0,0,1,/*size = 1*/
        0x42
    };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, 1));

    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(destination);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t), consumed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /*size*/
    const CONSTBUFFER* content = CONSTBUFFER_GetContent(destination);
    ASSERT_ARE_EQUAL(uint32_t, 1, content->size);

    /*content*/
    ASSERT_IS_TRUE(memcmp(source + CONSTBUFFER_CONTENT_OFFSET, content->buffer, content->size)==0);

    ///clean
    CONSTBUFFER_DecRef(destination);
}

/*Tests_SRS_CONSTBUFFER_02_069: [ CONSTBUFFER_from_buffer shall read the number of serialized content bytes from offset 1 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_071: [ CONSTBUFFER_from_buffer shall create a CONSTBUFFER_HANDLE from the bytes at offset 5 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_072: [ CONSTBUFFER_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed CONSTBUFFER_HANDLE and return CONSTBUFFER_FROM_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_1_size_from_greater_size_buffer_succeeds)
{
    ///arrange
    CONSTBUFFER_HANDLE destination = NULL;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[] = {
        1, /*version*/
        0,0,0,1,/*size = 1*/
        0x42, 0x43 /*0x43 is extraneous*/
    };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 1, 1));

    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(destination);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint8_t), consumed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /*size*/
    const CONSTBUFFER* content = CONSTBUFFER_GetContent(destination);
    ASSERT_ARE_EQUAL(uint32_t, 1, content->size);

    /*content*/
    ASSERT_IS_TRUE(memcmp(source + CONSTBUFFER_CONTENT_OFFSET, content->buffer, content->size)==0);

    ///clean
    CONSTBUFFER_DecRef(destination);
}

/*Tests_SRS_CONSTBUFFER_02_069: [ CONSTBUFFER_from_buffer shall read the number of serialized content bytes from offset 1 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_071: [ CONSTBUFFER_from_buffer shall create a CONSTBUFFER_HANDLE from the bytes at offset 5 of source. ]*/
/*Tests_SRS_CONSTBUFFER_02_072: [ CONSTBUFFER_from_buffer shall succeed, write in consumed the total number of consumed bytes from source, write in destination the constructed CONSTBUFFER_HANDLE and return CONSTBUFFER_FROM_BUFFER_RESULT_OK. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_2_size_succeeds)
{
    ///arrange
    CONSTBUFFER_HANDLE destination = NULL;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[] = {
        1, /*version*/
        0,0,0,2,/*size = 2*/
        0x42, 0x43
    };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, 1));

    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_OK, result);
    ASSERT_IS_NOT_NULL(destination);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(uint8_t) + sizeof(uint32_t) + 2*sizeof(uint8_t), consumed);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    /*size*/
    const CONSTBUFFER* content = CONSTBUFFER_GetContent(destination);
    ASSERT_ARE_EQUAL(uint32_t, 2, content->size);

    /*content*/
    ASSERT_IS_TRUE(memcmp(source + CONSTBUFFER_CONTENT_OFFSET, content->buffer, content->size) == 0);

    ///clean
    CONSTBUFFER_DecRef(destination);
}

/*Tests_SRS_CONSTBUFFER_02_073: [ If there are any failures then shall fail and return CONSTBUFFER_FROM_BUFFER_RESULT_ERROR. ]*/
TEST_FUNCTION(CONSTBUFFER_from_buffer_with_2_size_unhappy_path)
{
    ///arrange
    CONSTBUFFER_HANDLE destination = NULL;
    CONSTBUFFER_FROM_BUFFER_RESULT result;
    uint32_t consumed;
    unsigned char source[] = {
        1, /*version*/
        0,0,0,2,/*size = 2*/
        0x42, 0x43
    };

    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, 2, 1))
        .SetReturn(NULL);

    ///act
    result = CONSTBUFFER_from_buffer(source, sizeof(source), &consumed, &destination);

    ///assert
    ASSERT_ARE_EQUAL(CONSTBUFFER_FROM_BUFFER_RESULT, CONSTBUFFER_FROM_BUFFER_RESULT_ERROR, result);
    ASSERT_IS_NULL(destination);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*CONSTBUFFER_createWritableHandle*/

/*Tests_SRS_CONSTBUFFER_51_001: [ If `size` is 0, then CONSTBUFFER_createWritableHandle shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_createWritableHandle_invalid_args_fails)
{
    ///arrange

    ///act
    CONSTBUFFER_WRITABLE_HANDLE handle = CONSTBUFFER_createWritableHandle(0);

    ///assert
    ASSERT_IS_NULL(handle);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_003: [ If any error occurs, `CONSTBUFFER_createWritableHandle` shall fail and return NULL. ]*/
TEST_FUNCTION(CONSTBUFFER_createWritableHandle_fails_when_malloc_fails)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE handle;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1))
        .SetReturn(NULL);

    ///act
    handle = CONSTBUFFER_createWritableHandle(BUFFER1_length);

    ///assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_004: [ `CONSTBUFFER_createWritableHandle` shall set the ref count of the newly created `CONSTBUFFER_WRITABLE_HANDLE` to 1. ]*/
TEST_FUNCTION(CONSTBUFFER_createWritableHandle_is_ref_counted_1)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE handle = CONSTBUFFER_createWritableHandle(BUFFER1_length);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    CONSTBUFFER_WritableHandleDecRef(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_002: [ `CONSTBUFFER_createWritableHandle` shall allocate memory for the `CONSTBUFFER_WRITABLE_HANDLE`. ]*/
/*Tests_SRS_CONSTBUFFER_51_005: [ `CONSTBUFFER_createWritableHandle` shall succeed and return a non - `NULL` `CONSTBUFFER_WRITABLE_HANDLE`. ]*/
TEST_FUNCTION(CONSTBUFFER_createWritableHandle_succeeds)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE handle;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1));

    ///act
    handle = CONSTBUFFER_createWritableHandle(BUFFER1_length);

    ///assert
    ASSERT_IS_NOT_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    CONSTBUFFER_WritableHandleDecRef(handle);
}

/*CONSTBUFFER_getWritableBuffer*/

/*Tests_SRS_CONSTBUFFER_51_006: [ If `constbufferHandle` is `NULL`, then `CONSTBUFFER_getWritableBuffer` shall fail and return `NULL`. ] */
TEST_FUNCTION(CONSTBUFFER_getWritableBuffer_invalid_args_fails)
{
    ///arrange
    unsigned char* buffer;

    ///act
    buffer = CONSTBUFFER_getWritableBuffer(NULL);

    ///assert
    ASSERT_IS_NULL(buffer);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_007: [ `CONSTBUFFER_getWritableBuffer` shall succeed and returns a pointer to the non-CONST buffer of `constbufferWritableHandle`. ]*/
TEST_FUNCTION(CONSTBUFFER_getWritableBuffer_succeeds)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1));

    ///act
    constbufferWritableHandle = CONSTBUFFER_createWritableHandle(BUFFER1_length); 
    unsigned char* buffer = CONSTBUFFER_getWritableBuffer(constbufferWritableHandle);

    ///assert
    ASSERT_IS_NOT_NULL(buffer);   
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    CONSTBUFFER_WritableHandleDecRef(constbufferWritableHandle);
}

/*CONSTBUFFER_sealWritableHandle*/

/*Tests_SRS_CONSTBUFFER_51_008: [ If `constbufferWritableHandle` is `NULL` then `CONSTBUFFER_sealWritableHandle` shall fail and return `NULL`. ]*/
TEST_FUNCTION(CONSTBUFFER_sealWritableHandle_invalid_args_fails)
{
    ///arrange
    CONSTBUFFER_HANDLE handle;

    ///act
    handle = CONSTBUFFER_sealWritableHandle(NULL);

    ///assert
    ASSERT_IS_NULL(handle);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_009: [`CONSTBUFFER_sealWritableHandle` shall succeed and return a non - `NULL` `CONSTBUFFER_HANDLE`.]*/
TEST_FUNCTION(CONSTBUFFER_sealWritableHandle_succeeds)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle;
    CONSTBUFFER_HANDLE handle;
    uint32_t writableBufferSize;
    unsigned char* writableBuffer;  
    const CONSTBUFFER * content;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1));
       
    ///act
    constbufferWritableHandle = CONSTBUFFER_createWritableHandle(BUFFER1_length);
    writableBufferSize = CONSTBUFFER_getWritableBufferSize(constbufferWritableHandle);
    writableBuffer = CONSTBUFFER_getWritableBuffer(constbufferWritableHandle);

    /*filling data*/
    (void)memcpy(writableBuffer, buffer1, writableBufferSize);

    /*sealing*/
    handle = CONSTBUFFER_sealWritableHandle(constbufferWritableHandle);
    content = CONSTBUFFER_GetContent(handle);
    
    ///assert
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, content->size, writableBufferSize);
    ASSERT_ARE_EQUAL(char_ptr, content->buffer, writableBuffer);
    /*content*/
    ASSERT_IS_TRUE(memcmp(buffer1, content->buffer, content->size) == 0);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    CONSTBUFFER_DecRef(handle);
}

/*CONSTBUFFER_WritableHandleIncRef*/

/*Tests_SRS_CONSTBUFFER_51_010: [ If `constbufferWritableHandle` is NULL then `CONSTBUFFER_WritableHandleIncRef` shall return. ]*/
TEST_FUNCTION(CONSTBUFFER_WritableHandleIncRef_invalid_args_fails)
{
    ///arrange

    ///act
    CONSTBUFFER_WritableHandleIncRef(NULL);

    ///assert
   
    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_011: [ Otherwise, `CONSTBUFFER_WritableHandleIncRef` shall increment the reference count. ]*/
TEST_FUNCTION(CONSTBUFFER_WritableHandleIncRef_succeeds)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE handle = CONSTBUFFER_createWritableHandle(BUFFER1_length);
    umock_c_reset_all_calls();

    ///act
    CONSTBUFFER_WritableHandleIncRef(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    CONSTBUFFER_WritableHandleDecRef(handle);
    CONSTBUFFER_WritableHandleDecRef(handle);
}

/*CONSTBUFFER_WritableHandleDecRef*/

/*Tests_SRS_CONSTBUFFER_51_012: [ If `constbufferWritableHandle` is NULL then `CONSTBUFFER_WritableHandleDecRef` shall do nothing. ]*/
TEST_FUNCTION(CONSTBUFFER_WritableHandleDecReff_invalid_args_fails)
{
    ///arrange

    ///act
    CONSTBUFFER_WritableHandleDecRef(NULL);

    ///assert

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_013: [ Otherwise, `CONSTBUFFER_WritableHandleDecRef` shall decrement the refcount on the `constbufferWritableHandle` handle. ]*/
/*Tests_SRS_CONSTBUFFER_51_014: [ If the refcount reaches zero, then `CONSTBUFFER_WritableHandleDecRef` shall deallocate all resources used by the CONSTBUFFER_HANDLE. ]*/
TEST_FUNCTION(CONSTBUFFER_WritableHandleDecReff_succeeds)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE handle = CONSTBUFFER_createWritableHandle(BUFFER1_length);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    CONSTBUFFER_WritableHandleDecRef(handle);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
}

/*CONSTBUFFER_getWritableBufferSize*/

/*Tests_SRS_CONSTBUFFER_51_015: [ If `constbufferWritableHandle` is `NULL`, then `CONSTBUFFER_getWritableBufferSize` return 0. ] */
TEST_FUNCTION(CONSTBUFFER_getWritableBufferSize_invalid_args_fails)
{
    ///arrange
    uint32_t bufferSize;

    ///act
    bufferSize = CONSTBUFFER_getWritableBufferSize(NULL);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, bufferSize, 0);

    ///cleanup
}

/*Tests_SRS_CONSTBUFFER_51_016: [ `CONSTBUFFER_getWritableBufferSize` shall succeed and returns the size of the writable buffer of `constbufferWritableHandle`. ]*/
TEST_FUNCTION(CONSTBUFFER_getWritableBufferSize_succeeds)
{
    ///arrange
    CONSTBUFFER_WRITABLE_HANDLE constbufferWritableHandle;
    uint32_t bufferSize;
    STRICT_EXPECTED_CALL(malloc_flex(IGNORED_ARG, BUFFER1_length, 1));

    ///act
    constbufferWritableHandle = CONSTBUFFER_createWritableHandle(BUFFER1_length);


    bufferSize = CONSTBUFFER_getWritableBufferSize(constbufferWritableHandle);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, bufferSize, BUFFER1_length);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    CONSTBUFFER_WritableHandleDecRef(constbufferWritableHandle);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
