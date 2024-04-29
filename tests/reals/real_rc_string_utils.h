// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_RC_STRING_UTILS_H
#define REAL_RC_STRING_UTILS_H

#include <stdint.h>
#include <stddef.h>

#include "c_util/rc_string_array.h"

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_RC_STRING_UTILS_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        rc_string_utils_split_by_char \
    )

RC_STRING_ARRAY* real_rc_string_utils_split_by_char(THANDLE(RC_STRING) str, char delimiter);

#endif //REAL_RC_STRING_UTILS_H
