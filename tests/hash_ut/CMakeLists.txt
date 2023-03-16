#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName hash_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_cpp_files
hash_under_test.cpp
)

set(${theseTestsName}_h_files
../../inc/c_util/hash.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util)
