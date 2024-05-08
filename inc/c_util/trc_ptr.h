// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TRC_PTR_H
#define TRC_PTR_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "c_pal/thandle_ll.h"

#include "trc_ptr_ll.h"

#define PRI_TRC_PTR "p"
#define TRC_PTR_OR_NULL(rc) (((rc) == NULL) ? NULL : (rc)->ptr)

/* Use this type to refer to the THANDLE-wrapped type which encapsulates the pointer. */
#define TRC_PTR(T) THANDLE(TRC_PTR_STRUCT(T))
/* Access T* stored in TRC_PTR(T) */
#define TRC_PTR_VALUE(rc) ((rc)->ptr)

/* Create a new TRC_PTR(T) from a pointer and dispose function */
#define TRC_PTR_CREATE_WITH_MOVE_POINTER(T) TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(T)
/* Remap THANDLE_* functions to TRC_PTR names */
#define TRC_PTR_ASSIGN(T) THANDLE_ASSIGN(TRC_PTR_STRUCT(T))
#define TRC_PTR_INITIALIZE(T) THANDLE_INITIALIZE(TRC_PTR_STRUCT(T))
#define TRC_PTR_MOVE(T) THANDLE_MOVE(TRC_PTR_STRUCT(T))
#define TRC_PTR_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(TRC_PTR_STRUCT(T))

/* Declare a TRC_PTR type */
#define TRC_PTR_DECLARE(T) \
    TYPED_RC_PTR_LL_DECLARE(T, T)

/* Define a TRC_PTR type */
#define TRC_PTR_DEFINE(T) \
    TYPED_RC_PTR_LL_DEFINE(T, T)

#endif  /* TRC_PTR_H */
