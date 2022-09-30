// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"


#include "c_util/slist.h"
#include "c_util/containing_record.h"
#include "testrunnerswitcher.h"

typedef struct simpleItem_tag
{
    unsigned char index;
    SINGLYLINKEDLIST_ENTRY link;
} simpleItem, * pSimpleItem;

#define TEST_CONTEXT ((const void*)0x0101)

#define ENABLE_MOCKS

MOCK_FUNCTION_WITH_CODE(, bool, test_match_function, PSINGLYLINKEDLIST_ENTRY, list_entry, const void*, match_context)
MOCK_FUNCTION_END(true);

MOCK_FUNCTION_WITH_CODE(, bool, test_condition_function, PSINGLYLINKEDLIST_ENTRY, list_entry, const void*, action_context, bool*, continueProcessing);
*continueProcessing = true;
MOCK_FUNCTION_END(true);

MOCK_FUNCTION_WITH_CODE(, bool, test_condition_function_2, PSINGLYLINKEDLIST_ENTRY, list_entry, const void*, action_context, bool*, continueProcessing);
simpleItem* item = CONTAINING_RECORD(list_entry, simpleItem, link);
*continueProcessing = item->index != 3;
MOCK_FUNCTION_END(true);

MOCK_FUNCTION_WITH_CODE(, int, test_action_function, PSINGLYLINKEDLIST_ENTRY, list_entry, const void*, action_context, bool*, continueProcessing);
*continueProcessing = true;
MOCK_FUNCTION_END(0);

MOCK_FUNCTION_WITH_CODE(, int, test_action_function_2, PSINGLYLINKEDLIST_ENTRY, list_entry, const void*, action_context, bool*, continueProcessing);
simpleItem* item = CONTAINING_RECORD(list_entry, simpleItem, link);
*continueProcessing = item->index != 3;
MOCK_FUNCTION_END(0);

MOCK_FUNCTION_WITH_CODE(, int, test_action_function_fail, PSINGLYLINKEDLIST_ENTRY, list_entry, const void*, action_context, bool*, continueProcessing);
*continueProcessing = true;
simpleItem* item = CONTAINING_RECORD(list_entry, simpleItem, link);
(int)item->index;
if (item->index == 3)
{
    return MU_FAILURE;
}
return 0;
MOCK_FUNCTION_END();

#undef ENABLE_MOCKS

static TEST_MUTEX_HANDLE test_serialize_mutex;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    int result;

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    umock_c_init(on_umock_c_error);

    result = umocktypes_bool_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    result = umocktypes_charptr_register_types();
    ASSERT_ARE_EQUAL(int, 0, result);

    REGISTER_UMOCK_ALIAS_TYPE(PSINGLYLINKEDLIST_ENTRY, void*);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_reset_all_calls();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/*slist_initialize*/

/* Tests_SRS_SLIST_07_001: [ If `list_head` is `NULL`, `slist_initialize` shall fail and return `false`. ]*/
TEST_FUNCTION(slist_initialize_with_NULL_fails)
{
    // arrange

    // act
    bool result = slist_initialize(NULL);

    // assert
    ASSERT_ARE_EQUAL(bool, false, result);
}

/* Tests_SRS_SLIST_07_002: [ `slist_initialize` shall initialize the `next` pointer in `list_head` points to `NULL`and return `true` on success. ]*/
TEST_FUNCTION(slist_initialize_succeeds)
{
    // arrange
    SINGLYLINKEDLIST_ENTRY head;

    // act
    bool result = slist_initialize(&head);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, head.next);
    ASSERT_ARE_EQUAL(bool, true, result);
}

/*slist_is_empty*/

/* Tests_SRS_SLIST_07_038: [ If `list_head` is `NULL`, `slist_is_empty` shall fail and return `true`. ]*/
TEST_FUNCTION(slist_is_empty_with_NULL_list_fails)
{
    // arrange

    // act
    bool result = slist_is_empty(NULL);

    // assert
    ASSERT_ARE_EQUAL(bool, true, result);
}

/* Tests_SRS_SLIST_07_003: [ `slist_is_empty` shall return `true` if there is no `SINGLYLINKEDLIST_ENTRY` in this list. ]*/
TEST_FUNCTION(slist_is_empty_with_empty_list_succeeds)
{
    // arrange
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    // act
    bool result = slist_is_empty(&head);

    // assert
    ASSERT_ARE_EQUAL(bool, true, result);
}

