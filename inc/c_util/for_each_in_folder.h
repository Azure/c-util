// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef FOR_EACH_IN_FOLDER_H
#define FOR_EACH_IN_FOLDER_H

#ifdef __cplusplus
#else
#include <stdbool.h>
#endif

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int(*ON_EACH_IN_FOLDER)(const char* folder, const WIN32_FIND_DATAA* findData, void* context, bool* enumerationShouldContinue);

MOCKABLE_FUNCTION_WITH_RETURNS(, int, for_each_in_folder, const char*, folder, ON_EACH_IN_FOLDER, on_each_in_folder, void*, context)(0, MU_FAILURE);

#ifdef __cplusplus
}
#endif

#endif /* FOR_EACH_IN_FOLDER_H */
