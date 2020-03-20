// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#endif

#include "rpc.h"
#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "azure_c_util/uniqueid.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCK_FUNCTION_WITH_CODE(, RPC_STATUS, mocked_UuidCreate,
    UUID __RPC_FAR*, Uuid);
MOCK_FUNCTION_END(UuidCreate(Uuid))
MOCK_FUNCTION_WITH_CODE(, RPC_STATUS, mocked_UuidToStringA,
    const UUID __RPC_FAR*, Uuid,
    RPC_CSTR __RPC_FAR*, StringUuid);
MOCK_FUNCTION_END(UuidToStringA(Uuid, StringUuid))
MOCK_FUNCTION_WITH_CODE(, RPC_STATUS, mocked_RpcStringFreeA,
    RPC_CSTR __RPC_FAR*, String)
MOCK_FUNCTION_END(RpcStringFreeA(String))

#ifdef __cplusplus
}
#endif

static TEST_MUTEX_HANDLE g_testByTest;

#define BUFFER_SIZE         37

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
TEST_DEFINE_ENUM_TYPE(UNIQUEID_RESULT, UNIQUEID_RESULT_VALUES);

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(uniqueid_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_UuidCreate, RPC_S_INTERNAL_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_UuidToStringA, RPC_S_INTERNAL_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_RpcStringFreeA, RPC_S_INTERNAL_ERROR);
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
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* UniqueId_Generate */
/* Tests_SRS_UNIQUEID_07_002: [If uid is NULL then UniqueId_Generate shall return UNIQUEID_INVALID_ARG] */
TEST_FUNCTION(UniqueId_Generate_UID_NULL_Fail)
{
    //Arrange

    //Act
    UNIQUEID_RESULT result = UniqueId_Generate(NULL, BUFFER_SIZE);

    //Assert
    ASSERT_ARE_EQUAL(UNIQUEID_RESULT, UNIQUEID_INVALID_ARG, result);
}

/* Tests_SRS_UNIQUEID_07_003: [If len is less then 37 then UniqueId_Generate shall return UNIQUEID_INVALID_ARG] */
TEST_FUNCTION(UniqueId_Generate_Len_too_small_Fail)
{
    //Arrange
    char uid[BUFFER_SIZE];

    //Act
    UNIQUEID_RESULT result = UniqueId_Generate(uid, BUFFER_SIZE/2);

    //Assert
    ASSERT_ARE_EQUAL(UNIQUEID_RESULT, UNIQUEID_INVALID_ARG, result);
}

/* Tests_SRS_UNIQUEID_07_001: [UniqueId_Generate shall create a unique Id 36 character long string.] */
TEST_FUNCTION(UniqueId_Generate_Succeed)
{
    //Arrange
    char uid[BUFFER_SIZE];

    STRICT_EXPECTED_CALL(mocked_UuidCreate(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_UuidToStringA(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_RpcStringFreeA(IGNORED_ARG));

    //Act
    UNIQUEID_RESULT result = UniqueId_Generate(uid, BUFFER_SIZE);

    //Assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(UNIQUEID_RESULT, UNIQUEID_OK, result);
    ASSERT_ARE_EQUAL(size_t, 36, strlen(uid) );
}

/* Tests_SRS_UNIQUEID_07_004: [ If there is a failure for any reason the UniqueId_Generate shall return UNIQUEID_ERROR ] */
TEST_FUNCTION(when_underlying_calls_fail_UniqueId_Generate_fails)
{
    //Arrange
    char uid[BUFFER_SIZE];

    STRICT_EXPECTED_CALL(mocked_UuidCreate(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_UuidToStringA(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_RpcStringFreeA(IGNORED_ARG))
        .CallCannotFail();

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            //Act
            UNIQUEID_RESULT result = UniqueId_Generate(uid, BUFFER_SIZE);

            ///assert
            ASSERT_ARE_EQUAL(UNIQUEID_RESULT, UNIQUEID_ERROR, result, "Negative test %zu", i);
        }
    }
}

END_TEST_SUITE(uniqueid_unittests)
