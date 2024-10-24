// Copyright (c) Microsoft. All rights reserved.

#ifndef ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_H
#define ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_H

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_util/async_type_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(COPY_VALUE_TYPE) \
    /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_001: [ DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall declare the copy handler by expanding to: ]*/ \
    DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(COPY_VALUE_TYPE, dst, src); \
    /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_002: [ DECLARE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall declare the free handler by expanding to: ]*/ \
    DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(COPY_VALUE_TYPE, arg);

#define DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER(COPY_VALUE_TYPE) \
    /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_003: [ DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall implement the copy handler by expanding to: ]*/ \
    DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(COPY_VALUE_TYPE, dst, src) \
    { \
        int result; \
        if (dst == NULL) \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_004: [ If dst is NULL, the copy handler shall fail and return a non-zero value. ]*/ \
            LogError("Invalid arguments: dst=%p", dst); \
            result = MU_FAILURE; \
        } \
        else \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_005: [ Otherwise the copy handler shall copy all the contents of src to dst. ]*/ \
            *dst = src; \
            /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_006: [ The copy handler shall succeed and return 0. ]*/ \
            result = 0; \
        } \
        return result; \
    } \
    /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_007: [ DEFINE_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER shall implement the free handler by expanding to: ]*/ \
    DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(COPY_VALUE_TYPE, arg) \
    { \
        /* Codes_SRS_ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_01_009: [ The free handler shall return. ]*/ \
        (void)arg; \
    }

#ifdef __cplusplus
}
#endif

#endif // ASYNC_TYPE_HELPER_COPY_VALUE_HANDLER_H
