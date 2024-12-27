// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"
#include "c_pal/srw_lock.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"

#include "c_util/channel_2.h"

typedef struct CHANNEL_2_TAG
{
    int placeholder;
}CHANNEL_2;

THANDLE(CHANNEL_2) channel_2_create_and_open(THANDLE(PTR(LOG_CONTEXT_HANDLE)) log_context, THANDLE(THREADPOOL) threadpool)
{
    (void)log_context;
    (void)threadpool;
    return NULL;
}


void channel_2_close(THANDLE(CHANNEL_2) channel_2)
{
    (void)channel_2;
}

int channel_2_reset(THANDLE(CHANNEL_2) channel_2)
{
    (void)channel_2;
    return 0;
}

CHANNEL_2_RESULT channel_2_pull(THANDLE(CHANNEL_2) channel_2, THANDLE(RC_STRING) correlation_id, CHANNEL_2_PULL_CALLBACK pull_callback, void* pull_context)
{
    (void)channel_2;
    (void)correlation_id;
    (void)pull_callback;
    (void)pull_context;
    return CHANNEL_2_RESULT_OK;
}

CHANNEL_2_RESULT channel_2_push(THANDLE(CHANNEL_2) channel_2, THANDLE(RC_STRING) correlation_id, THANDLE(RC_PTR) data, CHANNEL_2_PUSH_CALLBACK push_callback, void* push_context)
{
    (void)channel_2;
    (void)correlation_id;
    (void)data;
    (void)push_callback;
    (void)push_context;
    return CHANNEL_2_RESULT_OK;
}
