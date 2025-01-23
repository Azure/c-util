// Copyright(C) Microsoft Corporation.All rights reserved.

/*queue_processor_perf tests require that a configuration is provided to them*/
/*the "configuration" is a pointer called queue_processor_perf_config*/
/*queue_processor_perf_config cannot be NULL (that would indicate a mistake in the driving code), as in "it did not prepare the pointer by assigning it to something"*/

#ifndef QUEUE_PROCESSOR_PERF_CONFIG_H
#define QUEUE_PROCESSOR_PERF_CONFIG_H

#include <stdint.h>

typedef struct QUEUE_PROCESSOR_PERF_CONFIG_DATA_TAG
{
    /*for how long should the test run*/
    double test_runtime;
} QUEUE_PROCESSOR_PERF_CONFIG_DATA;

extern const QUEUE_PROCESSOR_PERF_CONFIG_DATA* queue_processor_perf_config; /*the pointer that is read by the test*/
extern const QUEUE_PROCESSOR_PERF_CONFIG_DATA default_queue_processor_perf_config; /*out of the box configuration, provided for convenience*/

#endif // QUEUE_PROCESSOR_PERF_CONFIG_H
