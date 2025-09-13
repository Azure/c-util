// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_CONSTBUFFER_ARRAY_H
#define REAL_CONSTBUFFER_ARRAY_H


#include <stdint.h>


#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CONSTBUFFER_ARRAY_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        constbuffer_array_create, \
        constbuffer_array_create_with_move_buffers, \
        constbuffer_array_create_from_buffer_index_and_count, \
        constbuffer_array_create_from_buffer_offset_and_count, \
        constbuffer_array_create_empty, \
        constbuffer_array_create_from_array_array, \
        constbuffer_array_inc_ref, \
        constbuffer_array_dec_ref, \
        constbuffer_array_add_front, \
        constbuffer_array_remove_front, \
        constbuffer_array_add_back, \
        constbuffer_array_remove_back, \
        constbuffer_array_get_buffer_count, \
        constbuffer_array_get_buffer, \
        constbuffer_array_get_buffer_content, \
        constbuffer_array_get_all_buffers_size, \
        constbuffer_array_get_const_buffer_handle_array, \
        constbuffer_array_remove_empty_buffers, \
        CONSTBUFFER_ARRAY_HANDLE_contain_same \
)

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"



CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_create(const CONSTBUFFER_HANDLE* buffers, uint32_t buffer_count);
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_create_empty(void);
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_create_with_move_buffers(CONSTBUFFER_HANDLE* buffers, uint32_t buffer_count);
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_create_from_buffer_index_and_count(CONSTBUFFER_ARRAY_HANDLE original, uint32_t start_buffer_index, uint32_t buffer_count);
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_create_from_buffer_offset_and_count(CONSTBUFFER_ARRAY_HANDLE original, uint32_t start_buffer_index, uint32_t buffer_count, uint32_t start_buffer_offset, uint32_t end_buffer_size);
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_create_from_array_array(const CONSTBUFFER_ARRAY_HANDLE* buffer_arrays, uint32_t buffer_array_count);

void real_constbuffer_array_inc_ref(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle);
void real_constbuffer_array_dec_ref(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle);

/*add in front*/
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_add_front(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, CONSTBUFFER_HANDLE constbuffer_handle);

/*remove front*/
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_remove_front(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, CONSTBUFFER_HANDLE* constbuffer_handle);

/*add in back*/
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_add_back(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, CONSTBUFFER_HANDLE constbuffer_handle);

/*remove back*/
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_remove_back(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, CONSTBUFFER_HANDLE* constbuffer_handle);

/*remove empty buffers*/
CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_remove_empty_buffers(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle);

/* getters */
int real_constbuffer_array_get_buffer_count(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, uint32_t* buffer_count);
CONSTBUFFER_HANDLE real_constbuffer_array_get_buffer(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, uint32_t buffer_index);
const CONSTBUFFER* real_constbuffer_array_get_buffer_content(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, uint32_t buffer_index);
int real_constbuffer_array_get_all_buffers_size(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle, uint32_t* all_buffers_size);
const CONSTBUFFER_HANDLE* real_constbuffer_array_get_const_buffer_handle_array(CONSTBUFFER_ARRAY_HANDLE constbuffer_array_handle);
bool real_CONSTBUFFER_ARRAY_HANDLE_contain_same(CONSTBUFFER_ARRAY_HANDLE left, CONSTBUFFER_ARRAY_HANDLE right);



#endif // REAL_CONSTBUFFER_ARRAY_H
