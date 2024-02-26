// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TWO_D_ARRAY_H
#define TWO_D_ARRAY_H

#include "c_pal/thandle_ll.h"
#include "c_util/two_d_array_ll.h"

#include "umock_c/umock_c_prod.h"

/*TWO_D_ARRAY is-a THANDLE.*/
/*given a type "T" TWO_D_ARRAY(T) expands to the name of the type. */
#define TWO_D_ARRAY(T) TWO_D_ARRAY_LL(T)

#define TWO_D_ARRAY_CREATE_DECLARE(T) TWO_D_ARRAY_LL_CREATE_DECLARE(T, T)
#define TWO_D_ARRAY_CREATE_DEFINE(T) TWO_D_ARRAY_LL_CREATE_DEFINE(T, T)

#define TWO_D_ARRAY_FREE_ROW_DECLARE(T) TWO_D_ARRAY_LL_FREE_ROW_DECLARE(T, T)
#define TWO_D_ARRAY_FREE_ROW_DEFINE(T) TWO_D_ARRAY_LL_FREE_ROW_DEFINE(T, T)

#define TWO_D_ARRAY_ALLOCATE_NEW_ROW_DECLARE(T) TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_DECLARE(T, T)
#define TWO_D_ARRAY_ALLOCATE_NEW_ROW_DEFINE(T) TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW_DEFINE(T, T)

#define TWO_D_ARRAY_GET_ROW_DECLARE(T) TWO_D_ARRAY_LL_GET_ROW_DECLARE(T, T)
#define TWO_D_ARRAY_GET_ROW_DEFINE(T) TWO_D_ARRAY_LL_GET_ROW_DEFINE(T, T)

#define TWO_D_ARRAY_FREE_DEFINE(T) TWO_D_ARRAY_LL_FREE_DEFINE(T, T)

#define TWO_D_ARRAY_CREATE(C) TWO_D_ARRAY_LL_CREATE(C)
#define TWO_D_ARRAY_FREE_ROW(C) TWO_D_ARRAY_LL_FREE_ROW(C)
#define TWO_D_ARRAY_ALLOCATE_NEW_ROW(C) TWO_D_ARRAY_LL_ALLOCATE_NEW_ROW(C)
#define TWO_D_ARRAY_GET_ROW(C) TWO_D_ARRAY_LL_GET_ROW(C)

#define TWO_D_ARRAY_INITIALIZE(T) TWO_D_ARRAY_LL_INITIALIZE(T)
#define TWO_D_ARRAY_ASSIGN(T) TWO_D_ARRAY_LL_ASSIGN(T)
#define TWO_D_ARRAY_MOVE(T) TWO_D_ARRAY_LL_MOVE(T)
#define TWO_D_ARRAY_INITIALIZE_MOVE(T) TWO_D_ARRAY_LL_INITIALIZE_MOVE(T)


/*macro to be used in headers*/                                                                                       \
#define TWO_D_ARRAY_TYPE_DECLARE(T)                                                                                   \
    /*hint: have TWO_D_ARRAY_TYPE_DEFINE(T) before TWO_D_ARRAY_TYPE_DECLARE                                        */ \
    /*hint: have THANDLE_TYPE_DECLARE(TWO_D_ARRAY_TYPEDEF_NAME(T)) before TWO_D_ARRAY_TYPE_DECLARE                 */ \
    TWO_D_ARRAY_CREATE_DECLARE(T)                                                                                     \
    TWO_D_ARRAY_FREE_ROW_DECLARE(T)                                                                                   \
    TWO_D_ARRAY_ALLOCATE_NEW_ROW_DECLARE(T)                                                                           \
    TWO_D_ARRAY_GET_ROW_DECLARE(T)                                                                                    \

#define TWO_D_ARRAY_TYPE_DEFINE(T)                                                                                    \
    /*hint: have THANDLE_TYPE_DEFINE(TWO_D_ARRAY_TYPEDEF_NAME(T)) before TWO_D_ARRAY_TYPE_DEFINE                   */ \
    TWO_D_ARRAY_FREE_DEFINE(T)                                                                                        \
    TWO_D_ARRAY_CREATE_DEFINE(T)                                                                                      \
    TWO_D_ARRAY_FREE_ROW_DEFINE(T)                                                                                    \
    TWO_D_ARRAY_ALLOCATE_NEW_ROW_DEFINE(T)                                                                            \
    TWO_D_ARRAY_GET_ROW_DEFINE(T)                                                                                     \

#endif /*TWO_D_ARRAY_H*/
