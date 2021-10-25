// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef MALLOC_MOCKS_H
#define MALLOC_MOCKS_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    MOCKABLE_FUNCTION(, void*, global_malloc, size_t, size);                                    /*a function used globally when nothing else is specified*/
    MOCKABLE_FUNCTION(, void*, global_malloc_flex, size_t, base, size_t, nmemb, size_t, size);  /*a function used globally when nothing else is specified*/
    MOCKABLE_FUNCTION(, void, global_free, void*, ptr);                                         /*a function used globally when nothing else is specified*/

    MOCKABLE_FUNCTION(, void*, type_malloc, size_t, size);                                      /*a function used when THANDLE_TYPE is defined*/
    MOCKABLE_FUNCTION(, void*, type_malloc_flex, size_t, base, size_t, nmemb, size_t, size);    /*a function used when THANDLE_TYPE is defined*/
    MOCKABLE_FUNCTION(, void, type_free, void*, ptr);                                           /*a function used when THANDLE_TYPE is defined*/

    MOCKABLE_FUNCTION(, void*, var_malloc, size_t, size);                                       /*a function used when an instance of a THANDLE_TYPE is created*/
    MOCKABLE_FUNCTION(, void*, var_malloc_flex, size_t, base, size_t, nmemb, size_t, size);     /*a function used when an instance of a THANDLE_TYPE is created*/
    MOCKABLE_FUNCTION(, void, var_free, void*, ptr);                                            /*a function used when an instance of a THANDLE_TYPE is created*/
#ifdef __cplusplus
}
#endif

#endif /*MALLOC_MOCKS_H*/
