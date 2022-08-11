// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef COLLECTION_TWO_H
#define COLLECTION_TWO_H

#include "macro_utils/macro_utils.h"

#include "c_util/flags_to_string.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C"
{
#endif

    /*file only wants to see that two flag stringifications can exist in the same file. If it compiles - it is good.*/

    FLAGS_TO_STRING_DECLARE_FUNCTION(COLLECTION_ALFA);
    FLAGS_TO_STRING_DECLARE_FUNCTION(COLLECTION_BETA);

#ifdef __cplusplus
}
#endif

#endif /*COLLECTION_TWO_H*/