/* Tests_SRS_SLIST_07_004: [ `slist_is_empty` shall return `false` if there is one or more entris in the list. ]*/
TEST_FUNCTION(slist_is_empty_with_one_entry_succeeds)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    slist_add(&head, &(simp1.link));

    // act
    bool result = slist_is_empty(&head);

    // assert
    ASSERT_ARE_EQUAL(bool, false, result);
}

/* Tests_SRS_SLIST_07_004: [ `slist_is_empty` shall return `false` if there is one or more entris in the list. ]*/
TEST_FUNCTION(slist_is_empty_with_multiple_entries_succeeds)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));

    // act
    bool result = slist_is_empty(&head);

    // assert
    ASSERT_ARE_EQUAL(bool, false, result);
}

/*slist_add*/

/* Tests_SRS_SLIST_07_005: [ If `list_head` is `NULL`, `slist_add` shall fail and return `NULL`. ]*/
TEST_FUNCTION(slist_add_with_head_entry_NULL_fails)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add(NULL, &(simp1.link));

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_SLIST_07_006: [ If `list_entry` is `NULL`, `slist_add` shall fail and return `NULL`. ]*/
TEST_FUNCTION(slist_add_with_add_entry_NULL_fails)
{
    // arrange
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add(&head, NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_SLIST_07_007: [ `slist_add` shall add one entry to the tail of the list on success and return a pointer to the added entry. ]*/
TEST_FUNCTION(slist_add_with_empty_list_succeeds)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
 
    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add(&head, &(simp1.link));

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, &(simp1.link), result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, result);
}

/* Tests_SRS_SLIST_07_007: [ `slist_add` shall add one entry to the tail of the list on success and return a pointer to the added entry. ]*/
TEST_FUNCTION(slist_add_with_multiple_entries_in_the_list_succeeds)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    head.next = &(simp1.link);
    head.next->next = &(simp2.link);
    head.next->next->next = &(simp3.link);

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add(&head, &(simp4.link));

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, &(simp4.link), result);
    ASSERT_ARE_EQUAL(void_ptr, simp3.link.next, result);
}

/*slist_add_head*/

/* Tests_SRS_SLIST_07_008: [ If `list_head` is `NULL`, `slist_add_head` shall fail and return `NULL`. ]*/
TEST_FUNCTION(slist_add_head_with_head_NULL_fails)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
   
    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add_head(NULL, &(simp1.link));

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_SLIST_07_009: [ If `list_entry` is `NULL`, `slist_add_head` shall fail and return `NULL`. ]*/
TEST_FUNCTION(slist_add_head_with_add_entry_NULL_fails)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add_head(&head, NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_SLIST_07_010: [ `slist_add_head` shall insert `list_entry` at head on success and return an entry to the added entry. ]*/
TEST_FUNCTION(slist_add_head_with_empty_list_succeeds)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    
    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add_head(&head, &(simp1.link));

    // assert
    ASSERT_ARE_EQUAL(void_ptr, &(simp1.link), result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, result);
}

/* Tests_SRS_SLIST_07_010: [ `slist_add_head` shall insert `list_entry` at head on success and return an entry to the added entry. ]*/
TEST_FUNCTION(slist_add_head_with_multiple_entries_in_the_list_succeeds)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_add_head(&head, &(simp4.link));

    // assert
    ASSERT_ARE_EQUAL(void_ptr, &(simp4.link), result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, result);
}

/*slist_remove*/

/* Tests_SRS_SLIST_07_011: [ If `list_head` is `NULL`, `slist_remove` shall fail and a non-zero value. ]*/
TEST_FUNCTION(slist_remove_with_head_NULL_fails)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    // act
    int result = slist_remove(NULL, &(simp1.link));

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_SLIST_07_012: [ If `list_entry` is `NULL`, `slist_remove` shall fail and a non-zero value. ]*/
TEST_FUNCTION(slist_remove_with_delete_entry_NULL_fails)
{
    //arrange
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    // act
    int result = slist_remove(&head, NULL);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_SLIST_07_014: [ If the entry `list_entry` is not found in the list, then `slist_remove` shall fail and a non-zero value. ]*/
TEST_FUNCTION(slist_remove_with_empty_list_fails)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    // act
    int result = slist_remove(&head, &(simp1.link));

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_SLIST_07_014: [ If the entry `list_entry` is not found in the list, then `slist_remove` shall fail and a non-zero value. ]*/
TEST_FUNCTION(slist_remove_with_remove_entry_not_in_list_fails)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));

    // act
    int result = slist_remove(&head, &(simp4.link));

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/* Tests_SRS_SLIST_07_013: [ `slist_remove` shall remove a list entry from the list and return zero on success. ]*/
TEST_FUNCTION(slist_remove_with_one_entry_in_list_succeeds)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    // act
    int result = slist_remove(&head, &(simp1.link));

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
}

