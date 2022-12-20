// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define sm_create               real_sm_create
#define sm_destroy              real_sm_destroy
#define sm_open_begin           real_sm_open_begin
#define sm_open_end             real_sm_open_end
#define sm_close_begin          real_sm_close_begin
#define sm_close_begin_with_cb  real_sm_close_begin_with_cb
#define sm_close_end            real_sm_close_end
#define sm_exec_begin           real_sm_exec_begin
#define sm_exec_end             real_sm_exec_end
#define sm_barrier_begin        real_sm_barrier_begin
#define sm_barrier_end          real_sm_barrier_end
#define sm_fault                real_sm_fault

#define SM_RESULT               real_SM_RESULT
#define SM_STATE                real_SM_STATE
