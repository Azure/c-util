// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdalign.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/thandle.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/channel.h"

#define CHANNEL_STATE_VALUES \
    CHANNEL_READY, \
    CHANNEL_PUSHING, \
    CHANNEL_PUSHED, \
    CHANNEL_PULLING, \
    CHANNEL_PULLED

MU_DEFINE_ENUM(CHANNEL_STATE, CHANNEL_STATE_VALUES)

#define CHANNEL_EPOCH_INCREMENT (1 << 3)

#define CHANNEL_STATE_MASK ((1 << 3) - 1)

struct CHANNEL_TAG{
    union
    {
        PULL_CALLBACK pull_callback;
        PUSH_CALLBACK push_callback;
    };
    void* context;
    THANDLE(RC_PTR) data;
    volatile_atomic int32_t state;
};

THANDLE_TYPE_DEFINE(CHANNEL);

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create)
{
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull)
{
    (void)channel;
    (void)pull_callback;
    (void)pull_context;
    (void)out_op_pull;
    return CHANNEL_RESULT_ERROR;
}

IMPLEMENT_MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push)
{
    (void)channel;
    (void)data;
    (void)push_callback;
    (void)push_context;
    (void)out_op_push;
    return CHANNEL_RESULT_ERROR;
}