#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName interlocked_hl_win32_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
    )

set(${theseTestsName}_c_files
../../adapters/interlocked_hl_win32.c
)

set(${theseTestsName}_h_files
../../inc/azure_c_util/interlocked_hl.h
../mocked/inc/windows.h
)

include_directories(BEFORE SYSTEM ../mocked/inc)

build_c_tests(${theseTestsName} ON "tests/azure_c_util_tests")
