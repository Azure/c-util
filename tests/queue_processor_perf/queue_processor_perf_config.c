// Copyright(C) Microsoft Corporation.All rights reserved.

#include <stddef.h>  // for NULL

#include "queue_processor_perf_config.h"

/*the default queue_processor perf tests will use the below numbers for configuration*/
#define DEFAULT_TEST_RUNTIME_DEFINE 10 /*seconds*/

const QUEUE_PROCESSOR_PERF_CONFIG_DATA default_queue_processor_perf_config =
{
    DEFAULT_TEST_RUNTIME_DEFINE
};

const QUEUE_PROCESSOR_PERF_CONFIG_DATA* queue_processor_perf_config = NULL; /*intentionally set to NULL so the running code is forced to set it to something (like the default config:) before running the tests*/
