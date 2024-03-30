// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef CONSTBUFFER_ARRAY_ARRAY_H
#define CONSTBUFFER_ARRAY_ARRAY_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "c_util/constbuffer_array.h"
#include "c_util/tarray.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    TARRAY_DEFINE_STRUCT_TYPE(CONSTBUFFER_ARRAY_HANDLE);
    THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(CONSTBUFFER_ARRAY_HANDLE));
    TARRAY_TYPE_DECLARE(CONSTBUFFER_ARRAY_HANDLE);

#ifdef __cplusplus
}
#endif

#endif // CONSTBUFFER_ARRAY_ARRAY_H
