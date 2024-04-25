// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "real_gballoc_hl.h"
#include "real_gballoc_hl_renames.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "real_constbuffer_array_tarray_renames.h"

#include "c_pal/thandle_ll.h"
#include "c_util/tarray_ll.h"

#include "real_constbuffer_array_tarray.h"

THANDLE_LL_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(real_CONSTBUFFER_ARRAY_HANDLE), TARRAY_TYPEDEF_NAME(CONSTBUFFER_ARRAY_HANDLE));
THANDLE_LL_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(real_CONSTBUFFER_ARRAY_HANDLE), TARRAY_TYPEDEF_NAME(CONSTBUFFER_ARRAY_HANDLE));

TARRAY_LL_TYPE_DEFINE(real_CONSTBUFFER_ARRAY_HANDLE, CONSTBUFFER_ARRAY_HANDLE);
