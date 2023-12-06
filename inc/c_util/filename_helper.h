// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef BS_FILENAME_HELPER_H
#define BS_FILENAME_HELPER_H

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, char*, bs_filename_append_suffix, const char*, filename, const char*, suffix);

#ifdef __cplusplus
}
#endif

#endif // BS_FILENAME_HELPER_H
