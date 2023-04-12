// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/job_queue.h"

static TEST_MUTEX_HANDLE g_testByTest;

TEST_DEFINE_ENUM_TYPE(JOB_QUEUE_JOB_RESULT, JOB_QUEUE_JOB_RESULT_VALUES);

static void* test_job = (void*)0x0001;
static JOB_COMPLETE_CALLBACK test_job_complete_callback = (JOB_COMPLETE_CALLBACK)0x0002;
static void* test_job_complete_callback_context = (void*)0x0003;
static void* test_creation_context = (void*)0x0004;
static void* test_request_context = (void*)0x0005;
static void* test_request_context_2 = (void*)0x0006;


static void job_complete_callback(void* job_complete_callback_context, JOB_QUEUE_JOB_RESULT result)
{
    ASSERT_ARE_EQUAL(void_ptr, test_job_complete_callback_context, job_complete_callback_context);
    ASSERT_ARE_EQUAL(JOB_QUEUE_JOB_RESULT, result, JOB_QUEUE_JOB_RESULT_OK);
}

static void job_complete_callback_with_error(void* job_complete_callback_context, JOB_QUEUE_JOB_RESULT result)
{
    ASSERT_ARE_EQUAL(void_ptr, test_job_complete_callback_context, job_complete_callback_context);
    ASSERT_ARE_EQUAL(JOB_QUEUE_JOB_RESULT, result, JOB_QUEUE_JOB_RESULT_ERROR);
}


static bool request_function_complete_job(const void* creation_context, void* job_request_context, void* job)
{
    ASSERT_ARE_EQUAL(void_ptr, test_creation_context, creation_context);
    ASSERT_ARE_EQUAL(void_ptr, test_request_context, job_request_context);
    ASSERT_ARE_EQUAL(void_ptr, test_job, job);
    return true;
}

static bool request_function_do_not_complete_job(const void* creation_context, void* job_request_context, void* job)
{
    ASSERT_ARE_EQUAL(void_ptr, test_creation_context, creation_context);
    ASSERT_ARE_EQUAL(void_ptr, test_request_context_2, job_request_context);
    ASSERT_ARE_EQUAL(void_ptr, test_job, job);
    return false;
}


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(it_does_something)
{
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}


