// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_RC_STRING_ARRAY_H
#define REAL_RC_STRING_ARRAY_H


#include <stdint.h>
#include <stddef.h>


#include "c_util/rc_string_array.h"

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_RC_STRING_ARRAY_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        rc_string_array_create, \
        rc_string_array_destroy \
    )



RC_STRING_ARRAY* real_rc_string_array_create(uint32_t count);
void real_rc_string_array_destroy(RC_STRING_ARRAY* rc_string_array);




#endif //REAL_RC_STRING_ARRAY_H
