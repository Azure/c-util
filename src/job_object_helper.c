// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "stdint.h"

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_util/job_object_helper.h"


IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_limit_resources, uint32_t, percent_pysical_memory, uint32_t, percent_cpu)
{
    (void) percent_pysical_memory;
    (void) percent_cpu;
    return MU_FAILURE;
}