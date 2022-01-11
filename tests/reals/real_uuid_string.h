// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_UUID_STRING_H
#define REAL_UUID_STRING_H

#include "macro_utils/macro_utils.h"
#include "c_pal/uuid.h"
#include "c_util/uuid_string.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_UUID_STRING_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        uuid_from_string, \
        uuid_to_string \
    )

#ifdef __cplusplus
}
#endif

    UUID_FROM_STRING_RESULT real_uuid_from_string(const char* uuid_string, UUID_T uuid);
    char* real_uuid_to_string(const UUID_T uuid);

#ifdef __cplusplus
}
#endif



#endif // REAL_UUID_STRING_H
