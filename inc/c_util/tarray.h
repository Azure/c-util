// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TARRAY_H
#define TARRAY_H

#ifdef __cplusplus
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "c_util/thandle_ll.h"
#include "c_util/tarray_ll.h"

#include "umock_c/umock_c_prod.h"

/*TARRAY is-a THANDLE.*/
/*given a type "T" TARRAY(T) expands to the name of the type. */ /*tarray_int folder in c-util\tests contains a template for usage in tests (with reals)*/
#define TARRAY(T) TARRAY_LL(T)

#define TARRAY_CREATE_DECLARE(T) TARRAY_LL_CREATE_DECLARE(T, T)
#define TARRAY_CREATE_DEFINE(T) TARRAY_LL_CREATE_DEFINE(T, T)

#define TARRAY_ENSURE_CAPACITY_DECLARE(T) TARRAY_LL_ENSURE_CAPACITY_DECLARE(T, T)
#define TARRAY_ENSURE_CAPACITY_DEFINE(T) TARRAY_LL_ENSURE_CAPACITY_DEFINE(T, T)

#define TARRAY_FREE_DEFINE(T) TARRAY_LL_FREE_DEFINE(T, T)

#define TARRAY_CREATE(C) TARRAY_LL_CREATE(C)
#define TARRAY_ENSURE_CAPACITY(C) TARRAY_LL_ENSURE_CAPACITY(C)

#define TARRAY_INITIALIZE(T) TARRAY_LL_INITIALIZE(T)
#define TARRAY_ASSIGN(T) TARRAY_LL_ASSIGN(T)
#define TARRAY_DEC_REF(T) TARRAY_LL_DEC_REF(T)
#define TARRAY_INC_REF(T) TARRAY_LL_INC_REF(T)
#define TARRAY_MOVE(T) TARRAY_LL_MOVE(T)
#define TARRAY_INITIALIZE_MOVE(T) TARRAY_LL_INITIALIZE_MOVE(T)

/*macro to be used in headers*/                                                                                     \
#define TARRAY_TYPE_DECLARE(T)                                                                                      \
    /*hint: have TARRAY_DEFINE_STRUCT_TYPE(T) before TARRAY_TYPE_DECLARE                                         */ \
    /*hint: have THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(T)) before TARRAY_TYPE_DECLARE                         */ \
    TARRAY_CREATE_DECLARE(T)                                                                                        \
    TARRAY_ENSURE_CAPACITY_DECLARE(T)                                                                               \

#define TARRAY_TYPE_DEFINE(T)                                                                                       \
    /*hint: have THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(T)) before  TARRAY_TYPE_DEFINE                          */ \
    TARRAY_FREE_DEFINE(T)                                                                                           \
    TARRAY_CREATE_DEFINE(T)                                                                                         \
    TARRAY_ENSURE_CAPACITY_DEFINE(T)                                                                                \

#endif  /*TARRAY_H*/
