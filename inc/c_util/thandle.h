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
    THANDLE_LL_TYPE_DEFINE(T, T)

/*given a previous type T, this introduces a wrapper type that contains T (and other fields) and defines the functions of that type T with special memory allocators*/
#define THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, malloc_function, malloc_flex_function, free_function) \
    THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(T, T, malloc_function, malloc_flex_function, free_function)


/*macro to be used in headers*/                                                                                       \
/*introduces an incomplete type based on a MU_DEFINE_STRUCT(T...) previously defined;*/                               \
#define THANDLE_TYPE_DECLARE(T)                                                                                       \
    THANDLE_LL_TYPE_DECLARE(T, T)

#endif /*THANDLE_H*/

