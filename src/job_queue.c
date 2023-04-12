// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"
#include "c_pal/sm.h"

#include "c_util/singlylinkedlist.h"

#include "c_util/job_queue.h"

MU_DEFINE_ENUM_STRINGS(JOB_QUEUE_JOB_RESULT, JOB_QUEUE_JOB_RESULT_VALUES);
MU_DEFINE_ENUM_STRINGS(JOB_QUEUE_EXECUTE_JOB_RESULT, JOB_QUEUE_EXECUTE_JOB_RESULT_VALUES);

typedef struct JOB_QUEUE_TAG
{
    SINGLYLINKEDLIST_HANDLE jobs;
    SINGLYLINKEDLIST_HANDLE sessions;
    uint32_t session_count;
    SRW_LOCK_HANDLE lock;
    SM_HANDLE sm;

    const void* creation_context;
    LIST_ITEM_HANDLE next_to_complete;
} JOB_QUEUE;

typedef struct JOB_QUEUE_SESSION_TAG
{
    LIST_ITEM_HANDLE next_to_pop;
    LIST_ITEM_HANDLE next_to_get;
    SINGLYLINKEDLIST_HANDLE pending_get_requests;
    SINGLYLINKEDLIST_HANDLE pending_pop_requests;
    SM_HANDLE sm;
} JOB_QUEUE_SESSION;

typedef struct JOB_TAG
{
    void* job;
    uint32_t expected_pop_count;
    uint32_t current_pop_count;
    JOB_COMPLETE_CALLBACK job_complete_callback;
    void* job_complete_callback_context;
} JOB;

typedef struct JOB_QUEUE_REQUEST_TAG
{
    JOB_REQUEST_FUNCTION request_function;
    void* request_context;
} JOB_QUEUE_REQUEST;


JOB_QUEUE_HANDLE job_queue_create(const void* creation_context)
{
    JOB_QUEUE_HANDLE result;
    JOB_QUEUE_HANDLE job_queue = malloc(sizeof(JOB_QUEUE));
    if (job_queue == NULL)
    {
        LogError("malloc(sizeof(JOB_QUEUE)) failed");
        result = NULL;
    }
    else
    {
        job_queue->jobs = singlylinkedlist_create();
        if (job_queue->jobs == NULL)
        {
            LogError("Failure in singlylinkedlist_create()");
            result = NULL;
        }
        else
        {
            job_queue->sessions = singlylinkedlist_create();
            if (job_queue->sessions == NULL)
            {
                LogError("Failure in singlylinkedlist_create()");
                result = NULL;
            }
            else
            {
                job_queue->lock = srw_lock_create(false, "job_queue");
                if (job_queue->lock == NULL)
                {
                    LogError("Failure in srw_lock_create(false, \"job_queue\")");
                    result = NULL;
                }
                else
                {
                    job_queue->sm = sm_create("job_queue");
                    if (job_queue->sm == NULL)
                    {
                        LogError("Failure in sm_create(\"job_queue\")");
                        result = NULL;
                    }
                    else
                    {
                        (void)sm_open_begin(job_queue->sm);
                        sm_open_end(job_queue->sm, true);
                        job_queue->session_count = 0;
                        job_queue->creation_context = creation_context;
                        job_queue->next_to_complete = singlylinkedlist_get_head_item(job_queue->jobs);
                        result = job_queue;
                        goto all_ok;
                    }
                    sm_destroy(job_queue->sm);
                }
                singlylinkedlist_destroy(job_queue->sessions);
            }
            singlylinkedlist_destroy(job_queue->jobs);
        }
        free(result);
    }
all_ok:
    return result;
}



static bool remove_session(const void* item, const void* match_context, bool* continue_processing)
{
    bool result;
    if (item == NULL || match_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid arguments: const void* item=%p, const void* match_context=%p, bool* continue_processing=%p", item, match_context, continue_processing);
        result = false;
    }
    else
    {
        JOB_QUEUE_SESSION_HANDLE job_queue_session = (JOB_QUEUE_SESSION_HANDLE)item;
        JOB_QUEUE_SESSION_HANDLE job_queue_session_to_remove = (JOB_QUEUE_SESSION_HANDLE)match_context;
        if (job_queue_session == job_queue_session_to_remove)
        {
            *continue_processing = false;
            result = true;
        }
        else
        {
            *continue_processing = true;
            result = false;
        }
    }
    return result;
}

