// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "macro_utils/macro_utils.h"

#define JOB_QUEUE_JOB_RESULT_VALUES \
    JOB_QUEUE_JOB_RESULT_OK, \
    JOB_QUEUE_JOB_RESULT_ERROR

MU_DEFINE_ENUM(JOB_QUEUE_JOB_RESULT, JOB_QUEUE_JOB_RESULT_VALUES);

#define JOB_QUEUE_EXECUTE_JOB_RESULT_VALUES \
    JOB_QUEUE_EXECUTE_JOB_RESULT_OK, \
    JOB_QUEUE_EXECUTE_JOB_RESULT_ERROR

MU_DEFINE_ENUM(JOB_QUEUE_EXECUTE_JOB_RESULT, JOB_QUEUE_EXECUTE_JOB_RESULT_VALUES);

typedef struct JOB_QUEUE_TAG* JOB_QUEUE_HANDLE;
typedef struct JOB_QUEUE_SESSION_TAG* JOB_QUEUE_SESSION_HANDLE;

typedef void(*JOB_COMPLETE_CALLBACK)(void* context, JOB_QUEUE_JOB_RESULT result);
typedef bool(*JOB_REQUEST_FUNCTION)(const void* creation_context, void* job_request_context, void* job);

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, JOB_QUEUE_HANDLE, job_queue_create, const void*, creation_context);
    MOCKABLE_FUNCTION(, void, job_queue_destroy, JOB_QUEUE_HANDLE, job_queue);
    MOCKABLE_FUNCTION(, int, job_queue_push, JOB_QUEUE_HANDLE, job_queue, void*, job, JOB_COMPLETE_CALLBACK, job_complete_callback, void*, job_complete_callback_context);

    MOCKABLE_FUNCTION(, JOB_QUEUE_SESSION_HANDLE, job_queue_session_begin, JOB_QUEUE_HANDLE, job_queue);
    MOCKABLE_FUNCTION(, void, job_queue_session_end, JOB_QUEUE_HANDLE, job_queue, JOB_QUEUE_SESSION_HANDLE, job_queue_session);
    MOCKABLE_FUNCTION(, int, job_queue_session_pop, JOB_QUEUE_HANDLE, job_queue, JOB_QUEUE_SESSION_HANDLE, job_queue_session, JOB_REQUEST_FUNCTION, job_request_function, void*, job_request_context);
    MOCKABLE_FUNCTION(, int, job_queue_session_get, JOB_QUEUE_HANDLE, job_queue, JOB_QUEUE_SESSION_HANDLE, job_queue_session, JOB_REQUEST_FUNCTION, job_request_function, void*, job_request_context);


#ifdef __cplusplus
}
#endif

#endif /*JOB_QUEUE_H*/


