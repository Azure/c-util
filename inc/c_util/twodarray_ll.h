// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TWODARRAY_LL_H
#define TWODARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#else // __cplusplus
#include <stdbool.h>
#include <inttypes.h>
#endif // __cplusplus

#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*TQUEUE is backed by a THANDLE build on the structure below*/
#define TWODARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TWODARRAY_TYPEDEF_NAME(T), _TAG)

#define TWODARRAY_TYPEDEF_NAME(T) MU_C2(TWODARRAY_STRUCT_, T)


#endif  /*TWODARRAY_LL_H*/
