#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName queue_processor_perf)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
queue_processor_perf_config.c
)

set(${theseTestsName}_h_files
queue_processor_perf_config.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util clds)
