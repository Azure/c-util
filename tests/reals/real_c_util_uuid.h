// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_C_UTIL_UUID_H
#define REAL_C_UTIL_UUID_H

#include "macro_utils/macro_utils.h"
#include "c_pal/uuid.h"
#include "c_util/uuid.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_C_UTIL_UUID_GLOBAL_MOCK_HOOK() /*vld.h trebuie schimbar numele ca e acelasi cu ala din pal*/\
    MU_FOR_EACH_1(R2, \
        uuid_from_string, \
        uuid_to_string \
    )



    UUID_FROM_STRING_RESULT real_uuid_from_string(const char* uuid_string, UUID_T uuid);
    char* real_uuid_to_string(const UUID_T uuid);




#endif // REAL_C_UTIL_UUID_H
