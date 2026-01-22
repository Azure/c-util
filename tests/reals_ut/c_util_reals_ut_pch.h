// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Precompiled header for c_util_reals_ut

#ifndef C_UTIL_REALS_UT_PCH_H
#define C_UTIL_REALS_UT_PCH_H

#include "testrunnerswitcher.h"

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "../reals/real_async_op.h"
#include "../reals/real_filename_helper.h"
#include "../reals/real_channel.h"
#include "../reals/real_constbuffer.h"
#include "../reals/real_constbuffer_array.h"
#include "../reals/real_constbuffer_array_sync_wrapper.h"
#include "../reals/real_constbuffer_array_batcher_nv.h"
#include "../reals/real_critical_section.h"
#include "../reals/real_doublylinkedlist.h"
#include "../reals/real_external_command_helper.h"
#include "../reals/real_hash.h"
#include "../reals/real_memory_data.h"
#include "../reals/real_rc_ptr.h"
#include "../reals/real_rc_string.h"
#include "../reals/real_rc_string_array.h"
#include "../reals/real_rc_string_utils.h"
#include "../reals/real_singlylinkedlist.h"
#include "../reals/real_tcall_dispatcher_cancellation_token_cancel_call.h"
#include "../reals/real_cancellation_token.h"
#include "../reals/real_uuid_string.h"

#include "c_util/async_op.h"
#include "c_util/channel.h"
#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"
#include "c_util/constbuffer_array_sync_wrapper.h"
#include "c_util/constbuffer_array_batcher_nv.h"
#include "c_util/critical_section.h"
#include "c_util/doublylinkedlist.h"
#include "c_util/external_command_helper.h"
#include "c_util/hash.h"
#include "c_util/memory_data.h"
#include "c_util/rc_ptr.h"
#include "c_util/rc_string.h"
#include "c_util/rc_string_array.h"
#include "c_util/rc_string_utils.h"
#include "c_util/singlylinkedlist.h"
#include "c_util/uuid_string.h"
#include "c_util/cancellation_token.h"

#if defined _MSC_VER
#include "../reals/real_object_lifetime_tracker.h"
#include "../reals/real_worker_thread.h"
#include "../reals/real_tcall_dispatcher_thread_notification_call.h"
#include "../reals/real_thread_notifications_dispatcher.h"
#include "../reals/real_thread_notifications_lackey_dll.h"

#include "c_util/object_lifetime_tracker.h"
#include "c_util/worker_thread.h"
#include "c_util/tcall_dispatcher_thread_notification_call.h"
#include "c_util/thread_notifications_dispatcher.h"
#endif

#endif // C_UTIL_REALS_UT_PCH_H