/* Tests_SRS_SLIST_07_013: [ `slist_remove` shall remove a list entry from the list and return zero on success. ]*/
TEST_FUNCTION(slist_remove_with_multiple_entries_in_list_remove_tail_succeeds)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));
    slist_add(&head, &(simp4.link));

    // act
    int result = slist_remove(&head, &(simp4.link));

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, NULL, simp3.link.next);
}

/* Tests_SRS_SLIST_07_013: [ `slist_remove` shall remove a list entry from the list and return zero on success. ]*/
TEST_FUNCTION(slist_remove_with_multiple_entries_in_list_remove_head_succeeds)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));
    slist_add(&head, &(simp4.link));

    // act
    int result = slist_remove(&head, &(simp1.link));

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, &(simp2.link));
}

/* Tests_SRS_SLIST_07_013: [ `slist_remove` shall remove a list entry from the list and return zero on success. ]*/
TEST_FUNCTION(slist_remove_with_multiple_entries_in_list_remove_middle_succeeds)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));
    slist_add(&head, &(simp4.link));

    // act
    int result = slist_remove(&head, &(simp3.link));

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, simp2.link.next, &(simp4.link));
}

/*slist_remove_head*/

/* Tests_SRS_SLIST_07_015: [ If `list_head` is `NULL`, `slist_remove_head` shall fail and return `NULL`. ]*/
TEST_FUNCTION(slist_remove_head_with_head_NULL_fails)
{
    // arrange

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_remove_head(NULL);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, NULL, result);
}

/* Tests_SRS_SLIST_07_016: [ `slist_remove_head` removes the head entry from the list defined by the `list_head` parameter on success and return a pointer to that entry. ]*/
TEST_FUNCTION(slist_remove_head_with_one_entry_in_list_succeeds)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_remove_head(&head);

    // assert
    ASSERT_IS_NULL(head.next);
    ASSERT_ARE_EQUAL(void_ptr, &(simp1.link), result);
}

/* Tests_SRS_SLIST_07_016: [ `slist_remove_head` removes the head entry from the list defined by the `list_head` parameter on success and return a pointer to that entry. ]*/
TEST_FUNCTION(slist_remove_head_with_multiple_entries_in_list_succeeds)
{
    //arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_remove_head(&head);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, head.next, &(simp2.link));
    ASSERT_ARE_EQUAL(void_ptr, &(simp1.link), result);
}

/* Tests_SRS_SLIST_07_017: [ `slist_remove_head` shall return `list_head` if that's the only entry in the list. ]*/
TEST_FUNCTION(slist_remove_head_with_empty_list_succeeds)
{
    //arrange
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_remove_head(&head);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, &head, result);
}

/*slist_find*/

