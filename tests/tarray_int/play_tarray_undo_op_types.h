// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*PLAY is a module that pretends to be a production module that will embed a TARRAY in its handle*/

#ifndef PLAY_TARRAY_UNDO_OP_TYPES_H
#define PLAY_TARRAY_UNDO_OP_TYPES_H

#include "c_util/tarray_ll.h"

#include "play_undo_op_types.h"

TARRAY_DEFINE_STRUCT_TYPE(UNDO_OP);
THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(UNDO_OP));

#endif /*PLAY_TARRAY_UNDO_OP_TYPES_H*/


