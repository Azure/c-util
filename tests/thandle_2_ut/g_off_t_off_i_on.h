// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef G_OFF_T_OFF_I_ON_H
#define G_OFF_T_OFF_I_ON_H

/*
G_OFF - will not use the global THANDLE_MALLOC_FUNCTION macro
T_OFF will use THANDLE_LL_TYPE_DEFINE (will not use THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS)
I_ON - instance of the type will not be created with THANDLE_MALLOC (will use THANDLE_MALLOC_FUNCTION_WITH_MALLOC_FUNCTIONS_NAME)
*/

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

#include "c_util/thandle.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct G_OFF_T_OFF_I_ON_TAG
    {
        int x;
    }G_OFF_T_OFF_I_ON_DUMMY;

    THANDLE_TYPE_DECLARE(G_OFF_T_OFF_I_ON_DUMMY);

MOCKABLE_FUNCTION(, THANDLE(G_OFF_T_OFF_I_ON_DUMMY), G_OFF_T_OFF_I_ON_create, int, x);

#ifdef __cplusplus
}
#endif

#endif /*G_OFF_T_OFF_I_ON_H*/