/* Tests_SRS_SLIST_07_018: [ If `list_head` is `NULL`, `slist_find` shall fail and return `NULL`. ]*/
TEST_FUNCTION(slist_find_with_NULL_list_fails_with_NULL)
{
    //arrange

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(NULL, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_019: [ If `match_function` is `NULL`, `slist_find` shall fail and return `NULL`. ]*/
TEST_FUNCTION(slist_find_with_NULL_match_function_fails_with_NULL)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, NULL, TEST_CONTEXT);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_025: [ If the list is empty, `slist_find` shall return `NULL`. ]*/
TEST_FUNCTION(slist_find_with_empty_list_fails_with_NULL)
{
    //arrange
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_020: [ `slist_find` shall iterate through all entris in the list and return the first entry that satisfies the `match_function` on success. ]*/
/* Tests_SRS_SLIST_07_021: [ `slist_find` shall determine whether an item satisfies the match criteria by invoking the `match_function` for each entry in the list until a matching entry is found. ]*/
/* Tests_SRS_SLIST_07_022: [ The `match_function` shall get as arguments the list entry being attempted to be matched and the `match_context` as is. ]*/
/* Tests_SRS_SLIST_07_024: [ If the `match_function` returns `true`, `slist_find` shall consider that item as matching. ]*/
TEST_FUNCTION(slist_find_on_a_list_with_1_matching_item_yields_that_item)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/* Tests_SRS_SLIST_07_023: [ If the `match_function` returns `false`, `slist_find` shall consider that item as not matching. ]*/
TEST_FUNCTION(slist_find_on_a_list_with_1_items_that_does_not_match_returns_NULL)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT))
        .SetReturn(false);

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_020: [ `slist_find` shall iterate through all entris in the list and return the first entry that satisfies the `match_function` on success. ]*/
/* Tests_SRS_SLIST_07_021: [ `slist_find` shall determine whether an item satisfies the match criteria by invoking the `match_function` for each entry in the list until a matching entry is found. ]*/
/* Tests_SRS_SLIST_07_022: [ The `match_function` shall get as arguments the list entry being attempted to be matched and the `match_context` as is. ]*/
/* Tests_SRS_SLIST_07_024: [ If the `match_function` returns `true`, `slist_find` shall consider that item as matching. ]*/
TEST_FUNCTION(slist_find_on_a_list_with_2_items_where_the_first_matches_yields_the_first_item)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_020: [ `slist_find` shall iterate through all entris in the list and return the first entry that satisfies the `match_function` on success. ]*/
/* Tests_SRS_SLIST_07_021: [ `slist_find` shall determine whether an item satisfies the match criteria by invoking the `match_function` for each entry in the list until a matching entry is found. ]*/
/* Tests_SRS_SLIST_07_022: [ The `match_function` shall get as arguments the list entry being attempted to be matched and the `match_context` as is. ]*/
/* Tests_SRS_SLIST_07_024: [ If the `match_function` returns `true`, `slist_find` shall consider that item as matching. ]*/
/* Tests_SRS_SLIST_07_023: [ If the `match_function` returns `false`, `slist_find` shall consider that item as not matching. ]*/
TEST_FUNCTION(slist_find_on_a_list_with_2_items_where_the_second_matches_yields_the_second_item)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, &(simp2.link), result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_020: [ `slist_find` shall iterate through all entris in the list and return the first entry that satisfies the `match_function` on success. ]*/
TEST_FUNCTION(slist_find_on_a_list_with_2_items_both_matching_yields_the_first_item)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT));

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_039: [ If the item is not found, `slist_find` shall return `NULL`. ]*/
TEST_FUNCTION(slist_find_on_a_list_with_2_items_where_none_matches_returns_NULL)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(test_match_function(IGNORED_ARG, TEST_CONTEXT))
        .SetReturn(false);

    // act
    PSINGLYLINKEDLIST_ENTRY result = slist_find(&head, test_match_function, TEST_CONTEXT);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*slist_remove_if*/

/* Tests_SRS_SLIST_07_026: [ If `list_head` is `NULL`, `slist_remove_if` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(slist_remove_if_with_NULL_list_fails)
{
    //arrange

    // act
    int result = slist_remove_if(NULL, test_condition_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_027: [ If `condition_function` is `NULL`, `slist_remove_if` shall fail and return a non-zero value. ]*/
TEST_FUNCTION(slist_remove_if_with_NULL_condition_function_fails)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    // act
    int result = slist_remove_if(&head, NULL, TEST_CONTEXT);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_040: [ If the list is empty, `slist_find` shall return a non-zero value. ]*/
TEST_FUNCTION(slist_remove_if_with_empty_list_fails)
{
    // arrange
    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);

    // act
    int result = slist_remove_if(&head, NULL, TEST_CONTEXT);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_028: [ `slist_remove_if` shall iterate through all entris in a list, remove all that satisfies the `condition_function` and return zero. ]*/
