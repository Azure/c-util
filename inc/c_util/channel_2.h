// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CHANNEL_H
#define CHANNEL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/thandle_log_context_handle.h"

#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"


#define CHANNEL_2_RESULT_VALUES \
    CHANNEL_2_RESULT_OK, \
    CHANNEL_2_RESULT_INVALID_ARGS, \
    CHANNEL_2_RESULT_ERROR

MU_DEFINE_ENUM(CHANNEL_2_RESULT, CHANNEL_2_RESULT_VALUES);

#define CHANNEL_2_CALLBACK_RESULT_VALUES \
    CHANNEL_2_CALLBACK_RESULT_OK, \
    CHANNEL_2_CALLBACK_RESULT_ABANDONED

MU_DEFINE_ENUM(CHANNEL_2_CALLBACK_RESULT, CHANNEL_2_CALLBACK_RESULT_VALUES);

typedef void(*CHANNEL_2_PULL_CALLBACK)(void* pull_context, CHANNEL_2_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data);
typedef void(*CHANNEL_2_PUSH_CALLBACK)(void* push_context, CHANNEL_2_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id);

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct CHANNEL_2_TAG CHANNEL_2;

THANDLE_TYPE_DECLARE(CHANNEL_2);

    MOCKABLE_FUNCTION(, THANDLE(CHANNEL_2), channel_2_create_and_open, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool);
    MOCKABLE_FUNCTION(, void, channel_2_close, THANDLE(CHANNEL_2), channel_2);
    MOCKABLE_FUNCTION(, int, channel_2_reset, THANDLE(CHANNEL_2), channel_2);
    MOCKABLE_FUNCTION(, CHANNEL_2_RESULT, channel_2_pull, THANDLE(CHANNEL_2), channel_2, THANDLE(RC_STRING), correlation_id, CHANNEL_2_PULL_CALLBACK, pull_callback, void*, pull_context);
    MOCKABLE_FUNCTION(, CHANNEL_2_RESULT, channel_2_push, THANDLE(CHANNEL_2), channel_2, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, CHANNEL_2_PUSH_CALLBACK, push_callback, void*, push_context);

#ifdef __cplusplus
}
#endif /* __cplusplus */



#endif /* CHANNEL_H */
