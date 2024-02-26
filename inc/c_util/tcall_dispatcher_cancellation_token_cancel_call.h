// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

#ifndef TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_H
#define TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "c_util/tcall_dispatcher.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

TCALL_DISPATCHER_DEFINE_CALL_TYPE(CANCELLATION_TOKEN_CANCEL_CALL);
THANDLE_TYPE_DECLARE(TCALL_DISPATCHER_TYPEDEF_NAME(CANCELLATION_TOKEN_CANCEL_CALL));
TCALL_DISPATCHER_TYPE_DECLARE(CANCELLATION_TOKEN_CANCEL_CALL);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // TCALL_DISPATCHER_CANCELLATION_TOKEN_CANCEL_CALL_H
