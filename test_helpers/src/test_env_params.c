// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include "c_util_test_helpers/test_env_params.h"

#define DEFAULT_LEAK_LOG_FOLDER_NAME "g$\\blockstorage"

const char* test_env_params_get_leak_log_folder(void)
{
    const char* env_leak_log_folder = getenv("BLOCK_STORAGE_LEAK_LOG_FOLDER");
    return (env_leak_log_folder == NULL) ? DEFAULT_LEAK_LOG_FOLDER_NAME : env_leak_log_folder;
}

