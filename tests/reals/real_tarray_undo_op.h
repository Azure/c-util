// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_TARRAY_UNDO_OP_H
#define REAL_TARRAY_UNDO_OP_H

#include "macro_utils/macro_utils.h"

#include "c_util/tarray_ll.h"
#include "c_util/tarray.h"

#include "../tarray_int/play_undo_op_types.h"
#include "../tarray_int/play_undo_op_tarray_types.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_TARRAY_UNDO_OP_GLOBAL_MOCK_HOOK() \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_ENSURE_CAPACITY(UNDO_OP), TARRAY_LL_ENSURE_CAPACITY(real_UNDO_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_CREATE(UNDO_OP), TARRAY_LL_CREATE(real_UNDO_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_MOVE(UNDO_OP), TARRAY_LL_MOVE(real_UNDO_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_INITIALIZE(UNDO_OP), TARRAY_LL_INITIALIZE(real_UNDO_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_INITIALIZE_MOVE(UNDO_OP), TARRAY_LL_INITIALIZE_MOVE(real_UNDO_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_INC_REF(UNDO_OP), TARRAY_LL_INC_REF(real_UNDO_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_DEC_REF(UNDO_OP), TARRAY_LL_DEC_REF(real_UNDO_OP)) \
    REGISTER_GLOBAL_MOCK_HOOK(TARRAY_ASSIGN(UNDO_OP), TARRAY_LL_ASSIGN(real_UNDO_OP)) \

typedef UNDO_OP real_UNDO_OP;

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

    TARRAY_LL_TYPE_DECLARE(real_UNDO_OP, UNDO_OP);

#ifdef __cplusplus
}
#endif

#endif //REAL_TARRAY_UNDO_OP_H
