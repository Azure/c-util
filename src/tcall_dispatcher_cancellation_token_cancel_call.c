// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"

#include "c_util/tcall_dispatcher.h"

#include "c_util/tcall_dispatcher_cancellation_token_cancel_call.h"

THANDLE_TYPE_DEFINE(TCALL_DISPATCHER_TYPEDEF_NAME(CANCELLATION_TOKEN_CANCEL_CALL));
TCALL_DISPATCHER_TYPE_DEFINE(CANCELLATION_TOKEN_CANCEL_CALL);
