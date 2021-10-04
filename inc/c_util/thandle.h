// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_H
#define THANDLE_H

#include <stdlib.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "c_logging/xlogging.h"

#include "c_pal/interlocked.h"

#include "c_util/containing_record.h"
#include "c_util/thandle_ll.h"


#ifdef THANDLE_MALLOC_FUNCTION
    #ifndef THANDLE_FREE_FUNCTION
        #error THANDLE_MALLOC_FUNCTION and THANDLE_FREE_FUNCTION must be both defined or both not defined
    #else
        /*do nothing, the macros here will call whatever THANDLE_MALLOC_FUNCTION/THANDLE_FREE_FUNCTION expands to*/
    #endif
#else
    #ifdef THANDLE_FREE_FUNCTION
        #error THANDLE_MALLOC_FUNCTION and THANDLE_FREE_FUNCTION must be both defined or both not defined
    #else
        /*then use whatever malloc and free expand to*/
    #endif
#endif

/*given a previous type T, this introduces a wrapper type that contains T (and other fields) and defines the functions of that type T*/
#define THANDLE_TYPE_DEFINE(T) \
    MU_DEFINE_STRUCT(THANDLE_WRAPPER_TYPE_NAME(T), THANDLE_EXTRA_FIELDS(T), T, data);                                                                               \
    THANDLE_LL_MALLOC_MACRO(T, T)                                                                                                                                         \
    THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO(T, T)                                                                                                                         \
    THANDLE_LL_CREATE_FROM_CONTENT_FLEX_MACRO(T, T)                                                                                                                       \
    THANDLE_LL_CREATE_FROM_CONTENT_MACRO(T, T)                                                                                                                            \
    THANDLE_LL_FREE_MACRO(T, T)                                                                                                                                           \
    THANDLE_LL_DEC_REF_MACRO(T, T)                                                                                                                                        \
    THANDLE_LL_INC_REF_MACRO(T, T)                                                                                                                                        \
    THANDLE_LL_ASSIGN_MACRO(T, T)                                                                                                                                         \
    THANDLE_LL_INITIALIZE_MACRO(T, T)                                                                                                                                     \
    THANDLE_LL_GET_T_MACRO(T, T)                                                                                                                                          \
    THANDLE_LL_INSPECT_MACRO(T, T)                                                                                                                                        \
    THANDLE_LL_MOVE_MACRO(T, T)                                                                                                                                           \
    THANDLE_LL_INITIALIZE_MOVE_MACRO(T, T)                                                                                                                                \

/*macro to be used in headers*/                                                                                       \
/*introduces an incomplete type based on a MU_DEFINE_STRUCT(T...) previously defined;*/                               \
#define THANDLE_TYPE_DECLARE(T)                                                                                       \
    THANDLE_MACRO(T);                                                                                                 \
    MOCKABLE_FUNCTION(, void, THANDLE_DEC_REF(T), THANDLE(T), t);                                                     \
    MOCKABLE_FUNCTION(, void, THANDLE_INC_REF(T), THANDLE(T), t);                                                     \
    MOCKABLE_FUNCTION(, void, THANDLE_ASSIGN(T), THANDLE(T) *, t1, THANDLE(T), t2 );                                  \
    MOCKABLE_FUNCTION(, void, THANDLE_INITIALIZE(T), THANDLE(T) *, t1, THANDLE(T), t2 );                              \
    MOCKABLE_FUNCTION(, void, THANDLE_MOVE(T), THANDLE(T) *, t1, THANDLE(T)*, t2 );                                   \
    MOCKABLE_FUNCTION(, void, THANDLE_INITIALIZE_MOVE(T), THANDLE(T) *, t1, THANDLE(T)*, t2 );                        \

#endif /*THANDLE_H*/

