// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "c_util/flags_to_string.h"

#include "collection_one.h"

#define FLAG_ONE 1
#define FLAG_TWO 2

#define COLLECTION_ONE_FLAGS    \
    FLAG_ONE, "FLAG_ONE" ,      \
    FLAG_TWO, "FLAG_TWO"



FLAGS_TO_STRING_DEFINE_FUNCTION(COLLECTION_ONE, COLLECTION_ONE_FLAGS);
