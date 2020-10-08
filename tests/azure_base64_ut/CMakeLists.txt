#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 2.8.11)

set(theseTestsName azure_base64_ut)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/azure_base64.c
    ../../src/strings.c
    ../../src/buffer.c
)

set(${theseTestsName}_h_files
)

build_test_artifacts(${theseTestsName} ON "tests/azure_c_util" ADDITIONAL_LIBS azure_c_pal azure_c_pal_reals)