static void decrement_expected_pop_count(LIST_ITEM_HANDLE list_item, JOB_QUEUE_HANDLE job_queue)
{
    while (list_item != NULL)
    {
        bool updated_list_item = false;
        JOB* job = (JOB*)singlylinkedlist_item_get_value(list_item);
        job->expected_pop_count--;
        if (job->expected_pop_count == job->current_pop_count)
        {
            if (job != singlylinkedlist_item_get_value(job_queue->next_to_complete))
            {
                LogError("expected_pop_count == current_pop_count but the job is not the next to complete");
            }
            else
            {
                if (job->job_complete_callback != NULL)
                {
                    job->job_complete_callback(job->job_complete_callback_context, JOB_QUEUE_JOB_RESULT_ERROR);
                    job_queue->next_to_complete = singlylinkedlist_get_next_item(job_queue->next_to_complete);
                    free(job);
                    singlylinkedlist_remove(job_queue->jobs, list_item);
                    list_item = job_queue->next_to_complete;
                    updated_list_item = true;
                    sm_exec_end(job_queue->sm);
                }

            }
        }
        if (!updated_list_item)
        {
            list_item = singlylinkedlist_get_next_item(list_item);
        }
    }
}

static void drain_request(const void* item, const void* creation_context, bool* continue_processing)
{
    if (item == NULL || continue_processing == NULL)
    {
        LogError("Invalid arguments: const void* item=%p, const void* creation_context=%p, bool* continue_processing=%p", item, creation_context, continue_processing);
    }
    else
    {
        JOB_QUEUE_REQUEST* request = (JOB_QUEUE_REQUEST*)item;
        if (request->request_function != NULL)
        {
            request->request_function(creation_context, request->request_context, NULL);
        }
        free(request);
        *continue_processing = true;
    }
}

void job_queue_session_end_internal(JOB_QUEUE_HANDLE job_queue, JOB_QUEUE_SESSION_HANDLE session)
{
    if (
        job_queue == NULL ||
        session == NULL
        )
    {
        LogError("Invalid arg: JOB_QUEUE_HANDLE job_queue=%p, JOB_QUEUE_SESSION_HANDLE session=%p", job_queue, session);
    }
    else
    {
        SM_RESULT sm_close_result = sm_close_begin(session->sm);
        if (sm_close_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_close_begin(session->sm). SM_RESULT=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_close_result));
        }
        else
        {
            srw_lock_acquire_exclusive(job_queue->lock);
            if (singlylinkedlist_remove_if(job_queue->sessions, remove_session, session) != 0)
            {
                LogError("Failure in singlylinkedlist_remove_if(job_queue->sessions, remove_session, session)");
            }
            else
            {
                decrement_expected_pop_count(session->next_to_pop, job_queue);
                job_queue->session_count--;
            }
            srw_lock_release_exclusive(job_queue->lock);


            singlylinkedlist_foreach(session->pending_pop_requests, drain_request, job_queue->creation_context);
            singlylinkedlist_destroy(session->pending_pop_requests);

            singlylinkedlist_foreach(session->pending_get_requests, drain_request, job_queue->creation_context);
            singlylinkedlist_destroy(session->pending_get_requests);
            sm_close_end(session->sm);
            sm_exec_end(job_queue->sm);
        }
    }
}

void job_queue_session_end(JOB_QUEUE_HANDLE job_queue, JOB_QUEUE_SESSION_HANDLE session)
{
    if (
        job_queue == NULL ||
        session == NULL
        )
    {
        LogError("Invalid arg: JOB_QUEUE_HANDLE job_queue=%p, JOB_QUEUE_SESSION_HANDLE session=%p", job_queue, session);
    }
    else
    {
        job_queue_session_end_internal(job_queue, session);
        sm_destroy(session->sm);
        free(session);
    }
}

