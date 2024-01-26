// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TWO_D_ARRAY_LL_H
#define TWO_D_ARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#else // __cplusplus
#include <stdbool.h>
#include <inttypes.h>
#endif // __cplusplus

#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*TQUEUE is backed by a THANDLE build on the structure below*/
#define TWO_D_ARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TWO_D_ARRAY_TYPEDEF_NAME(T), _TAG)

#define TWO_D_ARRAY_TYPEDEF_NAME(T) MU_C2(TWO_D_ARRAY_STRUCT_, T)


#endif  /*TWO_D_ARRAY_LL_H*/
