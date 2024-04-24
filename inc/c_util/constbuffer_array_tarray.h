// Copyright (c) Microsoft. All rights reserved.

#ifndef CONSTBUFFER_ARRAY_TARRAY_H
#define CONSTBUFFER_ARRAY_TARRAY_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "c_util/tarray.h"
#include "c_pal/thandle.h"
#include "c_util/constbuffer_array.h"

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

#endif //CONSTBUFFER_ARRAY_TARRAY_H
