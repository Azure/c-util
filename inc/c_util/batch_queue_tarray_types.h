// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef BATCH_QUEUE_TARRAY_TYPES_H
#define BATCH_QUEUE_TARRAY_TYPES_H


#include "c_pal/thandle.h"
#include "c_util/tarray.h"

typedef struct BATCH_ITEM_CONTEXT_TAG* BATCH_ITEM_CONTEXT_HANDLE;

TARRAY_DEFINE_STRUCT_TYPE(BATCH_ITEM_CONTEXT_HANDLE);

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C"
{
#endif

    THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(BATCH_ITEM_CONTEXT_HANDLE));

    TARRAY_TYPE_DECLARE(BATCH_ITEM_CONTEXT_HANDLE);

#ifdef __cplusplus
}
#endif

#endif /* BATCH_QUEUE_TARRAY_TYPES_H */
