// Copyright (c) Microsoft. All rights reserved.

#ifndef TEST_THANDLE_H
#define TEST_THANDLE_H

#include "c_pal/refcount.h"
#include "c_pal/thandle.h"
#include "umock_c/umock_c.h"



    typedef struct TEST_THANDLE_TAG
    {
        unsigned char dummy;
    } TEST_THANDLE;

    THANDLE_TYPE_DECLARE(TEST_THANDLE);



#endif // TEST_THANDLE_H
