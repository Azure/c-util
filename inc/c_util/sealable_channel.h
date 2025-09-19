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

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SEALABLE_CHANNEL_RESULT_VALUES \
    SEALABLE_CHANNEL_RESULT_OK, \
    SEALABLE_CHANNEL_RESULT_INVALID_ARGS, \
    SEALABLE_CHANNEL_RESULT_ERROR, \
    SEALABLE_CHANNEL_RESULT_SEALED

MU_DEFINE_ENUM(SEALABLE_CHANNEL_RESULT, SEALABLE_CHANNEL_RESULT_VALUES);

#define SEALABLE_CHANNEL_CALLBACK_RESULT_VALUES \
    SEALABLE_CHANNEL_CALLBACK_RESULT_OK, \
    SEALABLE_CHANNEL_CALLBACK_RESULT_CANCELLED, \
    SEALABLE_CHANNEL_CALLBACK_RESULT_ABANDONED

MU_DEFINE_ENUM(SEALABLE_CHANNEL_CALLBACK_RESULT, SEALABLE_CHANNEL_CALLBACK_RESULT_VALUES);

typedef void(*ON_SEALABLE_CHANNEL_DATA_AVAILABLE_CB)(void* pull_context, SEALABLE_CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id, THANDLE(RC_PTR) data);
typedef void(*ON_SEALABLE_CHANNEL_DATA_CONSUMED_CB)(void* push_context, SEALABLE_CHANNEL_CALLBACK_RESULT result, THANDLE(RC_STRING) pull_correlation_id, THANDLE(RC_STRING) push_correlation_id);

typedef struct SEALABLE_CHANNEL_REPORTING_INFO_TAG
{
    uint32_t current_channel_item_count;
    size_t current_channel_size;
} SEALABLE_CHANNEL_REPORTING_INFO;

typedef struct SEALABLE_CHANNEL_TAG SEALABLE_CHANNEL;

THANDLE_TYPE_DECLARE(SEALABLE_CHANNEL);

    MOCKABLE_FUNCTION(, THANDLE(SEALABLE_CHANNEL), sealable_channel_create, THANDLE(PTR(LOG_CONTEXT_HANDLE)), log_context, THANDLE(THREADPOOL), threadpool, uint32_t, max_channel_item_count, size_t, max_channel_item_size);
    MOCKABLE_FUNCTION(, int, sealable_channel_open, THANDLE(SEALABLE_CHANNEL), sealable_channel);
    MOCKABLE_FUNCTION(, void, sealable_channel_close, THANDLE(SEALABLE_CHANNEL), sealable_channel);
    MOCKABLE_FUNCTION(, SEALABLE_CHANNEL_RESULT, sealable_channel_pull, THANDLE(SEALABLE_CHANNEL), sealable_channel, THANDLE(RC_STRING), correlation_id, ON_SEALABLE_CHANNEL_DATA_AVAILABLE_CB, on_data_available_cb, void*, pull_context);
    MOCKABLE_FUNCTION(, SEALABLE_CHANNEL_RESULT, sealable_channel_push, THANDLE(SEALABLE_CHANNEL), sealable_channel, THANDLE(RC_STRING), correlation_id, THANDLE(RC_PTR), data, ON_SEALABLE_CHANNEL_DATA_CONSUMED_CB, on_data_consumed_cb, void*, push_context);
    MOCKABLE_FUNCTION(, int, sealable_channel_get_reporting_info, THANDLE(SEALABLE_CHANNEL), sealable_channel, SEALABLE_CHANNEL_REPORTING_INFO*, reporting_info);
    MOCKABLE_FUNCTION(, int, sealable_channel_seal_channel, THANDLE(SEALABLE_CHANNEL), sealable_channel);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CHANNEL_H */
