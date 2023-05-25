// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"

#include "c_util/rc_ptr.h"

THANDLE_TYPE_DEFINE(RC_PTR);


THANDLE(RC_PTR) rc_ptr_create_with_move_pointer(void* ptr, RC_PTR_FREE_FUNC free_func)
{
    (void)ptr;
    (void)free_func;
    return NULL;
}
