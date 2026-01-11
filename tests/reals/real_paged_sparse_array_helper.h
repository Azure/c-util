// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_PAGED_SPARSE_ARRAY_HELPER_H
#define REAL_PAGED_SPARSE_ARRAY_HELPER_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle_ll.h"
#include "c_util/paged_sparse_array_ll.h"

#include "c_pal/gballoc_hl.h"
#include "real_gballoc_hl.h"

/*
 * Creates a PAGED_SPARSE_ARRAY alias of element type T called REAL_PAGED_SPARSE_ARRAY_T
 * Must have ENABLE_MOCKS undefined
 * Recommend including real_interlocked_renames.h before this call and real_interlocked_undo_renames.h after this call
 *
 * The pattern creates:
 * - A real type alias (REAL_T) for the element type
 * - A real PAGED_SPARSE_ARRAY struct definition for the real element type
 * - THANDLE support for the real PAGED_SPARSE_ARRAY
 * - Real implementations of CREATE, ALLOCATE, RELEASE, ALLOCATE_OR_GET, GET
 */

/* Declare a real PAGED_SPARSE_ARRAY for element type T */
#define REAL_PAGED_SPARSE_ARRAY_DECLARE(T)                                                                             \
    typedef T MU_C2(REAL_, T);                                                                                         \
    PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(MU_C2(REAL_, T))                                                             \
    THANDLE_LL_TYPE_DECLARE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(MU_C2(REAL_, T)), PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T));     \
    PAGED_SPARSE_ARRAY_LL_CREATE_DECLARE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                             \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_DECLARE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                           \
    PAGED_SPARSE_ARRAY_LL_RELEASE_DECLARE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                            \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_DECLARE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                    \
    PAGED_SPARSE_ARRAY_LL_GET_DECLARE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                                \

/* Define a real PAGED_SPARSE_ARRAY for element type T using real_gballoc_hl functions */
#define REAL_PAGED_SPARSE_ARRAY_DEFINE(T)                                                                              \
    THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(                                                                      \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(MU_C2(REAL_, T)),                                                              \
        PAGED_SPARSE_ARRAY_TYPEDEF_NAME(T),                                                                            \
        real_gballoc_hl_malloc,                                                                                        \
        real_gballoc_hl_malloc_flex,                                                                                   \
        real_gballoc_hl_free);                                                                                         \
    PAGED_SPARSE_ARRAY_LL_FREE_DEFINE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                                \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_PAGE_INTERNAL_DEFINE(MU_C2(REAL_, T), MU_C2(REAL_, T))                              \
    PAGED_SPARSE_ARRAY_LL_CREATE_DEFINE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                              \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_DEFINE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                            \
    PAGED_SPARSE_ARRAY_LL_RELEASE_DEFINE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                             \
    PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET_DEFINE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                     \
    PAGED_SPARSE_ARRAY_LL_GET_DEFINE(MU_C2(REAL_, T), MU_C2(REAL_, T))                                                 \

/* Hook the PAGED_SPARSE_ARRAY(TYPE) calls to the real implementations */
#define REGISTER_REAL_PAGED_SPARSE_ARRAY_MOCK_HOOK(T)                                                                  \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_CREATE(T), PAGED_SPARSE_ARRAY_LL_CREATE(MU_C2(REAL_, T)));            \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_ALLOCATE(T), PAGED_SPARSE_ARRAY_LL_ALLOCATE(MU_C2(REAL_, T)));        \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_RELEASE(T), PAGED_SPARSE_ARRAY_LL_RELEASE(MU_C2(REAL_, T)));          \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T), PAGED_SPARSE_ARRAY_LL_ALLOCATE_OR_GET(MU_C2(REAL_, T))); \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_GET(T), PAGED_SPARSE_ARRAY_LL_GET(MU_C2(REAL_, T)));                  \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_ASSIGN(T), THANDLE_ASSIGN(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(MU_C2(REAL_, T)))); \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_INITIALIZE(T), THANDLE_INITIALIZE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(MU_C2(REAL_, T)))); \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T), THANDLE_INITIALIZE_MOVE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(MU_C2(REAL_, T)))); \
    REGISTER_GLOBAL_MOCK_HOOK(PAGED_SPARSE_ARRAY_MOVE(T), THANDLE_MOVE(PAGED_SPARSE_ARRAY_TYPEDEF_NAME(MU_C2(REAL_, T))));

#endif // REAL_PAGED_SPARSE_ARRAY_HELPER_H
