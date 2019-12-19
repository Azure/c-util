// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#undef DECLSPEC_IMPORT

#pragma warning(disable: 4273)

#include <winsock2.h>

void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"
#include "azure_macro_utils/macro_utils.h"

#define ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"
#undef ENABLE_MOCKS

#include "azure_c_util/platform.h"

#define ENABLE_MOCKS

MOCK_FUNCTION_WITH_CODE(WSAAPI, int, WSAStartup, WORD, wVersionRequested, LPWSADATA, lpWSAData)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(WSAAPI, int, WSACleanup)
MOCK_FUNCTION_END(0)

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(platform_win32_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);
    result = umocktypes_stdint_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(WORD, int);
    REGISTER_UMOCK_ALIAS_TYPE(LPWSADATA, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

TEST_FUNCTION(platform_init_success)
{
    int result;

    //arrange
    STRICT_EXPECTED_CALL(WSAStartup(IGNORED_NUM_ARG, IGNORED_PTR_ARG));

    //act
    result = platform_init();

    //assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(platform_init_WSAStartup_0_fail)
{
    int result;

    //arrange
    STRICT_EXPECTED_CALL(WSAStartup(IGNORED_NUM_ARG, IGNORED_PTR_ARG)).SetReturn(1);

    //act
    result = platform_init();

    //assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

TEST_FUNCTION(platform_deinit_success)
{
    //arrange
    STRICT_EXPECTED_CALL(WSACleanup());

    //act
    platform_deinit();

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
}

END_TEST_SUITE(platform_win32_ut)
