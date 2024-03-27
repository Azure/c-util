// Copyright (c) Microsoft. All rights reserved.

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"

#include "c_util/doublylinkedlist.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_doublylinkedlist.h"
#include "real_srw_lock.h"

#include "c_util/object_lifetime_tracker.h"


MOCK_FUNCTION_WITH_CODE(, void, test_destroy_object, void*, object, const void*, context);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, void, test_destroy_object_2, void*, object, const void*, context);
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, KEY_MATCH_FUNCTION_RESULT, test_key_match_function, const void*, lhs, const void*, rhs)
MOCK_FUNCTION_END((lhs == rhs) ? KEY_MATCH_FUNCTION_RESULT_MATCHING : KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING);

MOCK_FUNCTION_WITH_CODE(, OBJECT_MATCH_FUNCTION_RESULT, test_object_match_function, const void*, lhs, const void*, rhs)
MOCK_FUNCTION_END((lhs == rhs) ? OBJECT_MATCH_FUNCTION_RESULT_MATCHING : OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING);

MOCK_FUNCTION_WITH_CODE(, int, test_action_function, void*, object, void*, context)
MOCK_FUNCTION_END(0);

static void* test_key_1 = (void*)0x1003;
static void* test_key_2 = (void*)0x1004;
static void* test_object_1 = (void*)0x1005;
static void* test_object_2 = (void*)0x1006;
static void* test_object_3 = (void*)0x1007;
static void* test_object_4 = (void*)0x1008;
static const void* test_destroy_context = (void*)0x1009;
static const void* test_destroy_context_2 = (void*)0x100A;
static void* test_context = (void*)0x100B;

TEST_DEFINE_ENUM_TYPE(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT_VALUES);

TEST_DEFINE_ENUM_TYPE(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_RESULT_VALUES);

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void setup_object_lifetime_tracker_create_expectations(PDLIST_ENTRY* captured_list)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_create(false, IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG))
        .CaptureArgumentValue_listHead(captured_list);
}

static OBJECT_LIFETIME_TRACKER_HANDLE test_create_object_lifetime_tracker(void)
{
    PDLIST_ENTRY captured_list;
    setup_object_lifetime_tracker_create_expectations(&captured_list);
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = object_lifetime_tracker_create(test_key_match_function, test_object_match_function);
    ASSERT_IS_NOT_NULL(object_lifetime_tracker);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    return object_lifetime_tracker;
}

static void setup_object_lifetime_tracker_register_object_expectations(size_t num_keys_before, bool is_new_key, size_t num_objects_before, bool is_new_object)
{

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (size_t i = 0; i < num_keys_before; i++)
    {
        STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, IGNORED_ARG));
    }
    if (is_new_key)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(DList_InitializeListHead(IGNORED_ARG));
        STRICT_EXPECTED_CALL(DList_InsertHeadList(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (size_t i = 0; i < num_objects_before; i++)
    {
        STRICT_EXPECTED_CALL(test_object_match_function(IGNORED_ARG, IGNORED_ARG));
    }
    if (is_new_object)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(DList_InsertHeadList(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
}

static void setup_object_lifetime_tracker_unregister_object_expectations(const void* key, const void* object, size_t num_keys_before, size_t num_objects_before, bool is_last_object)
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (size_t i = 0; i < num_keys_before; i++)
    {
        STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, key));
    }
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (size_t i = 0; i < num_objects_before; i++)
    {
        STRICT_EXPECTED_CALL(test_object_match_function(IGNORED_ARG, object));
    }
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_ARG))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    if (is_last_object)
    {
        STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_ARG))
            .CallCannotFail();
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
}

static void setup_object_lifetime_tracker_act_expectations(const void* key, void* object, size_t num_keys_before, size_t num_objects_before)
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (size_t i = 0; i <= num_keys_before; i++)
    {
        STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, key));
    }
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (size_t i = 0; i <= num_objects_before; i++)
    {
        STRICT_EXPECTED_CALL(test_object_match_function(IGNORED_ARG, object));
    }
    STRICT_EXPECTED_CALL(test_action_function(object, test_context));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));
}

