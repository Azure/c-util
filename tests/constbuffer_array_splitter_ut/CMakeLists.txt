#Copyright (c) Microsoft. All rights reserved.

set(theseTestsName constbuffer_array_splitter_ut)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
../../src/constbuffer_array_splitter.c
)

set(${theseTestsName}_h_files
../../inc/c_util/constbuffer_array_splitter.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_util_reals)
