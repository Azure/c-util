// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define GBALLOC_H

#include "real_threadapi_renames.h"

#if _MSC_VER
#include "../../adapters/threadapi_win32.c"
#else
#include "../../adapters/threadapi_pthreads.c"
#endif
