// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"

#include "c_pal/containing_record.h"

#include "c_util/doublylinkedlist.h"
#include "testrunnerswitcher.h"

typedef struct simpleItem_tag
{
    unsigned char index;
    DLIST_ENTRY link;
} simpleItem,*pSimpleItem;

static simpleItem simp1 = { 1, { NULL, NULL } };
static simpleItem simp2 = { 2, { NULL, NULL } };
static simpleItem simp3 = { 3, { NULL, NULL } };
static simpleItem simp4 = { 4, { NULL, NULL } };
static simpleItem simp5 = { 5, { NULL, NULL } };

static void* test_action_context = (void*)0x1000;
static PDLIST_ENTRY test_pdlist_entry = (PDLIST_ENTRY)0x1001;

MOCK_FUNCTION_WITH_CODE(, int, test_action_function, PDLIST_ENTRY, list_entry, void*, action_context, bool*, continueProcessing);
    (void)list_entry;
    (void)action_context;
    *continueProcessing = true;
    return 0;
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, int, test_action_function_2, PDLIST_ENTRY, list_entry, void*, action_context, bool*, continueProcessing);
    (void)action_context;
    simpleItem* item = CONTAINING_RECORD(list_entry, simpleItem, link);
    *continueProcessing = !(item->index == 3);
    return 0;
MOCK_FUNCTION_END();


MOCK_FUNCTION_WITH_CODE(, int, test_action_function_fail, PDLIST_ENTRY, list_entry, void*, action_context, bool*, continueProcessing);
    (void)action_context;
    *continueProcessing = true;
    simpleItem* item = CONTAINING_RECORD(list_entry, simpleItem, link);
    if (item->index == 3)
    {
        return MU_FAILURE;
    }
    return 0;
MOCK_FUNCTION_END();

MOCK_FUNCTION_WITH_CODE(, int, test_action_function_delete, PDLIST_ENTRY, list_entry, void*, action_context, bool*, continueProcessing);
    (void)action_context;
    *continueProcessing = true;
    simpleItem* item = CONTAINING_RECORD(list_entry, simpleItem, link);
    free(item);
    return 0;
