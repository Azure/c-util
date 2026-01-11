// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PAGED_SPARSE_ARRAY_H
#define PAGED_SPARSE_ARRAY_H

#include "c_pal/thandle_ll.h"
#include "c_util/paged_sparse_array_ll.h"

#include "umock_c/umock_c_prod.h"

/*PAGED_SPARSE_ARRAY is-a THANDLE.*/
/*given a type "T" PAGED_SPARSE_ARRAY(T) expands to the name of the type. */
#define PAGED_SPARSE_ARRAY(T) PAGED_SPARSE_ARRAY_LL(T)

#define PAGED_SPARSE_ARRAY_CREATE_DECLARE(T) PAGED_SPARSE_ARRAY_LL_CREATE_DECLARE(T, T)
#define PAGED_SPARSE_ARRAY_CREATE_DEFINE(T) PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(T, T)

#define PAGED_SPARSE_ARRAY_FREE_DEFINE(T) PAGED_SPARSE_ARRAY_LL_FREE_DEFINE(T, T)

#define PAGED_SPARSE_ARRAY_ALLOCATE_PAGE_INTERNAL_DEFINE(T) PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_DEFINE(T, T)

#define PAGED_SPARSE_ARRAY_ALLOCATE_DECLARE(T) PAGED_SPARSE_ARRAY_LL_ALLOCATE_DECLARE(T, T)
#define PAGED_SPARSE_ARRAY_ALLOCATE_DEFINE(T) PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(T, T)

#define PAGED_SPARSE_ARRAY_RELEASE_DECLARE(T) PAGED_SPARSE_ARRAY_LL_RELEASE_DECLARE(T, T)
#define PAGED_SPARSE_ARRAY_RELEASE_DEFINE(T) PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(T, T)

#define PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET_DECLARE(T) PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_DECLARE(T, T)
#define PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET_DEFINE(T) PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_DEFINE(T, T)

#define PAGED_SPARSE_ARRAY_GET_DECLARE(T) PAGED_SPARSE_ARRAY_LL_GET_DECLARE(T, T)
#define PAGED_SPARSE_ARRAY_GET_DEFINE(T) PAGED_SPARSE_ARRAY_LL_GET_DEFINE(T, T)

#define PAGED_SPARSE_ARRAY_CREATE(C) PAGED_SPARSE_ARRAY_LL_CREATE(C)
#define PAGED_SPARSE_ARRAY_ALLOCATE(C) PAGED_SPARSE_ARRAY_LL_ALLOCATE(C)
#define PAGED_SPARSE_ARRAY_RELEASE(C) PAGED_SPARSE_ARRAY_LL_RELEASE(C)
#define PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(C) PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET(C)
#define PAGED_SPARSE_ARRAY_GET(C) PAGED_SPARSE_ARRAY_LL_GET(C)

#define PAGED_SPARSE_ARRAY_INITIALIZE(T) PAGED_SPARSE_ARRAY_LL_INITIALIZE(T)
#define PAGED_SPARSE_ARRAY_ASSIGN(T) PAGED_SPARSE_ARRAY_LL_ASSIGN(T)
#define PAGED_SPARSE_ARRAY_MOVE(T) PAGED_SPARSE_ARRAY_LL_MOVE(T)
#define PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T) PAGED_SPARSE_ARRAY_LL_INITIALIZE_MOVE(T)

/*macro to be used in headers*/                                                                                       \
#define PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)                                                                             \
    /*hint: have PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T) before PAGED_SPARSE_ARRAY_TYPE_DECLARE                    */ \
    /*hint: have THANDLE_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)) before PAGED_SPARSE_ARRAY_TYPE_DECLARE    */ \
    PAGED_SPARSE_ARRAY_CREATE_DECLARE(T)                                                                               \
    PAGED_SPARSE_ARRAY_ALLOCATE_DECLARE(T)                                                                             \
    PAGED_SPARSE_ARRAY_RELEASE_DECLARE(T)                                                                              \
    PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET_DECLARE(T)                                                                      \
    PAGED_SPARSE_ARRAY_GET_DECLARE(T)                                                                                  \

#define PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)                                                                              \
    /*hint: have THANDLE_TYPE_DEFINE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T)) before PAGED_SPARSE_ARRAY_TYPE_DEFINE      */ \
    PAGED_SPARSE_ARRAY_FREE_DEFINE(T)                                                                                  \
    PAGED_SPARSE_ARRAY_ALLOCATE_PAGE_INTERNAL_DEFINE(T)                                                                \
    PAGED_SPARSE_ARRAY_CREATE_DEFINE(T)                                                                                \
    PAGED_SPARSE_ARRAY_ALLOCATE_DEFINE(T)                                                                              \
    PAGED_SPARSE_ARRAY_RELEASE_DEFINE(T)                                                                               \
    PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET_DEFINE(T)                                                                       \
    PAGED_SPARSE_ARRAY_GET_DEFINE(T)                                                                                   \

#endif /*PAGED_SPARSE_ARRAY_H*/
