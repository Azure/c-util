// Copyright (c) Microsoft. All rights reserved.

#ifndef ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_H
#define ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_H

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"
#include "async_type_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(REF_COUNTED_HANDLE_TYPE) \
    /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_001: [ DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER shall declare the copy handler by expanding to: ]*/ \
    DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(REF_COUNTED_HANDLE_TYPE, dst, src); \
    /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_002: [ DECLARE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER shall declare the free handler by expanding to: ]*/ \
    DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(REF_COUNTED_HANDLE_TYPE, arg);

#define DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER(REF_COUNTED_HANDLE_TYPE, inc_ref_function, dec_ref_function) \
    /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_003: [ DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER shall implement the copy handler by expanding to: ]*/ \
    DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(REF_COUNTED_HANDLE_TYPE, dst, src) \
    { \
        int result; \
        if (dst == NULL) \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_004: [ If dst is NULL, the copy handler shall fail and return a non-zero value. ]*/ \
            LogError("Invalid arguments: dst = %p, src = %p", dst, src); \
            result = MU_FAILURE; \
        } \
        else \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_011: [ The copy handler shall increment the reference count for src only if src is not NULL. ]*/ \
            if (src != NULL) \
            { \
                /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_006: [ Otherwise the copy handler shall increment the reference count for src by calling inc_ref_function. ]*/ \
                inc_ref_function(src); \
            } \
            /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_007: [ The copy handler shall store src in dst and return 0. ]*/ \
            *dst = src; \
            result = 0; \
        } \
        return result; \
    } \
    /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_008: [ DEFINE_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER shall implement the free handler by expanding to: ]*/ \
    DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(REF_COUNTED_HANDLE_TYPE, arg) \
    { \
        if (arg == NULL) \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_009: [ If arg is NULL, the free handler shall return. ]*/ \
            LogError("NULL arg in free handler"); \
        } \
        else \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_01_010: [ Otherwise the free handler shall decrement the reference count for arg by calling dec_ref_function. ]*/ \
            dec_ref_function(arg); \
        } \
    }

#ifdef __cplusplus
}
#endif

#endif // ASYNC_TYPE_HELPER_REF_COUNTED_HANDLER_H
