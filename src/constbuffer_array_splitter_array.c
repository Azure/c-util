// Copyright (c) Microsoft. All rights reserved.

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/tarray.h"
#include "c_pal/thandle.h"

#include "c_util/constbuffer_array_splitter_array.h"

THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(CONSTBUFFER_ARRAY_HANDLE));

TARRAY_TYPE_DEFINE(CONSTBUFFER_ARRAY_HANDLE);
