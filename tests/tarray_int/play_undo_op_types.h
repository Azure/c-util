// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*PLAY is a module that pretends to be a production module that will embed a TARRAY in its handle*/

#ifndef PLAY_UNDO_OP_TYPES_H
#define PLAY_UNDO_OP_TYPES_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

typedef struct UNDO_OP_TAG
{
    uint32_t* target;
    uint32_t old;
}UNDO_OP;

TARRAY_DEFINE_STRUCT_TYPE(UNDO_OP);

#endif /*PLAY_UNDO_OP_TYPES_H*/


