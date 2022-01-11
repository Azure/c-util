// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_USER_33_CHARACTERS_H
#define THANDLE_USER_33_CHARACTERS_H

#include "c_util/thandle.h"



    typedef struct LL3456789012345678901234567890123_TAG LL3456789012345678901234567890123;

THANDLE_TYPE_DECLARE(LL3456789012345678901234567890123) /*LL is a struct in thandle_user.c that has an "int" in it*/

THANDLE(LL3456789012345678901234567890123) ll33_create(int a);

/*function that retrieves the name only exists on debug*/
#if defined(DEBUG) || defined(_DEBUG)
const char* ll33_get_name(THANDLE(LL3456789012345678901234567890123) ll);
#endif




#endif /*THANDLE_USER_33_CHARACTERS_H*/
