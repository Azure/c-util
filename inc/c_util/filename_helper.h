// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef FILENAME_HELPER_H
#define FILENAME_HELPER_H

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, char*, filename_append_suffix, const char*, filename, const char*, suffix);

#ifdef __cplusplus
}
#endif

#endif // FILENAME_HELPER_H