/*
Set up expectations for object_lifetime_tracker_destroy_all_objects_for_key.

Assume that the first object to be registered for the key was registered with test_destroy_object as the DESTROY_OBJECT function.
The subsequent objects were registered with test_destroy_object_2 as the DESTROY_OBJECT function.
*/
static void setup_object_lifetime_tracker_destroy_all_objects_for_key_expectations(const void* key, size_t num_keys_before, size_t num_objects)
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    for (size_t i = 0; i < num_keys_before; i++)
    {
        STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, key));
    }
    for (size_t i = 0; i < num_objects - 1; i++)
    {
        STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_destroy_object_2(IGNORED_ARG, test_destroy_context_2));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    if (num_objects > 0)
    {
        STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_destroy_object(IGNORED_ARG, test_destroy_context));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(DList_RemoveHeadList(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_RemoveEntryList(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(DList_ForEach, MU_FAILURE);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_create, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(test_key_match_function, KEY_MATCH_FUNCTION_RESULT_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(test_object_match_function, OBJECT_MATCH_FUNCTION_RESULT_ERROR);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(test_action_function, MU_FAILURE);

    REGISTER_UMOCK_ALIAS_TYPE(OBJECT_LIFETIME_TRACKER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const PDLIST_ENTRY, void*);
    REGISTER_UMOCK_ALIAS_TYPE(DLIST_ACTION_FUNCTION, void*);

    REGISTER_TYPE(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT);
    REGISTER_TYPE(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_RESULT);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_negative_tests_init();
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

//
// object_lifetime_tracker_create
//

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_001: [ If key_match_function is NULL, object_lifetime_tracker_create shall fail and return NULL. ]*/
TEST_FUNCTION(object_lifetime_tracker_create_with_NULL_key_match_function_fails)
{
    // arrange

    // act
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = object_lifetime_tracker_create(NULL, test_object_match_function);

    // assert
    ASSERT_IS_NULL(object_lifetime_tracker);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_067: [ If object_match_function is NULL, object_lifetime_create shall fail and return NULL ]*/
TEST_FUNCTION(object_lifetime_tracker_create_with_NULL_object_match_function_fails)
{
    // arrange

    // act
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = object_lifetime_tracker_create(test_key_match_function, NULL);

    // assert
    ASSERT_IS_NULL(object_lifetime_tracker);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_002: [ object_lifetime_tracker_create shall allocate memory for the object lifetime tracker. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_003: [ object_lifetime_tracker_create shall call srw_lock_create. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_004: [ object_lifetime_tracker_create shall call DList_InitializeListHead to initialize a DList for storing keys. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_005: [ object_lifetime_tracker_create shall succeed and return the created object lifetime tracker. ]*/
TEST_FUNCTION(object_lifetime_tracker_create_with_succeeds)
{
    // arrange
    PDLIST_ENTRY captured_list;
    setup_object_lifetime_tracker_create_expectations(&captured_list);

    // act
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = object_lifetime_tracker_create(test_key_match_function, test_object_match_function);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_IS_NOT_NULL(object_lifetime_tracker);

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_006: [ If there are any failures, object_lifetime_tracker_create shall fail and return NULL. ]*/
TEST_FUNCTION(object_lifetime_tracker_create_fails_when_underlying_functions_fail)
{
    // arrange
    PDLIST_ENTRY captured_list;
    setup_object_lifetime_tracker_create_expectations(&captured_list);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = object_lifetime_tracker_create(test_key_match_function, test_object_match_function);

            // assert
            ASSERT_IS_NULL(object_lifetime_tracker, "On failed call %zu", i);
        }
    }
}

//
// object_lifetime_tracker_destroy
//

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_007: [ If object_lifetime_tracker is NULL, object_lifetime_tracker_destroy shall return. ]*/
TEST_FUNCTION(object_lifetime_tracker_destroy_with_NULL_returns)
{
    // arrange

    // act
    object_lifetime_tracker_destroy(NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_010: [ object_lifetime_tracker_destroy shall call srw_lock_destroy. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_011: [ object_lifetime_tracker_destroy shall free the memory for the object lifetime tracker. ]*/
TEST_FUNCTION(object_lifetime_tracker_destroy_succeeds)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    STRICT_EXPECTED_CALL(srw_lock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(object_lifetime_tracker));

    // act
    object_lifetime_tracker_destroy(object_lifetime_tracker);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// object_lifetime_tracker_register_object
//

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_012 : [ If object_lifetime_tracker is NULL, object_lifetime_tracker_register_object shall fail and return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_with_NULL_handle_fails)
{
    // arrange

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(NULL, test_key_1, test_object_1, test_destroy_object, test_destroy_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_014: [ If object is NULL, object_lifetime_tracker_register_object shall fail and return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_with_NULL_object_fails)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, NULL, test_destroy_object, test_destroy_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_015: [ If destroy_object is NULL, object_lifetime_tracker_register_object shall fail and return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_with_NULL_destroy_object_fails)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, NULL, test_destroy_context);

    // assert
    ASSERT_ARE_NOT_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_016: [ object_lifetime_tracker_register_object shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_017: [ object_lifetime_tracker_register_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_045: [ If the key is not found in the DList of keys: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_043: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_044: [ object_lifetime_tracker_register_object shall initialize a DList to store objects associated with the key by calling DList_InitializeListHead. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_018: [ object_lifetime_tracker_register_object shall add the given key to the DList of keys by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_076: [ object_lifetime_tracker_register_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_077: [ If the object is not found in the DList of objects:]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_060: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_019: [ object_lifetime_tracker_register_object shall store the given object and the given destroy_object in the DList of objects for given key by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_078: [ If the given key is not found, object_lifetime_tracker_registeer_object shall return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_with_NULL_destroy_context_succeeds)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, NULL);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_016: [ object_lifetime_tracker_register_object shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_017: [ object_lifetime_tracker_register_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_045: [ If the key is not found in the DList of keys: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_043: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_044: [ object_lifetime_tracker_register_object shall initialize a DList to store objects associated with the key by calling DList_InitializeListHead. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_018: [ object_lifetime_tracker_register_object shall add the given key to the DList of keys by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_076: [ object_lifetime_tracker_register_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_077: [ If the object is not found in the DList of objects:]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_060: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_019: [ object_lifetime_tracker_register_object shall store the given object and the given destroy_object in the DList of objects for given key by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_078: [ If the given key is not found, object_lifetime_tracker_registeer_object shall return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY. ]*/

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_051: [ object_lifetime_tracker_register_object shall release the lock. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_succeeds_for_new_key)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_016: [ object_lifetime_tracker_register_object shall acquire the lock in exclusive mode. ]*/

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_017: [ object_lifetime_tracker_register_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_045: [ If the key is not found in the DList of keys: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_043: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_044: [ object_lifetime_tracker_register_object shall initialize a DList to store objects associated with the key by calling DList_InitializeListHead. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_018: [ object_lifetime_tracker_register_object shall add the given key to the DList of keys by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_076: [ object_lifetime_tracker_register_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_077: [ If the object is not found in the DList of objects:]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_060: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_019: [ object_lifetime_tracker_register_object shall store the given object and the given destroy_object in the DList of objects for given key by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_078: [ If the given key is not found, object_lifetime_tracker_registeer_object shall return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_051: [ object_lifetime_tracker_register_object shall release the lock. ]*/

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_050: [ is_same_key shall call key_match_function on the obtained key from listEntry and the key in key_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_057: [ If key_match_function returns KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING, is_same_key shall set continueProcessing to true. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_058: [ is_same_key shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_succeeds_for_different_keys_with_different_objects)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_2, test_destroy_object, test_destroy_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_2, test_object_2));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_016: [ object_lifetime_tracker_register_object shall acquire the lock in exclusive mode. ]*/

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_017: [ object_lifetime_tracker_register_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_045: [ If the key is not found in the DList of keys: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_043: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_044: [ object_lifetime_tracker_register_object shall initialize a DList to store objects associated with the key by calling DList_InitializeListHead. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_018: [ object_lifetime_tracker_register_object shall add the given key to the DList of keys by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_076: [ object_lifetime_tracker_register_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_077: [ If the object is not found in the DList of objects:]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_060: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_019: [ object_lifetime_tracker_register_object shall store the given object and the given destroy_object in the DList of objects for given key by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_079: [ If the given key is found but the given object is not found, object_lifetime_tracker_register_object shall return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_051: [ object_lifetime_tracker_register_object shall release the lock. ]*/

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_050: [ is_same_key shall call key_match_function on the obtained key from listEntry and the key in key_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_054: [ If key_match_function returns KEY_MATCH_FUNCTION_RESULT_MATCHING: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_055: [ is_same_key shall set continueProcessing to false. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_056: [ is_same_key shall store listEntry in key_match_context ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_058: [ is_same_key shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_succeeds_for_repeated_key_with_different_objects)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object, test_destroy_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_2));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_016: [ object_lifetime_tracker_register_object shall acquire the lock in exclusive mode. ]*/

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_017: [ object_lifetime_tracker_register_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_045: [ If the key is not found in the DList of keys: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_043: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_044: [ object_lifetime_tracker_register_object shall initialize a DList to store objects associated with the key by calling DList_InitializeListHead. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_018: [ object_lifetime_tracker_register_object shall add the given key to the DList of keys by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_076: [ object_lifetime_tracker_register_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_080: [ If the object is found in the DList of objects, object_lifetime_tracker_register_object shall return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_EXISTS.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_051: [ object_lifetime_tracker_register_object shall release the lock. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_050: [ is_same_key shall call key_match_function on the obtained key from listEntry and the key in key_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_054: [ If key_match_function returns KEY_MATCH_FUNCTION_RESULT_MATCHING: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_055: [ is_same_key shall set continueProcessing to false. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_056: [ is_same_key shall store listEntry in key_match_context ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_058: [ is_same_key shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_succeeds_for_repeated_key_with_repeated_object)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, false);

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_EXISTS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_KEY_NOT_FOUND, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_016: [ object_lifetime_tracker_register_object shall acquire the lock in exclusive mode. ]*/

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_017: [ object_lifetime_tracker_register_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_045: [ If the key is not found in the DList of keys: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_043: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_044: [ object_lifetime_tracker_register_object shall initialize a DList to store objects associated with the key by calling DList_InitializeListHead. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_018: [ object_lifetime_tracker_register_object shall add the given key to the DList of keys by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_076: [ object_lifetime_tracker_register_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_077: [ If the object is not found in the DList of objects:]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_060: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_019: [ object_lifetime_tracker_register_object shall store the given object and the given destroy_object in the DList of objects for given key by calling DList_InsertHeadList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_078: [ If the given key is not found, object_lifetime_tracker_registeer_object shall return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_051: [ object_lifetime_tracker_register_object shall release the lock. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_050: [ is_same_key shall call key_match_function on the obtained key from listEntry and the key in key_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_057: [ If key_match_function returns KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING, is_same_key shall set continueProcessing to true. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_058: [ is_same_key shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_succeeds_for_different_keys_with_repeated_object)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);

    // act
    OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_1, test_destroy_object, test_destroy_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_2, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_021: [ If there are any failures, object_lifetime_tracker_register_object shall fail and return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_059: [ If there are any failures, is_same_key shall fail and return a non-zero value. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_fails_when_underlying_functions_fail_when_no_previous_key_exists)
{
    // arrange (no previous key)
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context);

            // assert
            ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR, result);

        }
    }

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_021: [ If there are any failures, object_lifetime_tracker_register_object shall fail and return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_059: [ If there are any failures, is_same_key shall fail and return a non-zero value. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_fails_when_underlying_functions_fail_while_registering_with_previously_registered_key)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object, test_destroy_context);

            // assert
            ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR, result);

        }
    }

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_021: [ If there are any failures, object_lifetime_tracker_register_object shall fail and return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_059: [ If there are any failures, is_same_key shall fail and return a non-zero value. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_fails_when_underlying_functions_fail_while_registering_previously_registered_key_with_previously_regiestered_object)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, false);
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context);

            // assert
            ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR, result);

        }
    }

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_021: [ If there are any failures, object_lifetime_tracker_register_object shall fail and return OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_059: [ If there are any failures, is_same_key shall fail and return a non-zero value. ]*/
TEST_FUNCTION(object_lifetime_tracker_register_object_fails_when_underlying_functions_fail_while_registering_object_with_new_key_when_previous_key_exists)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT result = object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_2, test_destroy_object, test_destroy_context);

            // assert
            ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_ERROR, result);

        }
    }

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

//
// object_lifetime_tracker_unregister_object
//

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_022: [ If object_lifetime_tracker is NULL, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_with_NULL_handle_fails)
{
    // arrange

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result = object_lifetime_tracker_unregister_object(NULL, test_key_1, test_object_1);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR, result);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_024: [ If object is NULL, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_with_NULL_object_fails)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, NULL);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR, result);

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_025: [ object_lifetime_tracker_unregister_object shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_026: [ object_lifetime_tracker_unregister_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_027: [ object_lifetime_tracker_unregister_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_029: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given object from the DList of objects for the given key by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_061: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_030: [ If the DList of objects for the given key is empty: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_062: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_063: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_052: [ object_lifetime_tracker_unregister_object shall release the lock. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_031: [ object_lifetime_tracker_unregister_object shall succeed and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_068: [ is_same_object shall call object_match_function on the obtained object from listEntry and the object in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_069: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_MATCHING: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_070: [ is_same_object shall set continueProcessing to false. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_071: [ is_same_object shall store listEntry in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_succeeds_for_1_key_with_1_object)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_1, 1, 1, true);
    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_025: [ object_lifetime_tracker_unregister_object shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_026: [ object_lifetime_tracker_unregister_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_027: [ object_lifetime_tracker_unregister_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_029: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given object from the DList of objects for the given key by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_061: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_030: [ If the DList of objects for the given key is empty: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_062: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_063: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_052: [ object_lifetime_tracker_unregister_object shall release the lock. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_031: [ object_lifetime_tracker_unregister_object shall succeed and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_068: [ is_same_object shall call object_match_function on the obtained object from listEntry and the object in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_069: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_MATCHING: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_070: [ is_same_object shall set continueProcessing to false. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_071: [ is_same_object shall store listEntry in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_072: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING, is_same_object shall set continueProcessing to true. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_succeeds_for_1_key_with_2_objects)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_1, 1, 2, false);
    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_2, 1, 1, true);

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_1 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_2 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_2);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_1);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_025: [ object_lifetime_tracker_unregister_object shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_026: [ object_lifetime_tracker_unregister_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_027: [ object_lifetime_tracker_unregister_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_029: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given object from the DList of objects for the given key by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_061: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_030: [ If the DList of objects for the given key is empty: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_062: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_063: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_052: [ object_lifetime_tracker_unregister_object shall release the lock. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_031: [ object_lifetime_tracker_unregister_object shall succeed and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_068: [ is_same_object shall call object_match_function on the obtained object from listEntry and the object in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_069: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_MATCHING: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_070: [ is_same_object shall set continueProcessing to false. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_071: [ is_same_object shall store listEntry in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_succeeds_for_1_key_with_2_objects_reverse_order)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_2, 1, 1, false);
    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_1, 1, 1, true);

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_1 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_2);
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_2 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_1);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_025: [ object_lifetime_tracker_unregister_object shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_026: [ object_lifetime_tracker_unregister_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_027: [ object_lifetime_tracker_unregister_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_029: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given object from the DList of objects for the given key by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_061: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_030: [ If the DList of objects for the given key is empty: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_062: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_063: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_052: [ object_lifetime_tracker_unregister_object shall release the lock. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_031: [ object_lifetime_tracker_unregister_object shall succeed and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_068: [ is_same_object shall call object_match_function on the obtained object from listEntry and the object in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_069: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_MATCHING: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_070: [ is_same_object shall set continueProcessing to false. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_071: [ is_same_object shall store listEntry in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_072: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING, is_same_object shall set continueProcessing to true. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_succeeds_for_2_keys_with_1_object_each)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_2, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_1, 2, 1, true);
    setup_object_lifetime_tracker_unregister_object_expectations(test_key_2, test_object_2, 1, 1, true);

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_1 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_2 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_2, test_object_2);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_1);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_025: [ object_lifetime_tracker_unregister_object shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_026: [ object_lifetime_tracker_unregister_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_027: [ object_lifetime_tracker_unregister_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_029: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given object from the DList of objects for the given key by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_061: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given object. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_030: [ If the DList of objects for the given key is empty: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_062: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_063: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_052: [ object_lifetime_tracker_unregister_object shall release the lock. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_031: [ object_lifetime_tracker_unregister_object shall succeed and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_068: [ is_same_object shall call object_match_function on the obtained object from listEntry and the object in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_069: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_MATCHING: ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_070: [ is_same_object shall set continueProcessing to false. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_071: [ is_same_object shall store listEntry in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_succeeds_for_2_keys_with_1_object_each_reverse_order)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_2, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_unregister_object_expectations(test_key_2, test_object_2, 1, 1, true);
    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_1, 1, 1, true);

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_1 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_2, test_object_2);
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result_2 = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_1);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, result_2);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_046: [ If the given key is not found, object_lifetime_tracker_unregister_object shall return OBJECT_LIFETIME_TRACKER_UNREGISTER_KEY_NOT_FOUND. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_key_not_found)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, test_key_2));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_2, test_object_2);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_KEY_NOT_FOUND, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_047: [ If the given object is not found, object_lifetime_tracker_unregister_object shall return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_NOT_FOUND. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_068: [ is_same_object shall call object_match_function on the obtained object from listEntry and the object in object_match_context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_072: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING, is_same_object shall set continueProcessing to true. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_key_found_object_not_found)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, test_key_1));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_object_match_function(IGNORED_ARG, test_object_2));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    // act
    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_2);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_NOT_FOUND, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_032: [ If there are any failures, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_074: [ If there are any failures, is_same_object shall fail and return a non-zero value. ]*/
TEST_FUNCTION(object_lifetime_tracker_unregister_object_fails_when_underlying_functions_fail)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_unregister_object_expectations(test_key_1, test_object_1, 1, 1, true);
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result = object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1);

            // assert
            ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR, result);
        }
    }

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

//
// object_lifetime_tracker_destroy_all_objects_for_key
//

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_033: [ If object_lifetime_tracker is NULL, object_lifetime_tracker_destroy_all_objects_for_key shall return. ]*/
TEST_FUNCTION(object_lifetime_tracker_destroy_all_objects_for_key_with_NULL_handle_fails)
{
    // arrange

    // act
    object_lifetime_tracker_destroy_all_objects_for_key(NULL, test_key_1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_035: [ object_lifetime_tracker_destroy_all_objects_for_key shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_036: [ object_lifetime_tracker_destroy_all_objects_for_key shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_038: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entries for all the objects in the DList of objects for the given key by calling DList_RemoveHeadList for each list entry. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_037: [ object_lifetime_tracker_destroy_all_objects_for_key shall destroy all the objects in the DList of objects for the given key in the reverse order in which they were registered by calling destroy_object with destroy_context as context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_066: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with all the objects. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_039: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_065: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_053: [ object_lifetime_tracker_destroy_all_objects_for_key shall release the lock. ]*/
TEST_FUNCTION(object_lifetime_tracker_destroy_all_objects_for_key_succeeds_for_1_key_with_2_objects)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object_2, test_destroy_context_2));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    setup_object_lifetime_tracker_destroy_all_objects_for_key_expectations(test_key_1, 1, 2);

    // act
    object_lifetime_tracker_destroy_all_objects_for_key(object_lifetime_tracker, test_key_1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_035: [ object_lifetime_tracker_destroy_all_objects_for_key shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_036: [ object_lifetime_tracker_destroy_all_objects_for_key shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_038: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entries for all the objects in the DList of objects for the given key by calling DList_RemoveHeadList for each list entry. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_037: [ object_lifetime_tracker_destroy_all_objects_for_key shall destroy all the objects in the DList of objects for the given key in the reverse order in which they were registered by calling destroy_object with destroy_context as context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_066: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with all the objects. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_039: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_065: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_053: [ object_lifetime_tracker_destroy_all_objects_for_key shall release the lock. ]*/
TEST_FUNCTION(object_lifetime_tracker_destroy_all_objects_for_key_succeeds_for_2_keys_with_2_objects_each)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object_2, test_destroy_context_2));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_3, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_4, test_destroy_object_2, test_destroy_context_2));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    setup_object_lifetime_tracker_destroy_all_objects_for_key_expectations(test_key_1, 2, 2);
    setup_object_lifetime_tracker_destroy_all_objects_for_key_expectations(test_key_2, 1, 2);

    // act
    object_lifetime_tracker_destroy_all_objects_for_key(object_lifetime_tracker, test_key_1);
    object_lifetime_tracker_destroy_all_objects_for_key(object_lifetime_tracker, test_key_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_035: [ object_lifetime_tracker_destroy_all_objects_for_key shall acquire the lock in exclusive mode. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_036: [ object_lifetime_tracker_destroy_all_objects_for_key shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_038: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entries for all the objects in the DList of objects for the given key by calling DList_RemoveHeadList for each list entry. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_037: [ object_lifetime_tracker_destroy_all_objects_for_key shall destroy all the objects in the DList of objects for the given key in the reverse order in which they were registered by calling destroy_object with destroy_context as context. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_066: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with all the objects. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_039: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_065: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with the given key. ]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_053: [ object_lifetime_tracker_destroy_all_objects_for_key shall release the lock. ]*/
TEST_FUNCTION(object_lifetime_tracker_destroy_all_objects_for_key_succeeds_for_2_keys_with_2_objects_each_reverse_order)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object_2, test_destroy_context_2));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_3, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_4, test_destroy_object_2, test_destroy_context_2));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    setup_object_lifetime_tracker_destroy_all_objects_for_key_expectations(test_key_2, 1, 2);
    setup_object_lifetime_tracker_destroy_all_objects_for_key_expectations(test_key_1, 1, 2);

    // act
    object_lifetime_tracker_destroy_all_objects_for_key(object_lifetime_tracker, test_key_2);
    object_lifetime_tracker_destroy_all_objects_for_key(object_lifetime_tracker, test_key_1);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_048: [ If the given key is not found, object_lifetime_tracker_destroy_all_objects_for_key shall return. ]*/
TEST_FUNCTION(object_lifetime_tracker_destroy_all_objects_for_key_key_not_found)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_2, test_destroy_object_2, test_destroy_context_2));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, test_key_2));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(IGNORED_ARG));

    // act
    object_lifetime_tracker_destroy_all_objects_for_key(object_lifetime_tracker, test_key_2);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy_all_objects_for_key(object_lifetime_tracker, test_key_1);
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/* object_lifetime_tracker_act */

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_081: [ If object_lifetime_tracker is NULL, object_lifetime_tracker_act shall fail and return OBJECT_LIFETIME_TRACKER_ACT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_act_with_NULL_handle_fails)
{
    // arrange

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(NULL, test_key_1, test_object_1, test_action_function, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_082: [ If key is NULL, object_lifetime_tracker_act shall fail and return OBJECT_LIFETIME_TRACKER_ACT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_act_with_NULL_key_fails)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, NULL, test_object_1, test_action_function, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_083: [ If object is NULL, object_lifetime_tracker_act shall fail and return OBJECT_LIFETIME_TRACKER_ACT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_act_with_NULL_OBJECT_FAILS)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, test_key_1, NULL, test_action_function, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_084: [ If action_function is NULL, object_lifetime_tracker_act shall fail and return OBJECT_LIFETIME_TRACKER_ACT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_act_with_NULL_action_function_fails)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, test_key_1, test_object_1, NULL, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_ERROR, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_087: [If the given key is not found, object_lifetime_tracker_act shall return OBJECT_LIFETIME_TRACKER_ACT_KEY_NOT_FOUND.]*/
TEST_FUNCTION(object_lifetime_tracker_act_with_key_not_found_fails)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, test_key_1, test_object_1, test_action_function, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_KEY_NOT_FOUND, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_089: [If the given object is not found, object_lifetime_tracker_act shall return OBJECT_LIFETIME_TRACKER_ACT_OBJECT_NOT_FOUND.]*/
TEST_FUNCTION(object_lifetime_tracker_act_with_object_not_found_fails)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));

    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(srw_lock_acquire_shared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_key_match_function(IGNORED_ARG, test_key_1));
    STRICT_EXPECTED_CALL(DList_ForEach(IGNORED_ARG, IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_object_match_function(IGNORED_ARG, test_object_2));
    STRICT_EXPECTED_CALL(srw_lock_release_shared(IGNORED_ARG));

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, test_key_1, test_object_2, test_action_function, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_OBJECT_NOT_FOUND, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_085: [object_lifetime_tracker_act shall acquire the lock in shared mode.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_086 : [object_lifetime_tracker_act shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_088 : [object_lifetime_tracker_act shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_090 : [object_lifetime_tracker_act shall call action_function with the given object and context.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_091 : [object_lifetime_tracker_act shall release the lock.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_092 : [object_lifetime_tracker_act shall succeed and return OBJECT_LIFETIME_TRACKER_ACT_OK.]*/
TEST_FUNCTION(object_lifetime_tracker_act_succeeds_for_1_key_with_1_object)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_act_expectations(test_key_1, test_object_1, 0, 0);

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, test_key_1, test_object_1, test_action_function, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_085: [object_lifetime_tracker_act shall acquire the lock in shared mode.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_086 : [object_lifetime_tracker_act shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_088 : [object_lifetime_tracker_act shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_090 : [object_lifetime_tracker_act shall call action_function with the given object and context.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_091 : [object_lifetime_tracker_act shall release the lock.]*/
/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_092 : [object_lifetime_tracker_act shall succeed and return OBJECT_LIFETIME_TRACKER_ACT_OK.]*/
TEST_FUNCTION(object_lifetime_tracker_act_succeeds_for_2_keys_and_2_objects)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, true, 0, true);
    setup_object_lifetime_tracker_register_object_expectations(1, false, 1, true);

    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_2, test_destroy_object, test_destroy_context));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_OBJECT, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_2, test_object_3, test_destroy_object, test_destroy_context));

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    setup_object_lifetime_tracker_act_expectations(test_key_2, test_object_3, 0, 0);

    // act
    OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, test_key_2, test_object_3, test_action_function, test_context);

    // assert
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_2, test_object_3));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_2, test_object_2));
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}


/*Tests_SRS_OBJECT_LIFETIME_TRACKER_43_093: [ If there are any failures, object_lifetime_tracker_act shall fail and return OBJECT_LIFETIME_TRACKER_ACT_ERROR. ]*/
TEST_FUNCTION(object_lifetime_tracker_act_fails_when_underlying_functions_fail)
{
    // arrange
    OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker = test_create_object_lifetime_tracker();
    setup_object_lifetime_tracker_register_object_expectations(0, true, 0, true);
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_NEW_KEY, object_lifetime_tracker_register_object(object_lifetime_tracker, test_key_1, test_object_1, test_destroy_object, test_destroy_context));

    umock_c_reset_all_calls();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            OBJECT_LIFETIME_TRACKER_ACT_RESULT result = object_lifetime_tracker_act(object_lifetime_tracker, test_key_1, test_object_1, test_action_function, test_context);

            // assert
            ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_ACT_RESULT, OBJECT_LIFETIME_TRACKER_ACT_ERROR, result);
        }
    }

    // cleanup
    ASSERT_ARE_EQUAL(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK, object_lifetime_tracker_unregister_object(object_lifetime_tracker, test_key_1, test_object_1));
    object_lifetime_tracker_destroy(object_lifetime_tracker);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