TEST_FUNCTION(test_job_queue_create)
{
    //arrange

    //act
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);

    //assert
    ASSERT_IS_NOT_NULL(job_queue);

    //cleanup
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_job_queue_session)
{
    //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);

    //act
    JOB_QUEUE_SESSION_HANDLE job_queue_session = job_queue_session_begin(job_queue);

    //assert
    ASSERT_IS_NOT_NULL(job_queue_session);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_multiple_sessions)
{
    //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);

    //act
    JOB_QUEUE_SESSION_HANDLE job_queue_session1 = job_queue_session_begin(job_queue);
    JOB_QUEUE_SESSION_HANDLE job_queue_session2 = job_queue_session_begin(job_queue);

    //assert
    ASSERT_IS_NOT_NULL(job_queue_session1);
    ASSERT_IS_NOT_NULL(job_queue_session2);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session1);
    job_queue_session_end(job_queue, job_queue_session2);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_job_popped_by_1_session)
{
     //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session = job_queue_session_begin(job_queue);
    job_queue_push(job_queue, test_job, job_complete_callback, test_job_complete_callback_context);

    //act
    int result = job_queue_session_pop(job_queue, job_queue_session, request_function_complete_job, test_request_context);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_job_popped_by_2_sessions)
{
        //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session1 = job_queue_session_begin(job_queue);
    JOB_QUEUE_SESSION_HANDLE job_queue_session2 = job_queue_session_begin(job_queue);
    job_queue_push(job_queue, test_job, job_complete_callback, test_job_complete_callback_context);

    //act
    int result1 = job_queue_session_pop(job_queue, job_queue_session1, request_function_complete_job, test_request_context);
    int result2 = job_queue_session_pop(job_queue, job_queue_session2, request_function_complete_job, test_request_context);

    //assert
    ASSERT_ARE_EQUAL(int, result1, 0);
    ASSERT_ARE_EQUAL(int, result2, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session1);
    job_queue_session_end(job_queue, job_queue_session2);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_1_session_ends_1_session_pops)
{
    //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session_1 = job_queue_session_begin(job_queue);
    JOB_QUEUE_SESSION_HANDLE job_queue_session_2 = job_queue_session_begin(job_queue);
    job_queue_push(job_queue, test_job, job_complete_callback, test_job_complete_callback_context);

    //act
    job_queue_session_end(job_queue, job_queue_session_1);
    int result = job_queue_session_pop(job_queue, job_queue_session_2, request_function_complete_job, test_request_context);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session_2);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_1_session_pops_1_session_ends)
{
    //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session_1 = job_queue_session_begin(job_queue);
    JOB_QUEUE_SESSION_HANDLE job_queue_session_2 = job_queue_session_begin(job_queue);
    job_queue_push(job_queue, test_job, job_complete_callback_with_error, test_job_complete_callback_context);

    //act
    int result = job_queue_session_pop(job_queue, job_queue_session_1, request_function_do_not_complete_job, test_request_context_2);
    job_queue_session_end(job_queue, job_queue_session_2);

    //assert
    ASSERT_ARE_EQUAL(int, result, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session_1);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_session_pop_then_job_push)
{
    //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session = job_queue_session_begin(job_queue);

    //act
    int pop_result = job_queue_session_pop(job_queue, job_queue_session, request_function_complete_job, test_request_context);
    int push_result = job_queue_push(job_queue, test_job, job_complete_callback, test_job_complete_callback_context);

    //assert
    ASSERT_ARE_EQUAL(int, pop_result, 0);
    ASSERT_ARE_EQUAL(int, push_result, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_2_sessions_pop_then_job_push)
{
    //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session_1 = job_queue_session_begin(job_queue);
    JOB_QUEUE_SESSION_HANDLE job_queue_session_2 = job_queue_session_begin(job_queue);

    //act
    int pop_result_1 = job_queue_session_pop(job_queue, job_queue_session_1, request_function_do_not_complete_job, test_request_context_2);
    int pop_result_2 = job_queue_session_pop(job_queue, job_queue_session_2, request_function_complete_job, test_request_context);
    int push_result = job_queue_push(job_queue, test_job, job_complete_callback, test_job_complete_callback_context);

    //assert
    ASSERT_ARE_EQUAL(int, pop_result_1, 0);
    ASSERT_ARE_EQUAL(int, pop_result_2, 0);
    ASSERT_ARE_EQUAL(int, push_result, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session_1);
    job_queue_session_end(job_queue, job_queue_session_2);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_1_job_pushed_then_session_get_and_pop)
{
        //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session = job_queue_session_begin(job_queue);

    //act
    int push_result = job_queue_push(job_queue, test_job, job_complete_callback, test_job_complete_callback_context);
    int get_result = job_queue_session_get(job_queue, job_queue_session, request_function_do_not_complete_job, test_request_context_2);
    int pop_result = job_queue_session_pop(job_queue, job_queue_session, request_function_complete_job, test_request_context);

    //assert
    ASSERT_ARE_EQUAL(int, push_result, 0);
    ASSERT_ARE_EQUAL(int, get_result, 0);
    ASSERT_ARE_EQUAL(int, pop_result, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session);
    job_queue_destroy(job_queue);
}

TEST_FUNCTION(test_job_pushed_then_session_gets_and_pops)
{
    //arrange
    JOB_QUEUE_HANDLE job_queue = job_queue_create(test_creation_context);
    JOB_QUEUE_SESSION_HANDLE job_queue_session = job_queue_session_begin(job_queue);

    //act
    int push_result = job_queue_push(job_queue, test_job, job_complete_callback, test_job_complete_callback_context);
    int get_result = job_queue_session_get(job_queue, job_queue_session, request_function_complete_job, test_request_context);
    int pop_result = job_queue_session_pop(job_queue, job_queue_session, request_function_do_not_complete_job, test_request_context_2);

    //assert
    ASSERT_ARE_EQUAL(int, push_result, 0);
    ASSERT_ARE_EQUAL(int, get_result, 0);
    ASSERT_ARE_EQUAL(int, pop_result, 0);

    //cleanup
    job_queue_session_end(job_queue, job_queue_session);
    job_queue_destroy(job_queue);
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
