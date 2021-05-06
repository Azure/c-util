// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef PS_UTIL_H
#define PS_UTIL_H

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, void, ps_util_terminate_process);
MOCKABLE_FUNCTION(, void, ps_util_exit_process, int, exit_code);

#ifdef __cplusplus
}
#endif

#endif /* PS_UTIL_H */
