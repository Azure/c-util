// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_USER_H
#define THANDLE_USER_H

#include "azure_c_util/thandle.h"

#ifdef __cplusplus
extern "C" {
#endif

THANDLE_TYPE_DECLARE(LL) /*LL is a struct in thandle_user.c that has an "int" in it and a char*. This macro brings a dec_ref, inc_ref, assign, and initialize capabilities*/

THANDLE(LL) ll_create(int a, const char* b);

#ifdef __cplusplus
}
#endif

#endif /*THANDLE_USER_H*/