static void drain_session(const void* item, const void* action_context, bool* continue_processing)
{
    if (item == NULL || continue_processing == NULL)
    {
        LogError("Invalid arguments: const void* item=%p, const void* action_context=%p, bool* continue_processing=%p", item, action_context, continue_processing);
    }
    else
    {
        JOB_QUEUE_SESSION_HANDLE session = (JOB_QUEUE_SESSION_HANDLE)item;
        job_queue_session_end_internal((JOB_QUEUE_HANDLE)action_context, session);
        *continue_processing = true;
    }
}

static void drain_job(const void* item, const void* action_context, bool* continue_processing)
{
    if (item == NULL || action_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid arguments: const void* item=%p, const void* action_context=%p, bool* continue_processing=%p", item, action_context, continue_processing);
    }
    else
    {
        JOB* job = (JOB*)item;
        JOB_QUEUE_HANDLE job_queue = (JOB_QUEUE_HANDLE)action_context;
        if (job == singlylinkedlist_item_get_value(job_queue->next_to_complete))
        {
            if (job->job_complete_callback != NULL)
            {
                job->job_complete_callback(job->job_complete_callback_context, JOB_QUEUE_JOB_RESULT_ERROR);
            }
            job_queue->next_to_complete = singlylinkedlist_get_next_item(job_queue->next_to_complete);
        }
        sm_exec_end(job_queue->sm);
        free(job);
        *continue_processing = true;
    }
}

void job_queue_destroy(JOB_QUEUE_HANDLE job_queue)
{
    if (job_queue == NULL)
    {
        LogError("Invalid arg: JOB_QUEUE_HANDLE job_queue=%p", job_queue);
    }
    else
    {
        sm_close_begin(job_queue->sm);
        singlylinkedlist_foreach(job_queue->sessions, drain_session, job_queue);
        singlylinkedlist_foreach(job_queue->jobs, drain_job, job_queue);
        sm_close_end(job_queue->sm);

        sm_destroy(job_queue->sm);
        srw_lock_destroy(job_queue->lock);
        singlylinkedlist_destroy(job_queue->sessions);
        singlylinkedlist_destroy(job_queue->jobs);
        free(job_queue);
    }
}

static void increment_expected_pop_count(LIST_ITEM_HANDLE list_item)
{
    while (list_item != NULL)
    {
        JOB* job = (JOB*)singlylinkedlist_item_get_value(list_item);
        job->expected_pop_count++;
        list_item = singlylinkedlist_get_next_item(list_item);
    }
}

