#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName thandle_2_ut) #these tests are crafted to only prove the usage of different patterns for memory allocation (global. at type define time, at instantiation time)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
    g_on_t_off.c
    g_on_t_on.c
)

set(${theseTestsName}_h_files
    ../../inc/c_util/thandle.h
    ../../inc/c_util/thandle_ll.h
    malloc_mocks.h
    g_on_t_off.h
    g_on_t_on.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal c_pal_reals)

