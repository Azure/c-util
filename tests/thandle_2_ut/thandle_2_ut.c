// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "real_gballoc_hl.h"
#include "real_gballoc_hl_renames.h"

static void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "umock_c/umock_c.h"
#define ENABLE_MOCKS
#include "malloc_mocks.h"
#undef ENABLE_MOCKS

#include "g_on_t_off_i_off.h"
#include "g_on_t_off_i_on.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    REGISTER_GLOBAL_MOCK_HOOK(global_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(global_malloc_flex, real_gballoc_hl_malloc_flex);
    REGISTER_GLOBAL_MOCK_HOOK(global_free, real_gballoc_hl_free);

    REGISTER_GLOBAL_MOCK_HOOK(type_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(type_malloc_flex, real_gballoc_hl_malloc_flex);
    REGISTER_GLOBAL_MOCK_HOOK(type_free, real_gballoc_hl_free);

    REGISTER_GLOBAL_MOCK_HOOK(var_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(var_malloc_flex, real_gballoc_hl_malloc_flex);
    REGISTER_GLOBAL_MOCK_HOOK(var_free, real_gballoc_hl_free);

    umock_c_init(on_umock_c_error);
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

TEST_FUNCTION(g_on_t_off_i_off_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(global_malloc(IGNORED_ARG));

    ///act
    THANDLE(G_ON_T_OFF_I_OFF_DUMMY) dummy = G_ON_T_OFF_I_OFF_create(3);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 3, dummy->x);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_ON_T_OFF_I_OFF_DUMMY)(&dummy, NULL);
}

TEST_FUNCTION(g_off_t_off_i_on_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(var_malloc(IGNORED_ARG));

    ///act
    THANDLE(G_OFF_T_OFF_I_ON_DUMMY) dummy = G_OFF_T_OFF_I_ON_create(4);

    ///assert
    ASSERT_IS_NOT_NULL(dummy);
    ASSERT_ARE_EQUAL(int, 4, dummy->x);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    THANDLE_ASSIGN(G_OFF_T_OFF_I_ON_DUMMY)(&dummy, NULL);
}



END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