JOB_QUEUE_SESSION_HANDLE job_queue_session_begin(JOB_QUEUE_HANDLE job_queue)
{
    JOB_QUEUE_SESSION_HANDLE result;
    if (job_queue == NULL)
    {
        LogError("Invalid arg: JOB_QUEUE_HANDLE job_queue=%p", job_queue);
        result = NULL;
    }
    else
    {
        JOB_QUEUE_SESSION_HANDLE session = malloc(sizeof(JOB_QUEUE_SESSION));
        if (session == NULL)
        {
            LogError("Failure in malloc(sizeof(JOB_QUEUE_SESSION))");
            result = NULL;
        }
        else
        {
            session->pending_get_requests = singlylinkedlist_create();
            if (session->pending_get_requests == NULL)
            {
                LogError("Failure in singlylinkedlist_create()");
                result = NULL;
            }
            else
            {
                session->pending_pop_requests = singlylinkedlist_create();
                if (session->pending_pop_requests == NULL)
                {
                    LogError("Failure in singlylinkedlist_create()");
                    result = NULL;
                }
                else
                {
                    session->sm = sm_create("job_queue_session");
                    if (session->sm == NULL)
                    {
                        LogError("Failure in sm_create(\"job_queue_session\")");
                        result = NULL;
                    }
                    else
                    {
                        (void)sm_open_begin(session->sm);
                        sm_open_end(session->sm, true);

                        session->next_to_pop = job_queue->next_to_complete;
                        session->next_to_get = job_queue->next_to_complete;

                        SM_RESULT sm_exec_begin_result = sm_exec_begin(job_queue->sm);
                        if (sm_exec_begin_result != SM_EXEC_GRANTED)
                        {
                            LogError("Failure in sm_exec_begin(job_queue->sm), SM_RESULT sm_exec_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_exec_begin_result));
                            result = NULL;
                        }
                        else
                        {
                            srw_lock_acquire_exclusive(job_queue->lock);
                            if (singlylinkedlist_add(job_queue->sessions, session) == NULL)
                            {
                                LogError("Failure in singlylinkedlist_add(job_queue->sessions, session)");
                                result = NULL;
                            }
                            else
                            {
                                job_queue->session_count++;
                                increment_expected_pop_count(session->next_to_pop);
                                result = session;
                            }
                            srw_lock_release_exclusive(job_queue->lock);
                            if (result)
                            {
                                goto all_ok;
                            }
                        }
                        sm_destroy(session->sm);
                    }
                    singlylinkedlist_destroy(session->pending_pop_requests);
                }
                singlylinkedlist_destroy(session->pending_get_requests);
            }
            free(session);
        }
    }
all_ok:
    return result;
}

static void process_session_get_request(const void* item, const void* action_context, bool* continue_processing)
{
    if (item == NULL || action_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid arguments: const void* item=%p, const void* action_context=%p, bool* continue_processing=%p", item, action_context, continue_processing);
    }
    else
    {
        JOB_QUEUE_HANDLE job_queue = (JOB_QUEUE_HANDLE)action_context;
        JOB_QUEUE_SESSION_HANDLE session = (JOB_QUEUE_SESSION_HANDLE)item;
        LIST_ITEM_HANDLE request_list_item = singlylinkedlist_get_head_item(session->pending_get_requests);
        JOB_QUEUE_REQUEST* request = (JOB_QUEUE_REQUEST*)singlylinkedlist_item_get_value(request_list_item);
        if (request != NULL)
        {
            JOB* job = (JOB*)singlylinkedlist_item_get_value(session->next_to_get);
            if (job != NULL && request->request_function != NULL)
            {
                bool call_complete_callback = request->request_function(job_queue->creation_context, request->request_context, job->job);
                singlylinkedlist_remove(session->pending_get_requests, request_list_item);
                free(request);
                sm_exec_end(session->sm);
                if (call_complete_callback && job_queue->next_to_complete == session->next_to_get && job->job_complete_callback != NULL)
                {
                    job->job_complete_callback(job->job_complete_callback_context, JOB_QUEUE_JOB_RESULT_OK);
                    job->job_complete_callback = NULL;
                    job->job_complete_callback_context = NULL;
                    job_queue->next_to_complete = singlylinkedlist_get_next_item(job_queue->next_to_complete);
                }
                session->next_to_get = singlylinkedlist_get_next_item(session->next_to_get);
            }
        }
        *continue_processing = true;
    }
}

static void process_session_pop_request(const void* item, const void* action_context, bool* continue_processing)
{
    if (item == NULL || action_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid arguments: const void* item=%p, const void* action_context=%p, bool* continue_processing=%p", item, action_context, continue_processing);
    }
    else
    {
        JOB_QUEUE_HANDLE job_queue = (JOB_QUEUE_HANDLE)action_context;
        JOB_QUEUE_SESSION_HANDLE session = (JOB_QUEUE_SESSION_HANDLE)item;
        LIST_ITEM_HANDLE request_list_item = singlylinkedlist_get_head_item(session->pending_pop_requests);
        JOB_QUEUE_REQUEST* request = (JOB_QUEUE_REQUEST*)singlylinkedlist_item_get_value(request_list_item);
        if (request != NULL)
        {
            LIST_ITEM_HANDLE job_list_item = session->next_to_pop;
            JOB* job = (JOB*)singlylinkedlist_item_get_value(job_list_item);
            if (job != NULL && request->request_function != NULL)
            {
                bool call_complete_callback = request->request_function(job_queue->creation_context, request->request_context, job->job);
                singlylinkedlist_remove(session->pending_pop_requests, request_list_item);
                if (session->next_to_get == session->next_to_pop)
                {
                    session->next_to_get = singlylinkedlist_get_next_item(session->next_to_get);
                }
                session->next_to_pop = singlylinkedlist_get_next_item(session->next_to_pop);
                free(request);
                sm_exec_end(session->sm);
                if (call_complete_callback && job_queue->next_to_complete == session->next_to_pop && job->job_complete_callback != NULL)
                {
                    job->job_complete_callback(job->job_complete_callback_context, JOB_QUEUE_JOB_RESULT_OK);
                    job->job_complete_callback = NULL;
                    job->job_complete_callback_context = NULL;
                    job_queue->next_to_complete = singlylinkedlist_get_next_item(job_queue->next_to_complete);
                }
                job->current_pop_count++;
                if (job->current_pop_count == job->expected_pop_count)
                {
                    if (job_list_item != singlylinkedlist_get_head_item(job_queue->jobs))
                    {
                        LogError("Cannot pop from middle of queue.");
                    }
                    else
                    {
                        singlylinkedlist_remove(job_queue->jobs, job_list_item);
                        sm_exec_end(job_queue->sm);
                        free(job);
                    }
                }
            }
        }
        *continue_processing = true;
    }
}

static void process_requests(JOB_QUEUE_HANDLE job_queue)
{
    if (job_queue == NULL)
    {
        LogError("Invalid arguments: JOB_QUEUE_HANDLE job_queue=%p", job_queue);
    }
    else
    {
        singlylinkedlist_foreach(job_queue->sessions, process_session_get_request, job_queue);
        singlylinkedlist_foreach(job_queue->sessions, process_session_pop_request, job_queue);
    }
}

static void update_next_job(const void* item, const void* action_context, bool* continue_processing)
{
    if (item == NULL || action_context == NULL || continue_processing == NULL)
    {
        LogError("Invalid arguments: const void* item=%p, const void* action_context=%p, bool* continue_process=%p", item, action_context, continue_processing);
    }
    else
    {
        JOB_QUEUE_SESSION_HANDLE session = (JOB_QUEUE_SESSION_HANDLE)item;
        LIST_ITEM_HANDLE next_list_item = (LIST_ITEM_HANDLE)action_context;
        if (session->next_to_get == NULL)
        {
            session->next_to_get = next_list_item;
        }
        if (session->next_to_pop == NULL)
        {
            session->next_to_pop = next_list_item;
        }
        *continue_processing = true;
    }
}

int job_queue_push(JOB_QUEUE_HANDLE job_queue, void* user_job, JOB_COMPLETE_CALLBACK job_complete_callback, void* job_complete_callback_context)
{
    int result;
    if (
        job_queue == NULL ||
        user_job == NULL ||
        job_complete_callback == NULL
        )
    {
        LogError("Invalid arg: JOB_QUEUE_HANDLE job_queue=%p, void* user_job=%p, JOB_COMPLETE_CALLBACK job_complete_callback=%p, void* job_complete_callback_context=%p", job_queue, user_job, job_complete_callback, job_complete_callback_context);
        result = MU_FAILURE;
    }
    else
    {
        SM_RESULT sm_exec_begin_result = sm_exec_begin(job_queue->sm);
        if (sm_exec_begin_result != SM_EXEC_GRANTED)
        {
            LogError("Failure in sm_exec_begin(job_queue->sm), SM_RESULT sm_exec_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_exec_begin_result));
            result = MU_FAILURE;
        }
        else
        {
            JOB* job = malloc(sizeof(JOB));
            if (job == NULL)
            {
                LogError("Failure in malloc(sizeof(JOB))");
                result = MU_FAILURE;
            }
            else
            {
                job->job = user_job;
                job->job_complete_callback = job_complete_callback;
                job->job_complete_callback_context = job_complete_callback_context;

                srw_lock_acquire_exclusive(job_queue->lock);
                job->expected_pop_count = job_queue->session_count;
                job->current_pop_count = 0;
                LIST_ITEM_HANDLE added_item = singlylinkedlist_add(job_queue->jobs, job);
                if (added_item == NULL)
                {
                    LogError("Failure in singlylinkedlist_add(job_queue->jobs, job)");
                    result = MU_FAILURE;
                }
                else
                {
                    if (job_queue->next_to_complete == NULL)
                    {
                        job_queue->next_to_complete = added_item;
                    }
                    singlylinkedlist_foreach(job_queue->sessions, update_next_job, added_item);
                    result = 0;
                }
                process_requests(job_queue);
                srw_lock_release_exclusive(job_queue->lock);


                if (result == 0)
                {
                    goto all_ok;
                }
                free(job);
            }
            sm_exec_end(job_queue->sm);
        }
    }
all_ok:
    return result;
}


int job_queue_session_pop(JOB_QUEUE_HANDLE job_queue, JOB_QUEUE_SESSION_HANDLE job_queue_session, JOB_REQUEST_FUNCTION job_request_function, void* job_request_context)
{
    int result;
    if (job_queue == NULL || job_queue_session == NULL || job_request_function == NULL)
    {
        LogError("Invalid arguments: JOB_QUEUE_HANDLE job_queue=%p, JOB_QUEUE_SESSION_HANDLE job_queue_session=%p, JOB_REQUEST_FUNCTION job_request_function=%p, void* job_request_context=%p", job_queue, job_queue_session, job_request_function, job_request_context);
        result = MU_FAILURE;
    }
    else
    {
        JOB_QUEUE_REQUEST* request = malloc(sizeof(JOB_QUEUE_REQUEST));
        if (request == NULL)
        {
            LogError("Failure in malooc(sizeof(JOB_QUEUE_REQUEST))");
            result = MU_FAILURE;
        }
        else
        {
            request->request_function = job_request_function;
            request->request_context = job_request_context;

            SM_RESULT sm_exec_begin_result = sm_exec_begin(job_queue_session->sm);
            if (sm_exec_begin_result != SM_EXEC_GRANTED)
            {
                LogError("Failure in sm_exec_begin(job_queue_session->sm), SM_RESULT sm_exec_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_exec_begin_result));
                result = MU_FAILURE;
            }
            else
            {
                srw_lock_acquire_exclusive(job_queue->lock);
                if (singlylinkedlist_add(job_queue_session->pending_pop_requests, request) == NULL)
                {
                    LogError("Failure in singlylinkedlist_add(job_queue_session->pending_pop_requests, request)");
                    result = MU_FAILURE;
                }
                else
                {
                    bool continue_processing;
                    process_session_pop_request(job_queue_session, job_queue, &continue_processing);
                    result = 0;
                }
                srw_lock_release_exclusive(job_queue->lock);
                if (result == 0)
                {
                    goto all_ok;
                }
                sm_exec_end(job_queue_session->sm);
            }
        }
    }
all_ok:
    return result;
}

int job_queue_session_get(JOB_QUEUE_HANDLE job_queue, JOB_QUEUE_SESSION_HANDLE job_queue_session, JOB_REQUEST_FUNCTION job_request_function, void* job_request_context)
{
    int result;
    if (job_queue == NULL || job_queue_session == NULL || job_request_function == NULL)
    {
        LogError("Invalid arguments: JOB_QUEUE_HANDLE job_queue=%p, JOB_QUEUE_SESSION_HANDLE job_queue_session=%p, JOB_REQUEST_FUNCTION job_request_function=%p, void* job_request_context=%p", job_queue, job_queue_session, job_request_function, job_request_context);
        result = MU_FAILURE;
    }
    else
    {
        JOB_QUEUE_REQUEST* request = malloc(sizeof(JOB_QUEUE_REQUEST));
        if (request == NULL)
        {
            LogError("Failure in malooc(sizeof(JOB_QUEUE_REQUEST))");
            result = MU_FAILURE;
        }
        else
        {
            request->request_function = job_request_function;
            request->request_context = job_request_context;

            SM_RESULT sm_exec_begin_result = sm_exec_begin(job_queue_session->sm);
            if (sm_exec_begin_result != SM_EXEC_GRANTED)
            {
                LogError("Failure in sm_exec_begin(job_queue_session->sm), SM_RESULT sm_exec_begin_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(SM_RESULT, sm_exec_begin_result));
                result = MU_FAILURE;
            }
            else
            {
                srw_lock_acquire_exclusive(job_queue->lock);
                if (singlylinkedlist_add(job_queue_session->pending_get_requests, request) == NULL)
                {
                    LogError("Failure in singlylinkedlist_add(job_queue_session->pending_get_requests, request)");
                    result = MU_FAILURE;
                }
                else
                {
                    bool continue_processing;
                    process_session_get_request(job_queue_session, job_queue, &continue_processing);
                    result = 0;
                }
                srw_lock_release_exclusive(job_queue->lock);
                if (result == 0)
                {
                    goto all_ok;
                }
                sm_exec_end(job_queue_session->sm);
            }
        }
    }
all_ok:
    return result;
}
