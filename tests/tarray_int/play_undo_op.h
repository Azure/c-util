// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*PLAY is a module that pretends to be a production module that will embed a TARRAY in its handle*/

#ifndef PLAY_UNDO_OP_H
#define PLAY_UNDO_OP_H

#include "macro_utils/macro_utils.h"

#include "c_util/tarray.h"

#include "play_undo_op_types.h"
#include "play_tarray_undo_op_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "umock_c/umock_c_prod.h"

    TARRAY_TYPE_DECLARE(UNDO_OP);

#ifdef __cplusplus
}
#endif

#endif /*PLAY_UNDO_OP_H*/


