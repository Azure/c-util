// Copyright (C) Microsoft Corporation. All rights reserved.

#include "c_util/constbuffer_array.h"
#include "c_util/async_type_helper_ref_counted_handler.h"
#include "c_util/constbuffer_array_sync_wrapper.h"

DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(CONSTBUFFER_ARRAY_HANDLE, constbuffer_array_inc_ref, constbuffer_array_dec_ref);
