// Copyright (c) Microsoft. All rights reserved.

#ifndef TLOG_FOO_H
#define TLOG_FOO_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "c_pal/thandle.h"

#include "c_util/tlog.h"

typedef struct FOO_TAG
{
    int64_t x;
} FOO;

// This is the call dispatcher used for most of the tests
TLOG_DEFINE_STRUCT_TYPE(FOO);
THANDLE_TYPE_DECLARE(TLOG_TYPEDEF_NAME(FOO))
TLOG_TYPE_DECLARE(FOO);

#endif // TLOG_FOO_H
