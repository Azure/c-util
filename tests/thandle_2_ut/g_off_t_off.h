// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef G_OFF_T_OFF_H
#define G_OFF_T_OFF_H

/*
G_OFF- will have as defines for the global functions NULL"
T_OFF will use THANDLE_LL_TYPE_DEFINE (will not use THANDLE_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS) - so "type off"
*/

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "c_util/thandle.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct G_OFF_T_OFF_TAG
    {
        int x;      /*some pretend coordinate*/
        uint32_t n; /*the number of characters in s[]. Might be 0, in which case s should not be accessed*/
        char s[]    /*has n characters*/;
    }G_OFF_T_OFF_DUMMY;

    THANDLE_TYPE_DECLARE(G_OFF_T_OFF_DUMMY);

    MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_DUMMY), G_OFF_T_OFF_create, int, x);
    MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_DUMMY), G_OFF_T_OFF_create_with_malloc_functions, int, x);
    MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_DUMMY), G_OFF_T_OFF_create_with_extra_size, int, x, const char*, s);
    MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_DUMMY), G_OFF_T_OFF_create_with_extra_size_with_malloc_functions, int, x, const char*, s);
    MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_DUMMY), G_OFF_T_OFF_create_from_content_flex, const G_OFF_T_OFF_DUMMY*, origin);
    MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_DUMMY), G_OFF_T_OFF_create_from_content_flex_with_malloc_functions, const G_OFF_T_OFF_DUMMY*, origin);

    MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_DUMMY), G_OFF_T_OFF_create_from_content_flex_with_getsizeof_NULL, const G_OFF_T_OFF_DUMMY*, origin);

#ifdef __cplusplus
}
#endif

#endif /*G_OFF_T_OFF_H*/