#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(theseTestsName tarray_int)

set(${theseTestsName}_test_files
${theseTestsName}.c
)

set(${theseTestsName}_c_files
play.c
)

set(${theseTestsName}_h_files
play.h
play_undo_op_types.h
play_undo_op_tarray_types.h
)

build_test_artifacts(${theseTestsName} "tests/c_util" ADDITIONAL_LIBS c_pal c_pal_reals c_util_reals)

