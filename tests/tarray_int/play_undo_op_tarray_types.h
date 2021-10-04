// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*PLAY is a module that pretends to be a production module that will embed a TARRAY in its handle*/

#ifndef PLAY_TARRAY_UNDO_OP_TYPES_H
#define PLAY_TARRAY_UNDO_OP_TYPES_H

#include "c_util/thandle_ll.h"

#include "play_undo_op_types.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C"
{
#endif

    THANDLE_LL_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(UNDO_OP), TARRAY_TYPEDEF_NAME(UNDO_OP));

#ifdef __cplusplus
}
#endif

#endif /*PLAY_TARRAY_UNDO_OP_TYPES_H*/


