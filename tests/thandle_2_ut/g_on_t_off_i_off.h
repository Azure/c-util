// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef G_ON_T_OFF_I_OFF_H
#define G_ON_T_OFF_I_OFF_H

/*
G_ON - will use the global THANDLE_MALLOC_FUNCTION macro
T_OFF will use THANDLE_LL_TYPE_DEFINE (will not use THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS)
I_OFF - instance of the type will be created with THANDLE_MALLOC (will not use THANDLE_MALLOC_FUNCTION_WITH_MALLOC_FUNCTIONS_NAME)
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

    typedef struct G_ON_T_OFF_I_OFF_TAG
    {
        int x;
    }G_ON_T_OFF_I_OFF_DUMMY;

    THANDLE_TYPE_DECLARE(G_ON_T_OFF_I_OFF_DUMMY);

MOCKABLE_FUNCTION(, THANDLE(G_ON_T_OFF_I_OFF_DUMMY), G_ON_T_OFF_I_OFF_create, int, x);

#ifdef __cplusplus
}
#endif

#endif /*G_ON_T_OFF_I_OFF_H*/
