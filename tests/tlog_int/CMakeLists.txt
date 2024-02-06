#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName tlog_int)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
tlog_foo.c
)

set(${theseTestsName}_h_files
../../inc/c_util/tqueue.h
tlog_foo.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal c_util)
