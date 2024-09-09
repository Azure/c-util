// Copyright (c) Microsoft. All rights reserved.

#ifndef ASYNC_TYPE_HELPER_THANDLE_HANDLER_H
#define ASYNC_TYPE_HELPER_THANDLE_HANDLER_H

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"
#include "c_pal/thandle.h"
#include "async_type_helper.h"

#define DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(THANDLE_TYPE) \
    /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_001: [ DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall declare the copy handler by expanding to: ]*/ \
    MOCKABLE_FUNCTION(, int, ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(THANDLE_TYPE)), THANDLE(THANDLE_TYPE)*, dst, THANDLE(THANDLE_TYPE), src); \
    /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_002: [ DECLARE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall declare the free handler by expanding to: ]*/ \
    MOCKABLE_FUNCTION(, void, ASYNC_TYPE_HELPER_FREE_HANDLER(THANDLE(THANDLE_TYPE)), THANDLE(THANDLE_TYPE), arg); \

#define DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER(THANDLE_TYPE) \
    /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_003: [ DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall implement the copy handler by expanding to: ]*/ \
    int ASYNC_TYPE_HELPER_COPY_HANDLER(THANDLE(THANDLE_TYPE))(THANDLE(THANDLE_TYPE)* dst, THANDLE(THANDLE_TYPE) src) \
    { \
        int result; \
        if (dst == NULL) \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_004: [ If dst is NULL, the copy handler shall fail and return a non-zero value. ]*/ \
            LogError("Invalid arguments: dst = %p, src = %p", dst, src); \
            result = MU_FAILURE; \
        } \
        else \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_006: [ Otherwise the copy handler shall assign src to dst by calling THANDLE_INITIALIZE. ]*/ \
            /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_007: [ The copy handler shall succeed and return 0. ]*/ \
            THANDLE_INITIALIZE(THANDLE_TYPE)(dst, src); \
            result = 0; \
        } \
        return result; \
    } \
    /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_008: [ DEFINE_ASYNC_TYPE_HELPER_THANDLE_HANDLER shall implement the free handler by expanding to: ]*/ \
    void ASYNC_TYPE_HELPER_FREE_HANDLER(THANDLE(THANDLE_TYPE))(THANDLE(THANDLE_TYPE) arg) \
    { \
        if (arg == NULL) \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_009: [ If arg is NULL, the free handler shall return. ]*/ \
            LogError("NULL arg in free handler"); \
        } \
        else \
        { \
            /* Codes_SRS_ASYNC_TYPE_HELPER_THANDLE_HANDLER_01_010: [ Otherwise the free handler shall release arg by assigning NULL to it. ]*/ \
            THANDLE_ASSIGN(THANDLE_TYPE)(&arg, NULL); \
        } \
    }

#endif // ASYNC_TYPE_HELPER_THANDLE_HANDLER_H
