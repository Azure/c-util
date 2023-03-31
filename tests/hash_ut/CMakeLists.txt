#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName hash_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_cpp_files
hash_under_test.cpp
)

set(${theseTestsName}_h_files
../../inc/c_util/hash.h
../../deps/smhasher/src/MurmurHash2.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util)
