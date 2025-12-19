// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef LEAK_LOG_CHECKER_H
#define LEAK_LOG_CHECKER_H

#ifdef __cplusplus
#include <cwchar>
#else
#include <wchar.h>
#endif

#include "macro_utils/macro_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LEAK_LOG_CHECKER_CHECK_RESULT_VALUES \
    LEAK_LOG_CHECKER_CHECK_OK, \
    LEAK_LOG_CHECKER_CHECK_ERROR, \
    LEAK_LOG_CHECKER_CHECK_LEAKS_FOUND

MU_DEFINE_ENUM(LEAK_LOG_CHECKER_CHECK_RESULT, LEAK_LOG_CHECKER_CHECK_RESULT_VALUES)

LEAK_LOG_CHECKER_CHECK_RESULT leak_log_checker_check_folder(const wchar_t* folder_to_check);

#ifdef __cplusplus
}
#endif

#endif /* LEAK_LOG_CHECKER_H */
