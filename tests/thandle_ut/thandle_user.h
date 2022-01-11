// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_USER_H
#define THANDLE_USER_H

#include "c_util/thandle.h"



    typedef struct LL_TAG LL;

THANDLE_TYPE_DECLARE(LL) /*LL is a struct in thandle_user.c that has an "int" in it and a char*. This macro brings a dec_ref, inc_ref, assign, and initialize capabilities*/

THANDLE(LL) ll_create(int a, const char* b);

void ll_increment_a(THANDLE(LL) ll, int amount);

int ll_get_a(THANDLE(LL) ll);




#endif /*THANDLE_USER_H*/
