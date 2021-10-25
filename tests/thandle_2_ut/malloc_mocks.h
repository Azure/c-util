// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef MALLOC_MOCKS_H
#define MALLOC_MOCKS_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "c_util/constbuffer.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, void*, global_malloc, size_t, size);                                    /*a function used #define THANDLE_MALLOC_FUNCTION*/
    MOCKABLE_FUNCTION(, void*, global_malloc_flex, size_t, base, size_t, nmemb, size_t, size);  /*a function used #define THANDLE_MALLOC_FUNCTION*/
    MOCKABLE_FUNCTION(, void, global_free, void*, ptr);                                         /*a function used #define THANDLE_FREE_FUNCTION*/

    MOCKABLE_FUNCTION(, void*, type_malloc, size_t, size);                                      /*a function used by the macro THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS*/
    MOCKABLE_FUNCTION(, void*, type_malloc_flex, size_t, base, size_t, nmemb, size_t, size);    /*a function used by THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS*/
    MOCKABLE_FUNCTION(, void, type_free, void*, ptr);                                           /*a function used by THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS*/

    MOCKABLE_FUNCTION(, void*, var_malloc, size_t, size);                                       /*a function used by the macro THANDLE_MALLOC_FUNCTION_WITH_MALLOC_FUNCTIONS*/
    MOCKABLE_FUNCTION(, void*, var_malloc_flex, size_t, base, size_t, nmemb, size_t, size);     /*a function used by THANDLE_MALLOC_FUNCTION_WITH_MALLOC_FUNCTIONS*/
    MOCKABLE_FUNCTION(, void, var_free, void*, ptr);                                            /*a function used by THANDLE_MALLOC_FUNCTION_WITH_MALLOC_FUNCTIONS*/
#ifdef __cplusplus
}
#endif

#endif /*MALLOC_MOCKS_H*/
