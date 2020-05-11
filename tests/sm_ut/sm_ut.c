// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "azure_macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

void* my_gballoc_malloc(size_t size)
{
    return malloc(size);
}

void my_gballoc_free(void* ptr)
{
    free(ptr);
}

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"

#define ENABLE_MOCKS

#include "azure_c_util/gballoc.h"
#include "azure_c_util/interlocked_hl.h"
#undef ENABLE_MOCKS

#include "real_interlocked_hl.h"

#include "azure_c_util/sm.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static SM_HANDLE TEST_sm_create(void)
{
    SM_HANDLE result;
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG));
    result = sm_create("a");
    ASSERT_IS_NOT_NULL(result);

    return result;
}

MU_DEFINE_ENUM_STRINGS(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(SM_RESULT, SM_RESULT_VALUES);

BEGIN_TEST_SUITE(sm_unittests)

TEST_SUITE_INITIALIZE(setsBufferTempSize)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    umock_c_init(on_umock_c_error);

    umocktypes_windows_register_types();

    REGISTER_GLOBAL_MOCK_HOOK(gballoc_malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(gballoc_free, my_gballoc_free);

    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();

    REGISTER_TYPE(INTERLOCKED_HL_RESULT, INTERLOCKED_HL_RESULT);
    REGISTER_TYPE(SM_RESULT, SM_RESULT);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
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

/*Tests_SRS_SM_02_001: [ If name is NULL then sm_create shall behave as if name was "NO_NAME". ]*/
TEST_FUNCTION(sm_create_with_name_NULL_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG));

    ///act
    SM_HANDLE sm = sm_create(NULL);

    ///assert
    ASSERT_IS_NOT_NULL(sm);

    ///clean
    sm_destroy(sm);
}

/*Tests_SRS_SM_02_037: [ sm_create shall set state of SM_CREATED and n to 0. ]*/
TEST_FUNCTION(sm_create_with_name_non_NULL_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(gballoc_malloc(IGNORED_ARG));

    ///act
    SM_HANDLE sm = sm_create("bleeding edge");

    ///assert
    ASSERT_IS_NOT_NULL(sm);

    ///clean
    sm_destroy(sm);
}

END_TEST_SUITE(sm_unittests)

