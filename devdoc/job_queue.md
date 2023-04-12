# `job_queue` requirements

`job_queue` is a module that allows jobs to be executed asynchronously. It queues jobs when there are no requests and queues requests when there are no jobs. When a job arrives and there are queued requests, it executes the least recently queued request with the job. When a request arrives and there are queued jobs, it executes the request with the least recently queued job.

Consumers of `job_queue` must create a "`session`" before they can queue requests. A session can make two kinds of requests:

- `get` requests
- `pop` requests

The consumer provides a `JOB_REQUEST_FUNCTION` that is executed when that request is executed. If the `JOB_REQUEST_FUNCTION` returns `true`, that job is considered completed, and it's  `JOB_COMPLETE_CALLBACK` is called. Once a job is completed, any new sessions that will be started will not see the completed job.

Even after a job is completed, it is not discarded. A job is discarded when all the sessions that were active when the job was pushed have popped it. If all the sessions that were active when the job was queued have popped a job without indicating that the job is complete, the `JOB_COMPLETE_CALLBACK` is called with `JOB_QUEUE_JOB_RESULT_ERROR` as the result.

Sessions make progress independent of each other. It is possible that one session has popped all the queued requests, whereas another session has not popped any. A session can be thought of as an independent "view" of the job queue. Once a session has called `pop` on a job, it cannot call `pop` or `get` on the same job again. If a session has called `get` on a job, it can call `pop` on that job but cannot call `get` on that `job`.

A producer of jobs must call `push` to queue jobs. All jobs are queued in a single queue and executed in FIFO order. The producer provides a `JOB_COMPLETE_CALLBACK` to be called when the job is completed. If a job is completed because a request marked it as completed (by returning `true` from `JOB_REQUEST_FUNCTION`), `JOB_COMPLETE_CALLBACK` is called with `JOB_QUEUE_JOB_RESULT_OK` as result. In any other case (session ends before job can be completed, job_queue is closing etc), the `JOB_COMPLETE_CALLBACK` is called with `JOB_QUEUE_JOB_RESULT_ERROR`.

## Exposed API

```c
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
```

