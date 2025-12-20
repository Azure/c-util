// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef LEAK_LOG_FOLDER_H
#define LEAK_LOG_FOLDER_H

#ifndef __cplusplus
#include <stddef.h>
#endif

#include "macro_utils/macro_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

wchar_t* leak_log_folder_create(const char* leak_log_root_folder_name);
void leak_log_folder_destroy(wchar_t* leak_log_folder);

#ifdef __cplusplus
}
#endif

#endif /* LEAK_LOG_FOLDER_H */
