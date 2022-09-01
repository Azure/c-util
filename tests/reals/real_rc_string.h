// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_RC_STRING_H
#define REAL_RC_STRING_H

#include "macro_utils/macro_utils.h"

#include "c_util/rc_string.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        rc_string_create, \
        rc_string_create_with_move_memory, \
        rc_string_create_with_custom_free \
    ) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_MOVE(RC_STRING), THANDLE_MOVE(real_RC_STRING)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE(RC_STRING), THANDLE_INITIALIZE(real_RC_STRING)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_INITIALIZE_MOVE(RC_STRING), THANDLE_INITIALIZE_MOVE(real_RC_STRING)) \
    REGISTER_GLOBAL_MOCK_HOOK(THANDLE_ASSIGN(RC_STRING), THANDLE_ASSIGN(real_RC_STRING)) \


#include <stdint.h>


    typedef struct RC_STRING_TAG real_RC_STRING;
    THANDLE_TYPE_DECLARE(real_RC_STRING);

    THANDLE(RC_STRING) real_rc_string_create(const char* string);
    THANDLE(RC_STRING) real_rc_string_create_with_format(const char* format, ...);
    THANDLE(RC_STRING) real_rc_string_create_with_move_memory(const char* string);
    THANDLE(RC_STRING) real_rc_string_create_with_custom_free(const char* string, RC_STRING_FREE_FUNC free_func, void* free_func_context);
    THANDLE(RC_STRING) real_rc_string_recreate(THANDLE(RC_STRING) source);



#endif //REAL_RC_STRING_H
