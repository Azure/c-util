// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef CONSTBUFFER_ARRAY_SYNC_WRAPPER_H
#define CONSTBUFFER_ARRAY_SYNC_WRAPPER_H

#include "c_util/async_type_helper_ref_counted_handler.h"
#include "c_util/constbuffer_array.h"

#ifdef __cplusplus
extern "C" {
#endif

    DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(CONSTBUFFER_ARRAY_HANDLE);

#ifdef __cplusplus
}
#endif

#endif // CONSTBUFFER_ARRAY_SYNC_WRAPPER_H
