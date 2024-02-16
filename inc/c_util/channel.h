// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CHANNEL_H
#define CHANNEL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"

#include "c_util/async_op.h"
#include "c_util/rc_ptr.h"

#include "c_util/channel_common.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct CHANNEL_TAG CHANNEL;

THANDLE_TYPE_DECLARE(CHANNEL);

    MOCKABLE_FUNCTION(, THANDLE(CHANNEL), channel_create, THANDLE(THREADPOOL), threadpool);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_pull, THANDLE(CHANNEL), channel, PULL_CALLBACK, pull_callback, void*, pull_context, THANDLE(ASYNC_OP)*, out_op_pull);
    MOCKABLE_FUNCTION(, CHANNEL_RESULT, channel_push, THANDLE(CHANNEL), channel, THANDLE(RC_PTR), data, PUSH_CALLBACK, push_callback, void*, push_context, THANDLE(ASYNC_OP)*, out_op_push);
    MOCKABLE_FUNCTION(, void, channel_reset, THANDLE(CHANNEL), channel);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CHANNEL_H */
