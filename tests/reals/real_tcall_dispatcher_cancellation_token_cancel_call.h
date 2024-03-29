// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_H
#define REAL_TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_H

#include "macro_utils/macro_utils.h"

#include "c_util/tcall_dispatcher_ll.h"
#include "c_util/tcall_dispatcher.h"

#include "../../inc/c_util/tcall_dispatcher_cancellation_token_cancel_call.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_GLOBAL_MOCK_HOOK() \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_REGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_REGISTER_TARGET(real_CANCELLATION_TOKEN_CANCEL_CALL)) \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_UNREGISTER_TARGET(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_UNREGISTER_TARGET(real_CANCELLATION_TOKEN_CANCEL_CALL)) \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_DISPATCH_CALL(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_DISPATCH_CALL(real_CANCELLATION_TOKEN_CANCEL_CALL)) \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_CREATE(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_CREATE(real_CANCELLATION_TOKEN_CANCEL_CALL)) \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_MOVE(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_MOVE(real_CANCELLATION_TOKEN_CANCEL_CALL)) \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_INITIALIZE(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_INITIALIZE(real_CANCELLATION_TOKEN_CANCEL_CALL)) \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_INITIALIZE_MOVE(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_INITIALIZE_MOVE(real_CANCELLATION_TOKEN_CANCEL_CALL)) \
    REGISTER_GLOBAL_MOCK_HOOK(TCALL_DISPATCHER_ASSIGN(CANCELLATION_TOKEN_CANCEL_CALL), TCALL_DISPATCHER_ASSIGN(real_CANCELLATION_TOKEN_CANCEL_CALL)) \

#include "umock_c/umock_c_prod.h"

TCALL_DISPATCHER_LL_TYPE_DECLARE(real_CANCELLATION_TOKEN_CANCEL_CALL, CANCELLATION_TOKEN_CANCEL_CALL);

#endif // REAL_TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_H
