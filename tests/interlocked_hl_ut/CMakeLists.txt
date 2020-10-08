#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName interlocked_hl_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/interlocked_hl.c
)

set(${theseTestsName}_h_files
../../inc/azure_c_util/interlocked_hl.h
../mocked/inc/windows.h
)

include_directories(BEFORE SYSTEM ../mocked/inc)

build_test_artifacts(${theseTestsName} ON "tests/azure_c_util" ADDITIONAL_LIBS azure_c_pal azure_c_pal_reals)
