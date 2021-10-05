#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

set(tarray_int_reals_c_files
    real_tarray_undo_op.c
)

set(tarray_int_reals_h_files
    real_tarray_undo_op.h
    real_tarray_undo_op_renames.h
)

add_library(tarray_int_reals ${tarray_int_reals_c_files} ${tarray_int_reals_h_files})
target_include_directories(tarray_int_reals PUBLIC .)
target_link_libraries(tarray_int_reals c_logging c_pal_reals)
