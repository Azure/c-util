// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/string_utils.h"
#include "c_pal/threadapi.h"

#include "c_util/thandle.h"
#include "c_util/tarray.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

static TEST_MUTEX_HANDLE g_testByTest;

/*CAT is the type used in TARRAY*/
typedef struct CAT_TAG
{
    char* name;
}CAT;

/*the following 5 lines are canon way of getting to have a TARRAY*/
TARRAY_DEFINE_STRUCT_TYPE(CAT);
THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(CAT));
TARRAY_TYPE_DECLARE(CAT);

THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(CAT));

TARRAY_TYPE_DEFINE(CAT);


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(it_undoes_something)
{
    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(init_f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(deinit_f)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*uniquely called just before the TARRAY is disposed*/
static void release_TARRAY_of_CATs(TARRAY_TYPEDEF_NAME(CAT)* cats)
{
    ASSERT_IS_NOT_NULL(cats->arr[0].name);
    free(cats->arr[0].name);
}

/*some thread that prints a cat name and then releases its hold on the TARRAY(CAT)*/
static int cat_thread(void* p)
{
    TARRAY(CAT) allCats = p;
    //LogInfo("hello from thread %" PRIu32 ". I have the cat \"%s\"", ThreadAPI_GetCurrentThreadId(), allCats->arr[0].name);
    TARRAY_ASSIGN(CAT)(&allCats, NULL);
    return 0;
}

#define N_TIMES 10000

/* the following test SHARES an array of cats between this thread (the main thread) and two others thread for the pupose of hearding the cats */
/* the main thread doesn't want to know about the cats anymore, so it just TARRAY_ASSIGN(CAT)(..., NULL) to its TARRAY(CAT) */
/* which one of the 2 threads that has a hold on the TARRAY(CAT) should call free "zuzu"? */
/* answer is: CLEANUP function passed at TARRAY creation time */
TEST_FUNCTION(an_array_of_cats_can_have_their_names_uniquely_freed)
{
    /*run this test for N_TIMES*/
    for(uint32_t i = 0; i < N_TIMES; i++)
    {
        ///arrange
        TARRAY(CAT) allCats = TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(CAT)(1, release_TARRAY_of_CATs);
        ASSERT_IS_NOT_NULL(allCats);
        ASSERT_ARE_EQUAL(int, 0, TARRAY_ENSURE_CAPACITY(CAT)(allCats, 1));

        allCats->arr[0].name = sprintf_char("%s", "zuzu");
        ASSERT_IS_NOT_NULL(allCats->arr[0].name);

        /*preparing data for the threads*/
        TARRAY(CAT) t1_cat = NULL;
        TARRAY_INITIALIZE(CAT)(&t1_cat, allCats);

        TARRAY(CAT) t2_cat = NULL;
        TARRAY_INITIALIZE(CAT)(&t2_cat, allCats);

        /*main thread doesn't want to deal with cats anymore*/
        TARRAY_ASSIGN(CAT)(&allCats, NULL);

        ///act - start thread 1
        THREAD_HANDLE t1;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&t1, cat_thread, (void*)t1_cat));

        ///act - start thread 2
        THREAD_HANDLE t2;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&t2, cat_thread, (void*)t2_cat));

        ///assert
        /*by means of "no crashing" and "no leaks"*/

        ///clean
        int dummy;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(t1, &dummy));
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(t2, &dummy));
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