MOCK_FUNCTION_END();

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");

    REGISTER_UMOCK_ALIAS_TYPE(PDLIST_ENTRY, void*);

}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    umock_c_reset_all_calls();
}

    /* Tests_SRS_DLIST_06_005: [DList_InitializeListHead will initialize the Flink & Blink to the address of the DLIST_ENTRY.] */
    TEST_FUNCTION(DList_InitializeListHead_with_a_list_head_points_Flink_and_Blink_to_its_address)
    {
        // arrange
        DLIST_ENTRY head;

        // act
        DList_InitializeListHead(&head);

        // assert
        ASSERT_ARE_EQUAL(void_ptr, &head, head.Flink);
        ASSERT_ARE_EQUAL(void_ptr, &head, head.Blink);
    }

    /* Tests_SRS_DLIST_06_003: [DList_IsListEmpty shall return a non-zero value if there are no DLIST_ENTRY's on this list other than the list head.] */
    TEST_FUNCTION(DList_IsListEmpty_with_a_list_head_only_returns_empty)
    {
        // arrange
        DLIST_ENTRY local;
        int result;

        DList_InitializeListHead(&local);
        // act
        result = DList_IsListEmpty(&local);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    /* Tests_SRS_DLIST_06_004: [DList_IsListEmpty shall return 0 if there is one or more items in the list.]*/
    TEST_FUNCTION(DList_IsListEmpty_with_a_list_head_and_items_returns_empty)
    {
        // arrange
        DLIST_ENTRY head;
        int result;

        DList_InitializeListHead(&head);
        DList_InsertTailList(&head, &(simp1.link));
        // act
        result = DList_IsListEmpty(&head);

        // assert
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    /* Tests_SRS_DLIST_06_006: [DListInsertTailList shall place the DLIST_ENTRY at the end of the list defined by the listHead parameter.] */
    TEST_FUNCTION(DList_InsertTailList_with_a_list_head_inserts_1st_item_at_the_end_of_the_list)
    {
        // arrange
        DLIST_ENTRY head;

        DList_InitializeListHead(&head);
        // act
        DList_InsertTailList(&head, &(simp1.link));

        // assert
        ASSERT_ARE_EQUAL(void_ptr, head.Flink, &(simp1.link));
        ASSERT_ARE_EQUAL(void_ptr, head.Blink, &(simp1.link));
    }

    /* Tests_SRS_DLIST_06_006: [DListInsertTailList shall place the DLIST_ENTRY at the end of the list defined by the listHead parameter.] */
    TEST_FUNCTION(DList_InsertTailList_with_a_list_head_inserts_2nd_item_at_the_end_of_the_list)
    {
        // arrange
        DLIST_ENTRY head;

        DList_InitializeListHead(&head);
        DList_InsertTailList(&head, &(simp1.link));
        // act
        DList_InsertTailList(&head, &(simp2.link));

        // assert
        ASSERT_ARE_EQUAL(void_ptr, head.Flink, &(simp1.link));
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Flink, &(simp2.link));
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Flink, &head);
        ASSERT_ARE_EQUAL(void_ptr, head.Blink, &(simp2.link));
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Blink, &head);
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Blink, &(simp1.link));
    }

    /* Tests_SRS_DLIST_06_007: [DList_AppendTailList shall place the list defined by ListToAppend at the end of the list defined by the listHead parameter.] */
    TEST_FUNCTION(DList_AppendTailList_adds_listToAppend_after_listHead)
    {
        // arrange
        DLIST_ENTRY listHead;
        PDLIST_ENTRY currentEntry;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp1.link));

        DList_InitializeListHead(&simp2.link);
        DList_InsertTailList(&simp2.link, &simp4.link);
        DList_InsertTailList(&simp2.link, &simp5.link);
        // act
        DList_AppendTailList(&listHead, &simp2.link);

        // assert
        // Go forwards
        ASSERT_ARE_EQUAL(int, 0, DList_IsListEmpty(&listHead));
        currentEntry = listHead.Flink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp1.link);
        ASSERT_ARE_EQUAL(short, (short)1, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
        currentEntry = currentEntry->Flink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp2.link);
        ASSERT_ARE_EQUAL(short, (short)2, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
        currentEntry = currentEntry->Flink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp4.link);
        ASSERT_ARE_EQUAL(short, (short)4, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
        currentEntry = currentEntry->Flink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp5.link);
        ASSERT_ARE_EQUAL(short, (short)5, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
        currentEntry = currentEntry->Flink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &listHead);

        // Now back
        currentEntry = listHead.Blink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp5.link);
        ASSERT_ARE_EQUAL(short, (short)5, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
#ifdef _MSC_VER
#pragma warning(suppress: 6011) /* test code, should crash if this is truly NULL */
#endif
        currentEntry = currentEntry->Blink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp4.link);
        ASSERT_ARE_EQUAL(short, (short)4, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
        currentEntry = currentEntry->Blink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp2.link);
        ASSERT_ARE_EQUAL(short, (short)2, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
        currentEntry = currentEntry->Blink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &simp1.link);
        ASSERT_ARE_EQUAL(short, (short)1, CONTAINING_RECORD(currentEntry, simpleItem, link)->index);
        currentEntry = currentEntry->Blink;
        ASSERT_ARE_EQUAL(void_ptr, currentEntry, &listHead);
    }

    /* Tests_SRS_DLIST_06_010: [DList_RemoveEntryList shall return non-zero if the remaining list is empty.] */
    TEST_FUNCTION(DList_RemoveEntryList_with_head_only_in_list_shall_return_non_zero)
    {
        // arrange
        DLIST_ENTRY listHead;
        int resultOfRemove;

        DList_InitializeListHead(&listHead);

        // act
        resultOfRemove = DList_RemoveEntryList(&listHead);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, resultOfRemove);
    }

    /* Tests_SRS_DLIST_06_008: [DList_RemoveEntryList shall remove a listEntry from whatever list it is properly part of.] */
    /* Tests_SRS_DLIST_06_010: [DList_RemoveEntryList shall return non-zero if the remaining list is empty.] */
    /* Tests_SRS_DLIST_06_009: [The remaining list is properly formed.] */
    TEST_FUNCTION(DList_RemoveEntryList_with_one_element_and_removing_that_one_element_shall_return_non_zero)
    {
        // arrange
        DLIST_ENTRY listHead;
        int resultOfRemove;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp1.link));

        // act
        resultOfRemove = DList_RemoveEntryList(&(simp1.link));

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, resultOfRemove);
        ASSERT_ARE_EQUAL(void_ptr, &listHead, listHead.Blink);
        ASSERT_ARE_EQUAL(void_ptr, &listHead, listHead.Flink);
    }

    /* Tests_SRS_DLIST_06_008: [DList_RemoveEntryList shall remove a listEntry from whatever list it is properly part of.] */
    /* Tests_SRS_DLIST_06_010: [DList_RemoveEntryList shall return non-zero if the remaining list is empty.] */
    TEST_FUNCTION(DList_RemoveEntryList_with_one_element_and_removing_the_head_shall_return_non_zero)
    {
        // arrange
        DLIST_ENTRY listHead;
        int resultOfRemove;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp1.link));

        // act
        resultOfRemove = DList_RemoveEntryList(&listHead);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, resultOfRemove);
        ASSERT_ARE_EQUAL(void_ptr, &(simp1.link), simp1.link.Blink);
        ASSERT_ARE_EQUAL(void_ptr, &(simp1.link), simp1.link.Flink);
    }

    /* Tests_SRS_DLIST_06_008: [DList_RemoveEntryList shall remove a listEntry from whatever list it is properly part of.] */
    /* Tests_SRS_DLIST_06_009: [The remaining list is properly formed.] */
    /* Tests_SRS_DLIST_06_010: [DList_RemoveEntryList shall return non-zero if the remaining list is empty.] */
    /* Tests_SRS_DLIST_06_011: [DList_RemoveEntryList shall return zero if the remaining list is NOT empty.] */
    TEST_FUNCTION(DList_RemoveEntryList_with_three_elements_and_removing_the_first_return_zero)
    {
        // arrange
        DLIST_ENTRY listHead;
        int resultOfRemove;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp1.link));
        DList_InsertTailList(&listHead, &(simp2.link));
        DList_InsertTailList(&listHead, &(simp3.link));

        // act
        resultOfRemove = DList_RemoveEntryList(&(simp1.link));

        // assert
        ASSERT_ARE_EQUAL(int, 0, resultOfRemove);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Flink, &(simp2.link));
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Flink, &(simp3.link));
        ASSERT_ARE_EQUAL(void_ptr, simp3.link.Flink, &listHead);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Blink, &(simp3.link));
        ASSERT_ARE_EQUAL(void_ptr, simp3.link.Blink, &(simp2.link));
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Blink, &(listHead));
    }

    /* Tests_SRS_DLIST_06_008: [DList_RemoveEntryList shall remove a listEntry from whatever list it is properly part of.] */
    /* Tests_SRS_DLIST_06_009: [The remaining list is properly formed.] */
    /* Tests_SRS_DLIST_06_010: [DList_RemoveEntryList shall return non-zero if the remaining list is empty.] */
    /* Tests_SRS_DLIST_06_011: [DList_RemoveEntryList shall return zero if the remaining list is NOT empty.] */
    TEST_FUNCTION(DList_RemoveEntryList_with_three_elements_and_removing_the_last_return_zero)
    {
        // arrange
        DLIST_ENTRY listHead;
        int resultOfRemove;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp1.link));
        DList_InsertTailList(&listHead, &(simp2.link));
        DList_InsertTailList(&listHead, &(simp3.link));

        // act
        resultOfRemove = DList_RemoveEntryList(&(simp3.link));

        // assert
        ASSERT_ARE_EQUAL(int, 0, resultOfRemove);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Flink, &(simp1.link));
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Flink, &(simp2.link));
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Flink, &listHead);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Blink, &(simp2.link));
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Blink, &(simp1.link));
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Blink, &(listHead));
    }

    /* Tests_SRS_DLIST_06_008: [DList_RemoveEntryList shall remove a listEntry from whatever list it is properly part of.] */
    /* Tests_SRS_DLIST_06_009: [The remaining list is properly formed.] */
    /* Tests_SRS_DLIST_06_010: [DList_RemoveEntryList shall return non-zero if the remaining list is empty.] */
    /* Tests_SRS_DLIST_06_011: [DList_RemoveEntryList shall return zero if the remaining list is NOT empty.] */
    TEST_FUNCTION(DList_RemoveEntryList_with_three_elements_and_removing_the_middle_return_zero)
    {
        // arrange
        DLIST_ENTRY listHead;
        int resultOfRemove;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp1.link));
        DList_InsertTailList(&listHead, &(simp2.link));
        DList_InsertTailList(&listHead, &(simp3.link));

        // act
        resultOfRemove = DList_RemoveEntryList(&(simp2.link));

        // assert
        ASSERT_ARE_EQUAL(int, 0, resultOfRemove);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Flink, &(simp1.link));
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Flink, &(simp3.link));
        ASSERT_ARE_EQUAL(void_ptr, simp3.link.Flink, &listHead);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Blink, &(simp3.link));
        ASSERT_ARE_EQUAL(void_ptr, simp3.link.Blink, &(simp1.link));
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Blink, &(listHead));
    }

    /* Tests_SRS_DLIST_06_013: [DList_RemoveHeadList shall return listHead if that's the only item in the list.] */
    TEST_FUNCTION(DList_RemoveHeadList_with_only_head_shall_return_the_head)
    {
        // arrange
        DLIST_ENTRY head;
        PDLIST_ENTRY returnedEntry;

        DList_InitializeListHead(&head);

        // act
        returnedEntry = DList_RemoveHeadList(&head);

        // assert
        ASSERT_ARE_EQUAL(void_ptr, &head, returnedEntry);
    }

    /* Tests_SRS_DLIST_06_012: [DList_RemoveHeadList removes the oldest entry from the list defined by the listHead parameter and returns a pointer to that entry.] */
    TEST_FUNCTION(DList_RemoveHeadList_with_one_entry_returns_entry)
    {
        // arrange
        DLIST_ENTRY listHead;
        PDLIST_ENTRY returnedEntry;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp1.link));

        // act
        returnedEntry = DList_RemoveHeadList(&listHead);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, DList_IsListEmpty(&listHead));
        ASSERT_ARE_EQUAL(void_ptr, &(simp1.link), returnedEntry);
    }

    /*Tests_SRS_DLIST_02_003: [ DList_RemoveTailList removes the newest entry inserted at the tail of the list defined by the listHead parameter and returns a pointer to that entry. ]*/
    /*Tests_SRS_DLIST_02_004: [ DList_RemoveTailList shall return listHead if that's the only item in the list. ]*/
    TEST_FUNCTION(DList_RemoveTailList_with_empty_list_returns_the_list)
    {
        // arrange
        DLIST_ENTRY theList;
        PDLIST_ENTRY returnedEntry;

        DList_InitializeListHead(&theList);
        
        // act
        returnedEntry = DList_RemoveTailList(&theList);

        // assert
        ASSERT_ARE_EQUAL(void_ptr, &theList, returnedEntry);

        /*is empty?*/
        ASSERT_IS_TRUE(DList_IsListEmpty(&theList));

        /*are links still properly set up?*/
        ASSERT_ARE_EQUAL(void_ptr, theList.Flink, &theList);
        ASSERT_ARE_EQUAL(void_ptr, theList.Blink, &theList);
    }

    /*Tests_SRS_DLIST_02_003: [ DList_RemoveTailList removes the newest entry inserted at the tail of the list defined by the listHead parameter and returns a pointer to that entry. ]*/
    TEST_FUNCTION(DList_RemoveTailList_with_1_entry_returns_theList)
    {
        // arrange
        DLIST_ENTRY theList;
        PDLIST_ENTRY returnedEntry;

        DList_InitializeListHead(&theList);
        DList_InsertTailList(&theList, &simp1.link);

        // act
        returnedEntry = DList_RemoveTailList(&theList);

        // assert
        ASSERT_ARE_EQUAL(void_ptr, &simp1.link, returnedEntry);

        /*is empty?*/
        ASSERT_IS_TRUE(DList_IsListEmpty(&theList));

        /*are links still properly set up?*/
        ASSERT_ARE_EQUAL(void_ptr, theList.Flink, &theList);
        ASSERT_ARE_EQUAL(void_ptr, theList.Blink, &theList);
    }

    /*Tests_SRS_DLIST_02_003: [ DList_RemoveTailList removes the newest entry inserted at the tail of the list defined by the listHead parameter and returns a pointer to that entry. ]*/
    TEST_FUNCTION(DList_RemoveTailList_with_2_entries_returns_the_removed_entry)
    {
        // arrange
        DLIST_ENTRY theList;
        PDLIST_ENTRY returnedEntry;

        DList_InitializeListHead(&theList);
        DList_InsertTailList(&theList, &simp1.link);
        DList_InsertTailList(&theList, &simp2.link);

        // act
        returnedEntry = DList_RemoveTailList(&theList);

        // assert
        ASSERT_ARE_EQUAL(void_ptr, &simp2.link, returnedEntry);

        /*is empty?*/
        ASSERT_IS_FALSE(DList_IsListEmpty(&theList));
        
        /*are links still properly set up?*/
        ASSERT_ARE_EQUAL(void_ptr, theList.Flink, &simp1.link);
        ASSERT_ARE_EQUAL(void_ptr, theList.Blink, &simp1.link);
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Flink, &theList);
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Blink, &theList);
    }

    /*Tests_SRS_DLIST_02_003: [ DList_RemoveTailList removes the newest entry inserted at the tail of the list defined by the listHead parameter and returns a pointer to that entry. ]*/
    TEST_FUNCTION(DList_RemoveTailList_with_3_entries_returns_the_removed_entry)
    {
        // arrange
        DLIST_ENTRY theList;
        PDLIST_ENTRY returnedEntry;

        DList_InitializeListHead(&theList);
        DList_InsertTailList(&theList, &simp1.link);
        DList_InsertTailList(&theList, &simp2.link);
        DList_InsertTailList(&theList, &simp3.link);

        // act
        returnedEntry = DList_RemoveTailList(&theList);

        // assert
        ASSERT_ARE_EQUAL(void_ptr, &simp3.link, returnedEntry);

        ASSERT_IS_FALSE(DList_IsListEmpty(&theList));

        ASSERT_ARE_EQUAL(void_ptr, theList.Flink, &simp1.link);
        ASSERT_ARE_EQUAL(void_ptr, theList.Blink, &simp2.link);
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Flink, &simp2.link);
        ASSERT_ARE_EQUAL(void_ptr, simp1.link.Blink, &theList);
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Flink, &theList);
        ASSERT_ARE_EQUAL(void_ptr, simp2.link.Blink, &simp1.link);
    }


    /* Tests_SRS_DLIST_06_012: [DList_RemoveHeadList removes the oldest entry from the list defined by the listHead parameter and returns a pointer to that entry.] */
    TEST_FUNCTION(DList_RemoveHeadList_with_two_entries_returns_first_entry)
    {
        // arrange
        DLIST_ENTRY listHead;
        PDLIST_ENTRY returnedEntry;

        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &(simp2.link));
        DList_InsertTailList(&listHead, &(simp1.link));

        // act
        returnedEntry = DList_RemoveHeadList(&listHead);

        // assert
        ASSERT_ARE_EQUAL(int, 0, DList_IsListEmpty(&listHead));
        ASSERT_ARE_EQUAL(void_ptr, &(simp2.link), returnedEntry);
    }

    /*Tests_SRS_DLIST_02_002: [DList_InsertHeadList inserts a singular entry in the list having as head listHead after "head".]*/
    TEST_FUNCTION(DList_InsertHeadList_with_empty_list_succeeds)
    {
        ///arrange
        DLIST_ENTRY listHead;
        DLIST_ENTRY toBeInserted;
        DList_InitializeListHead(&listHead);

        ///act
        DList_InsertHeadList(&listHead, &toBeInserted);

        ///assert
        ASSERT_ARE_EQUAL(void_ptr, listHead.Flink, &toBeInserted);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Blink, &toBeInserted);
        ASSERT_ARE_EQUAL(void_ptr, toBeInserted.Flink , &listHead);
        ASSERT_ARE_EQUAL(void_ptr, toBeInserted.Blink, &listHead);
    }

    /*Tests_SRS_DLIST_02_002: [DList_InsertHeadList inserts a singular entry in the list having as head listHead after "head".]*/
    TEST_FUNCTION(DList_InsertHeadList_with_1_item_in_list_succeeds)
    {
        ///arrange
        DLIST_ENTRY listHead;
        DLIST_ENTRY existingInList;
        DLIST_ENTRY toBeInserted;
        DList_InitializeListHead(&listHead);
        DList_InsertTailList(&listHead, &existingInList); /*would be same as insertHead when it is the first item... */

        ///act
        DList_InsertHeadList(&listHead, &toBeInserted);

        ///assert
        ASSERT_ARE_EQUAL(void_ptr, listHead.Flink, &toBeInserted);
        ASSERT_ARE_EQUAL(void_ptr, listHead.Blink, &existingInList);
        ASSERT_ARE_EQUAL(void_ptr, existingInList.Flink, &listHead);
        ASSERT_ARE_EQUAL(void_ptr, existingInList.Blink, &toBeInserted);
        ASSERT_ARE_EQUAL(void_ptr, toBeInserted.Flink, &existingInList);
        ASSERT_ARE_EQUAL(void_ptr, toBeInserted.Blink, &listHead);

    }

    /*Tests_SRS_DLIST_43_001: [If listHead is NULL, DList_ForEach shall fail and return a non - zero value.]*/
    /*Tests_SRS_DLIST_43_012 : [If there are any failures, DList_ForEach shall fail and return a non - zero value.]*/
    TEST_FUNCTION(DList_ForEach_fails_with_NULL_listHead)
    {
        ///arrange

        ///act
        int result = DList_ForEach(NULL, test_action_function, test_action_context);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }
    
    /*Tests_SRS_DLIST_43_002 : [If actionFunction is NULL, DList_ForEach shall fail and return a non - zero value.]*/
    /*Tests_SRS_DLIST_43_012 : [If there are any failures, DList_ForEach shall fail and return a non - zero value.]*/
    TEST_FUNCTION(DList_ForEach_fails_with_NULL_actionFunction)
    {
        ///arrange

        ///act
        int result = DList_ForEach(test_pdlist_entry, NULL, test_action_context);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    /*Tests_SRS_DLIST_43_009 : [DList_ForEach shall call actionFunction on each entry in the list defined by listHead along with actionContext.]*/
    /*Tests_SRS_DLIST_43_011 : [DList_ForEach shall succeed and return zero.]*/
    TEST_FUNCTION(DList_ForEach_succeeds)
    {
        ///arrange
        DLIST_ENTRY head;
        DList_InitializeListHead(&head);
        DList_InsertTailList(&head, &(simp1.link));
        DList_InsertTailList(&head, &(simp2.link));
        DList_InsertTailList(&head, &(simp3.link));
        DList_InsertTailList(&head, &(simp4.link));
        DList_InsertTailList(&head, &(simp5.link));

        STRICT_EXPECTED_CALL(test_action_function(&(simp1.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function(&(simp2.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function(&(simp3.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function(&(simp4.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function(&(simp5.link), test_action_context, IGNORED_ARG));
        
        ///act
        int result = DList_ForEach(&head, test_action_function, test_action_context);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    TEST_FUNCTION(DList_ForEach_succeeds_with_empty_list)
    {
        ///arrange
        DLIST_ENTRY head;
        DList_InitializeListHead(&head);

        ///act
        int result = DList_ForEach(&head, test_action_function, test_action_context);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    /*Tests_SRS_DLIST_43_009 : [DList_ForEach shall call actionFunction on each entry in the list defined by listHead along with actionContext.]*/
    /*Tests_SRS_DLIST_43_010 : [If continueProcessing is false, DList_ForEach shall stop iterating over the list.]*/
    /*Tests_SRS_DLIST_43_011 : [DList_ForEach shall succeed and return zero.]*/
    TEST_FUNCTION(DList_ForEach_stops_processing_when_continueProcessing_is_false)
    {
        ///arrange
        DLIST_ENTRY head;
        DList_InitializeListHead(&head);
        DList_InsertTailList(&head, &(simp1.link));
        DList_InsertTailList(&head, &(simp2.link));
        DList_InsertTailList(&head, &(simp3.link));
        DList_InsertTailList(&head, &(simp4.link));
        DList_InsertTailList(&head, &(simp5.link));

        STRICT_EXPECTED_CALL(test_action_function_2(&(simp1.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function_2(&(simp2.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function_2(&(simp3.link), test_action_context, IGNORED_ARG));

        ///act
        int result = DList_ForEach(&head, test_action_function_2, test_action_context);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    /*Tests_SRS_DLIST_43_009: [DList_ForEach shall call actionFunction on each entry in the list defined by listHead along with actionContext.]*/
    /*Tests_SRS_DLIST_43_012: [If there are any failures, DList_ForEach shall fail and return a non - zero value.]*/
    TEST_FUNCTION(DList_ForEach_fails_when_actionFunction_fails)
    {
        ///arrange
        DLIST_ENTRY head;
        DList_InitializeListHead(&head);
        DList_InsertTailList(&head, &(simp1.link));
        DList_InsertTailList(&head, &(simp2.link));
        DList_InsertTailList(&head, &(simp3.link));
        DList_InsertTailList(&head, &(simp4.link));
        DList_InsertTailList(&head, &(simp5.link));

        STRICT_EXPECTED_CALL(test_action_function_fail(&(simp1.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function_fail(&(simp2.link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function_fail(&(simp3.link), test_action_context, IGNORED_ARG));
        ///act
        int result = DList_ForEach(&head, test_action_function_fail, test_action_context);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
    }

    /*Tests_SRS_DLIST_24_001: [DList_ForEach shall allow deletion of elements during traversal.]*/
    TEST_FUNCTION(DList_ForEach_continues_traversal_when_items_are_deleted)
    {
        ///arrange
        DLIST_ENTRY head;
        simpleItem* item1 = malloc(sizeof(simpleItem));
        simpleItem* item2 = malloc(sizeof(simpleItem));
        simpleItem* item3 = malloc(sizeof(simpleItem));
        DList_InitializeListHead(&head);
        DList_InsertTailList(&head, &(item1->link));
        DList_InsertTailList(&head, &(item2->link));
        DList_InsertTailList(&head, &(item3->link));

        STRICT_EXPECTED_CALL(test_action_function_delete(&(item1->link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function_delete(&(item2->link), test_action_context, IGNORED_ARG));
        STRICT_EXPECTED_CALL(test_action_function_delete(&(item3->link), test_action_context, IGNORED_ARG));
        
        ///act
        int result = DList_ForEach(&head, test_action_function_delete, test_action_context);

        ///assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_ARE_EQUAL(int, 0, result);
    }

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
