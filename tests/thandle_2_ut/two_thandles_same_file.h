// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TWO_THANDLES_SAME_FILE_H
#define TWO_THANDLES_SAME_FILE_H

/*
this file exists to prove the 2 THANDLEs can be compiled in the same .h/.c file
*/


#include <stdint.h>


#include "c_util/thandle.h"

#include "umock_c/umock_c_prod.h"



    typedef struct TYPE_1_TAG
    {
        int x;      /*some pretend coordinate*/
    }TYPE_1;

    typedef struct TYPE_2_TAG
    {
        int y;      /*some other pretend coordinate*/
    }TYPE_2;

    THANDLE_TYPE_DECLARE(TYPE_1);
    THANDLE_TYPE_DECLARE(TYPE_2);

    MOCKABLE_FUNCTION(, void, donothing);




#endif /*two_thandles_same_file.h*/