/* Tests_SRS_SLIST_07_029: [ `slist_remove_if` shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. ]*/
/* Tests_SRS_SLIST_07_030: [ If the `condition_function` returns `true`, `slist_remove_if` shall consider that entry as to be removed. ]*/
TEST_FUNCTION(slist_remove_if_on_a_list_with_1_matching_item_removes_that_item)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));

    // act
    int result = slist_remove_if(&head, test_condition_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_031: [ If the `condition_function` returns `false`, `slist_remove_if` shall consider that entry as not to be removed. ]*/
TEST_FUNCTION(slist_remove_if_on_a_list_with_1_items_that_does_not_match_returns_orginal_head)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .SetReturn(false);

    // act
    int result = slist_remove_if(&head, test_condition_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, &(simp1.link));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_028: [ `slist_remove_if` shall iterate through all entris in a list, remove all that satisfies the `condition_function` and return zero. ]*/
/* Tests_SRS_SLIST_07_029: [ `slist_remove_if` shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. ]*/
/* Tests_SRS_SLIST_07_030: [ If the `condition_function` returns `true`, `slist_remove_if` shall consider that entry as to be removed. ]*/
TEST_FUNCTION(slist_remove_if_on_a_list_with_2_items_where_the_first_matches_deletes_the_first_item)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .SetReturn(false);

    // act
    int result = slist_remove_if(&head, test_condition_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, &(simp2.link));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_028: [ `slist_remove_if` shall iterate through all entris in a list, remove all that satisfies the `condition_function` and return zero. ]*/
/* Tests_SRS_SLIST_07_029: [ `slist_remove_if` shall determine whether an entry satisfies the condition criteria by invoking the condition function for that entry. ]*/
/* Tests_SRS_SLIST_07_030: [ If the `condition_function` returns `true`, `slist_remove_if` shall consider that entry as to be removed. ]*/
/* Tests_SRS_SLIST_07_031: [ If the `condition_function` returns `false`, `slist_remove_if` shall consider that entry as not to be removed. ]*/
TEST_FUNCTION(slist_remove_if_on_a_list_with_2_items_where_the_second_matches_deletes_the_second_item)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));

    // act
    int result = slist_remove_if(&head, test_condition_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(simp1.link.next);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_028: [ `slist_remove_if` shall iterate through all entris in a list, remove all that satisfies the `condition_function` and return zero. ]*/
TEST_FUNCTION(slist_remove_if_on_a_list_with_2_items_both_matching_deletes_all_items)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));

    // act
    int result = slist_remove_if(&head, test_condition_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NULL(head.next);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_031: [ If the `condition_function` returns `false`, `slist_remove_if` shall consider that entry as not to be removed. ]*/
TEST_FUNCTION(slist_remove_if_on_a_list_with_2_items_where_none_matches_returns_original_head)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));

    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(test_condition_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .SetReturn(false);

    // act
    int result = slist_remove_if(&head, test_condition_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, head.next, &(simp1.link));
    ASSERT_ARE_EQUAL(void_ptr, simp1.link.next, &(simp2.link));
    ASSERT_IS_NULL(simp2.link.next);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_032: [ If the `condition_function` returns `continue_processing` as `false`, `slist_remove_if` shall stop iterating through the list and return. ]*/
TEST_FUNCTION(slist_remove_if_with_continue_processing_false_returns_original_head)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));
    slist_add(&head, &(simp4.link));

    STRICT_EXPECTED_CALL(test_condition_function_2(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(test_condition_function_2(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(test_condition_function_2(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));

    // act
    int result = slist_remove_if(&head, test_condition_function_2, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(void_ptr, &(simp4.link), simp2.link.next);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*slist_for_each*/

/* Tests_SRS_SLIST_07_033: [ If `list_head` is `NULL`, `slist_for_each` shall fail and return a non - zero value. ]*/
TEST_FUNCTION(slist_for_each_with_NULL_list_fails_with_NULL)
{
    //arrange

    // act
    int result = slist_for_each(NULL, test_action_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_034: [ If `action_function` is `NULL`, `slist_for_each` shall fail and return a non - zero value. ]*/
TEST_FUNCTION(slist_for_each_with_NULL_action_function_fails_with_NULL)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));

    // act
    int result = slist_for_each(&head, NULL, TEST_CONTEXT);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_035: [ `slist_for_each` shall iterate through all entries in the list, invoke `action_function` for each one of themand return zero on success. ]*/
TEST_FUNCTION(slist_for_each_succeeds)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));
    slist_add(&head, &(simp4.link));

    STRICT_EXPECTED_CALL(test_action_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_action_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_action_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_action_function(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));

    // act
    int result = slist_for_each(&head, test_action_function, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_037: [ If the `action_function` returns `continue_processing` as `false`, `slist_for_each` shall stop iterating through the list and return. ]*/
TEST_FUNCTION(slist_for_each_with_continue_processing_false_stops_processing)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));
    slist_add(&head, &(simp4.link));

    STRICT_EXPECTED_CALL(test_action_function_2(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_action_function_2(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_action_function_2(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));

    // act
    int result = slist_for_each(&head, test_action_function_2, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SLIST_07_036: [ If the `action_function` fails, `slist_for_each` shall fail and return a non - zero value. ]*/
TEST_FUNCTION(slist_for_each_fails_when_continue_processing_fails)
{
    // arrange
    static simpleItem simp1 = { 1, { NULL } };
    static simpleItem simp2 = { 2, { NULL } };
    static simpleItem simp3 = { 3, { NULL } };
    static simpleItem simp4 = { 4, { NULL } };

    SINGLYLINKEDLIST_ENTRY head;
    slist_initialize(&head);
    slist_add(&head, &(simp1.link));
    slist_add(&head, &(simp2.link));
    slist_add(&head, &(simp3.link));
    slist_add(&head, &(simp4.link));

    STRICT_EXPECTED_CALL(test_action_function_fail(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_action_function_fail(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(test_action_function_fail(IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG));

    // act
    int result = slist_for_each(&head, test_action_function_fail, TEST_CONTEXT);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
