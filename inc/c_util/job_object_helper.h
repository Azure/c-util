// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef JOB_OBJECT_HELPER_H
#define JOB_OBJECT_HELPER_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif


#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MOCKABLE_FUNCTION(, int, job_object_helper_limit_resources, uint32_t, percent_pysical_memory, uint32_t, percent_cpu);

#ifdef __cplusplus
}
#endif

#endif // JOB_OBJECT_HELPER_H
