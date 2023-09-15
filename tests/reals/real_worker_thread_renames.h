// Copyright (c) Microsoft. All rights reserved.

#define worker_thread_create                                        real_worker_thread_create
#define worker_thread_destroy                                       real_worker_thread_destroy
#define worker_thread_schedule_process                              real_worker_thread_schedule_process
#define worker_thread_open                                          real_worker_thread_open
#define worker_thread_close                                         real_worker_thread_close

#define WORKER_THREAD_SCHEDULE_PROCESS_RESULT                       real_WORKER_THREAD_SCHEDULE_PROCESS_RESULT
#define WORKER_THREAD_STATE                                         real_WORKER_THREAD_STATE
