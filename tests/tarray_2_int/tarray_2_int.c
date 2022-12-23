// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

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
static void release_TARRAY_of_CATs(TARRAY_TYPEDEF_NAME(CAT)* cats, void* cleanup_context)
{
    (void)cleanup_context;
    ASSERT_IS_NOT_NULL(cats->arr[0].name);
    free(cats->arr[0].name);
}

/*some thread that prints a cat name and then releases its hold on the TARRAY(CAT)*/
static int cat_thread(void* p)
{
    TARRAY(CAT) all_cats = p;
    TARRAY_ASSIGN(CAT)(&all_cats, NULL);
    return 0;
}

#define N_TIMES 1000

/* the following test SHARES an array of cats between this thread (the main thread) and two others thread for the pupose of herding the cats */
/* the main thread doesn't want to know about the cats anymore, so it just TARRAY_ASSIGN(CAT)(..., NULL) to its TARRAY(CAT) */
/* which one of the 2 threads that has a hold on the TARRAY(CAT) should call free "zuzu"? */
/* answer is: CLEANUP function passed at TARRAY creation time */
TEST_FUNCTION(an_array_of_cats_can_have_their_names_uniquely_freed)
{
    /*run this test for N_TIMES*/
    for(uint32_t i = 0; i < N_TIMES; i++)
    {
        ///arrange
        TARRAY(CAT) all_cats = TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(CAT)(1, release_TARRAY_of_CATs, NULL);
        ASSERT_IS_NOT_NULL(all_cats);
        ASSERT_ARE_EQUAL(int, 0, TARRAY_ENSURE_CAPACITY(CAT)(all_cats, 1));

        all_cats->arr[0].name = sprintf_char("%s", "zuzu");
        ASSERT_IS_NOT_NULL(all_cats->arr[0].name);

        /*preparing data for the threads*/
        TARRAY(CAT) t1_cat = NULL;
        TARRAY_INITIALIZE(CAT)(&t1_cat, all_cats);

        TARRAY(CAT) t2_cat = NULL;
        TARRAY_INITIALIZE(CAT)(&t2_cat, all_cats);

        /*main thread doesn't want to deal with cats anymore*/
        TARRAY_ASSIGN(CAT)(&all_cats, NULL);

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


/*uniquely called just before the TARRAY is disposed*/
/*in this case, the array has 2 elements (by design) but whiever of these 2 elements is actually used (and therefore needs to be freed)*/
/*is passed as a bit mask in the context*/
static void release_TARRAY_of_CATs_2(TARRAY_TYPEDEF_NAME(CAT)* cats, void* cleanup_context)
{
    uint32_t used_cats_mask = *(uint32_t*)cleanup_context;
    if (used_cats_mask & 1)
    {
        ASSERT_IS_NOT_NULL(cats->arr[0].name);
        free(cats->arr[0].name);
    }
    if (used_cats_mask & 2)
    {
        ASSERT_IS_NOT_NULL(cats->arr[1].name);
        free(cats->arr[1].name);
    }
}

/*in this tests, MAYBE 0 or 1 or 2 CATs are stored in the array. Whiever indexes in the array are used are indicated by a bit mask*/
/*the cleanup function needs the bit mask to know which indexes are used (this comes as the cleanup_context) so that they can be freed*/
TEST_FUNCTION(an_array_of_cats_can_have_their_names_uniquely_freed_2)
{
    /*run this test for N_TIMES*/
    for (uint32_t i = 0; i < N_TIMES; i++)
    {
        uint32_t used_cats_mask=0; /*set to
            0 when 0 CATs are stored,
            1 when arr[0] is used,
            2 when arr[1] is used,
            3 when arr[2] is used
        */

        ///arrange
        TARRAY(CAT) all_cats = TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(CAT)(2, release_TARRAY_of_CATs_2, &used_cats_mask);
        ASSERT_IS_NOT_NULL(all_cats);
        ASSERT_ARE_EQUAL(int, 0, TARRAY_ENSURE_CAPACITY(CAT)(all_cats, 1));

        if (rand() % 2)
        {
            all_cats->arr[0].name = sprintf_char("%s", "zuzu");
            ASSERT_IS_NOT_NULL(all_cats->arr[0].name);
            used_cats_mask |= 1;
        }

        if (rand() % 2)
        {
            all_cats->arr[1].name = sprintf_char("%s", "snowflake");
            ASSERT_IS_NOT_NULL(all_cats->arr[1].name);
            used_cats_mask |= 2;
        }

        /*preparing data for the threads*/
        TARRAY(CAT) t1_cat = NULL;
        TARRAY_INITIALIZE(CAT)(&t1_cat, all_cats);

        TARRAY(CAT) t2_cat = NULL;
        TARRAY_INITIALIZE(CAT)(&t2_cat, all_cats);

        /*main thread doesn't want to deal with cats anymore*/
        TARRAY_ASSIGN(CAT)(&all_cats, NULL);

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

