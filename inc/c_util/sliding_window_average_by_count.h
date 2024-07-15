// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SLIDING_WINDOW_AVERAGE_BY_COUNT_H
#define SLIDING_WINDOW_AVERAGE_BY_COUNT_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#else
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SLIDING_WINDOW_AVERAGE_TAG SLIDING_WINDOW_AVERAGE;

THANDLE_TYPE_DECLARE(SLIDING_WINDOW_AVERAGE);

MOCKABLE_FUNCTION(, THANDLE(SLIDING_WINDOW_AVERAGE), sliding_window_average_by_count_create, int32_t, window_count);
MOCKABLE_FUNCTION(, int, sliding_window_average_by_count_add, THANDLE(SLIDING_WINDOW_AVERAGE), handle, int64_t, next_count);
MOCKABLE_FUNCTION(, int, sliding_window_average_by_count_get, THANDLE(SLIDING_WINDOW_AVERAGE), handle, double*, average);

#ifdef __cplusplus
}
#endif

#endif /* SLIDING_WINDOW_AVERAGE_BY_COUNT_H */
