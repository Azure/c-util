// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"

BEGIN_TEST_SUITE(constbuffer_array_int)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result = gballoc_hl_init(NULL, NULL);
    ASSERT_ARE_EQUAL(int, 0, result);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/* constbuffer_array_remove_empty_buffers integration tests */

TEST_FUNCTION(constbuffer_array_remove_empty_buffers_with_realistic_mixed_data_scenario)
{
    ///arrange
    // Create a realistic scenario with actual data that might occur in Azure storage
    unsigned char header_data[] = { 0x00, 0x01, 0x02, 0x03 };          // 4-byte header
    unsigned char payload_data[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE };   // 5-byte payload
    unsigned char footer_data[] = { 0xFF, 0xFE };                      // 2-byte footer
    
    // Create array with mixed empty and non-empty buffers
    CONSTBUFFER_HANDLE buffers[6];
    buffers[0] = CONSTBUFFER_Create(header_data, sizeof(header_data));
    ASSERT_IS_NOT_NULL(buffers[0]);
    buffers[1] = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(buffers[1]);
    buffers[2] = CONSTBUFFER_Create(payload_data, sizeof(payload_data));
    ASSERT_IS_NOT_NULL(buffers[2]);
    buffers[3] = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(buffers[3]);
    buffers[4] = CONSTBUFFER_Create(footer_data, sizeof(footer_data));
    ASSERT_IS_NOT_NULL(buffers[4]);
    buffers[5] = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(buffers[5]);
    
    CONSTBUFFER_ARRAY_HANDLE original_array = constbuffer_array_create(buffers, 6);
    ASSERT_IS_NOT_NULL(original_array);
    
    ///act
    CONSTBUFFER_ARRAY_HANDLE filtered_array = constbuffer_array_remove_empty_buffers(original_array);
    
    ///assert
    ASSERT_IS_NOT_NULL(filtered_array);
    
    // Verify the filtered array contains only non-empty buffers
    uint32_t filtered_count;
    ASSERT_ARE_EQUAL(int, 0, constbuffer_array_get_buffer_count(filtered_array, &filtered_count));
    ASSERT_ARE_EQUAL(uint32_t, 3, filtered_count);
    
    // Verify each buffer in the filtered array is correct
    const CONSTBUFFER* filtered_buffer = constbuffer_array_get_buffer_content(filtered_array, 0);
    ASSERT_IS_NOT_NULL(filtered_buffer);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(header_data), filtered_buffer->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(header_data, filtered_buffer->buffer, sizeof(header_data)));
    
    filtered_buffer = constbuffer_array_get_buffer_content(filtered_array, 1);
    ASSERT_IS_NOT_NULL(filtered_buffer);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(payload_data), filtered_buffer->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(payload_data, filtered_buffer->buffer, sizeof(payload_data)));
    
    filtered_buffer = constbuffer_array_get_buffer_content(filtered_array, 2);
    ASSERT_IS_NOT_NULL(filtered_buffer);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(footer_data), filtered_buffer->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(footer_data, filtered_buffer->buffer, sizeof(footer_data)));
    
    ///cleanup
    for (uint32_t i = 0; i < 6; i++)
    {
        CONSTBUFFER_DecRef(buffers[i]);
    }
    constbuffer_array_dec_ref(original_array);
    constbuffer_array_dec_ref(filtered_array);
}

TEST_FUNCTION(constbuffer_array_remove_empty_buffers_with_large_realistic_dataset)
{
    ///arrange
    // Simulate a realistic scenario with multiple data blocks and interspersed empty buffers
    const uint32_t total_buffers = 20;
    const uint32_t expected_non_empty = 11;
    CONSTBUFFER_HANDLE buffers[20];
    uint32_t next_buffer_index = 0;
    
    // Create pattern: data, empty, data, data, empty, empty, data, etc.
    for (uint32_t i = 0; i < total_buffers; i++)
    {
        if (i % 5 == 1 || i % 5 == 4 || i % 7 == 6)  // Create a pattern with some empty buffers
        {
            buffers[i] = CONSTBUFFER_Create(NULL, 0);
        }
        else
        {
            // Create buffers with varying sizes and patterns
            uint32_t data_size = (i % 3) + 1;  // 1, 2, or 3 bytes
            unsigned char* data = gballoc_hl_malloc(data_size);
            ASSERT_IS_NOT_NULL(data);
            
            for (uint32_t j = 0; j < data_size; j++)
            {
                data[j] = (unsigned char)(0x10 + next_buffer_index + j);
            }
            next_buffer_index++;
            
            buffers[i] = CONSTBUFFER_CreateWithMoveMemory(data, data_size);
        }
        ASSERT_IS_NOT_NULL(buffers[i]);
    }
    
    CONSTBUFFER_ARRAY_HANDLE original_array = constbuffer_array_create(buffers, total_buffers);
    ASSERT_IS_NOT_NULL(original_array);
    
    ///act
    CONSTBUFFER_ARRAY_HANDLE filtered_array = constbuffer_array_remove_empty_buffers(original_array);
    
    ///assert
    ASSERT_IS_NOT_NULL(filtered_array);
    
    // Verify the filtered array contains only non-empty buffers
    uint32_t filtered_count;
    int result = constbuffer_array_get_buffer_count(filtered_array, &filtered_count);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, expected_non_empty, filtered_count);
    
    // Verify all buffers in the filtered array are non-empty
    for (uint32_t i = 0; i < filtered_count; i++)
    {
        const CONSTBUFFER* buffer = constbuffer_array_get_buffer_content(filtered_array, i);
        ASSERT_IS_NOT_NULL(buffer);
        ASSERT_IS_TRUE(buffer->size > 0);
        ASSERT_IS_NOT_NULL(buffer->buffer);
    }
    
    ///cleanup
    for (uint32_t i = 0; i < total_buffers; i++)
    {
        CONSTBUFFER_DecRef(buffers[i]);
    }
    constbuffer_array_dec_ref(original_array);
    constbuffer_array_dec_ref(filtered_array);
}

TEST_FUNCTION(constbuffer_array_remove_empty_buffers_preserves_buffer_content_and_order)
{
    ///arrange
    // Create buffers with specific content to verify preservation
    unsigned char data1[] = { 0x01, 0x02, 0x03 };
    unsigned char data2[] = { 0x04 };
    unsigned char data3[] = { 0x05, 0x06 };
    unsigned char data4[] = { 0x07, 0x08, 0x09, 0x0A };
    
    CONSTBUFFER_HANDLE buffers[8];
    buffers[0] = CONSTBUFFER_Create(data1, sizeof(data1));
    ASSERT_IS_NOT_NULL(buffers[0]);
    buffers[1] = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(buffers[1]);
    buffers[2] = CONSTBUFFER_Create(data2, sizeof(data2));
    ASSERT_IS_NOT_NULL(buffers[2]);
    buffers[3] = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(buffers[3]);
    buffers[4] = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(buffers[4]);
    buffers[5] = CONSTBUFFER_Create(data3, sizeof(data3));
    ASSERT_IS_NOT_NULL(buffers[5]);
    buffers[6] = CONSTBUFFER_Create(data4, sizeof(data4));
    ASSERT_IS_NOT_NULL(buffers[6]);
    buffers[7] = CONSTBUFFER_Create(NULL, 0);
    ASSERT_IS_NOT_NULL(buffers[7]);
    
    CONSTBUFFER_ARRAY_HANDLE original_array = constbuffer_array_create(buffers, 8);
    ASSERT_IS_NOT_NULL(original_array);
    
    ///act
    CONSTBUFFER_ARRAY_HANDLE filtered_array = constbuffer_array_remove_empty_buffers(original_array);
    
    ///assert
    ASSERT_IS_NOT_NULL(filtered_array);
    
    // Verify count and order
    uint32_t filtered_count;
    int result = constbuffer_array_get_buffer_count(filtered_array, &filtered_count);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 4, filtered_count);
    
    // Verify first buffer (data1)
    const CONSTBUFFER* buffer = constbuffer_array_get_buffer_content(filtered_array, 0);
    ASSERT_IS_NOT_NULL(buffer);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(data1), buffer->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data1, buffer->buffer, sizeof(data1)));
    
    // Verify second buffer (data2)
    buffer = constbuffer_array_get_buffer_content(filtered_array, 1);
    ASSERT_IS_NOT_NULL(buffer);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(data2), buffer->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data2, buffer->buffer, sizeof(data2)));
    
    // Verify third buffer (data3)
    buffer = constbuffer_array_get_buffer_content(filtered_array, 2);
    ASSERT_IS_NOT_NULL(buffer);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(data3), buffer->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data3, buffer->buffer, sizeof(data3)));
    
    // Verify fourth buffer (data4)
    buffer = constbuffer_array_get_buffer_content(filtered_array, 3);
    ASSERT_IS_NOT_NULL(buffer);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(data4), buffer->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(data4, buffer->buffer, sizeof(data4)));
    
    ///cleanup
    for (uint32_t i = 0; i < 8; i++)
    {
        CONSTBUFFER_DecRef(buffers[i]);
    }
    constbuffer_array_dec_ref(original_array);
    constbuffer_array_dec_ref(filtered_array);
}

TEST_FUNCTION(constbuffer_array_remove_empty_buffers_with_only_empty_buffers_returns_empty_array)
{
    ///arrange
    const uint32_t buffer_count = 2;
    CONSTBUFFER_HANDLE empty_buffers[2];
    
    for (uint32_t i = 0; i < buffer_count; i++)
    {
        empty_buffers[i] = CONSTBUFFER_Create(NULL, 0);
        ASSERT_IS_NOT_NULL(empty_buffers[i]);
    }
    
    CONSTBUFFER_ARRAY_HANDLE original_array = constbuffer_array_create(empty_buffers, buffer_count);
    ASSERT_IS_NOT_NULL(original_array);
    
    ///act
    CONSTBUFFER_ARRAY_HANDLE filtered_array = constbuffer_array_remove_empty_buffers(original_array);
    
    ///assert
    ASSERT_IS_NOT_NULL(filtered_array);
    uint32_t filtered_count;
    int result = constbuffer_array_get_buffer_count(filtered_array, &filtered_count);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, 0, filtered_count);
    
    ///cleanup
    for (uint32_t i = 0; i < buffer_count; i++)
    {
        CONSTBUFFER_DecRef(empty_buffers[i]);
    }
    constbuffer_array_dec_ref(original_array);
    constbuffer_array_dec_ref(filtered_array);
}

TEST_FUNCTION(constbuffer_array_remove_empty_buffers_with_no_empty_buffers_returns_identical_array)
{
    ///arrange
    unsigned char data1[] = { 0x11, 0x22 };
    unsigned char data2[] = { 0x33, 0x44, 0x55 };
    unsigned char data3[] = { 0x66 };
    
    CONSTBUFFER_HANDLE buffers[3];
    buffers[0] = CONSTBUFFER_Create(data1, sizeof(data1));
    ASSERT_IS_NOT_NULL(buffers[0]);
    buffers[1] = CONSTBUFFER_Create(data2, sizeof(data2));
    ASSERT_IS_NOT_NULL(buffers[1]);
    buffers[2] = CONSTBUFFER_Create(data3, sizeof(data3));
    ASSERT_IS_NOT_NULL(buffers[2]);
    
    CONSTBUFFER_ARRAY_HANDLE original_array = constbuffer_array_create(buffers, 3);
    ASSERT_IS_NOT_NULL(original_array);
    
    ///act
    CONSTBUFFER_ARRAY_HANDLE filtered_array = constbuffer_array_remove_empty_buffers(original_array);
    
    ///assert
    ASSERT_IS_NOT_NULL(filtered_array);
    
    // Array should have the same count
    uint32_t original_count, filtered_count;
    int result = constbuffer_array_get_buffer_count(original_array, &original_count);
    ASSERT_ARE_EQUAL(int, 0, result);
    result = constbuffer_array_get_buffer_count(filtered_array, &filtered_count);
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(uint32_t, original_count, filtered_count);
    ASSERT_ARE_EQUAL(uint32_t, 3, filtered_count);
    
    // Verify all buffers are identical
    for (uint32_t i = 0; i < filtered_count; i++)
    {
        const CONSTBUFFER* original_buffer = constbuffer_array_get_buffer_content(original_array, i);
        const CONSTBUFFER* filtered_buffer = constbuffer_array_get_buffer_content(filtered_array, i);
        
        ASSERT_ARE_EQUAL(uint32_t, original_buffer->size, filtered_buffer->size);
        ASSERT_ARE_EQUAL(int, 0, memcmp(
            original_buffer->buffer,
            filtered_buffer->buffer,
            original_buffer->size
        ));
    }
    
    ///cleanup
    for (uint32_t i = 0; i < 3; i++)
    {
        CONSTBUFFER_DecRef(buffers[i]);
    }
    constbuffer_array_dec_ref(original_array);
    constbuffer_array_dec_ref(filtered_array);
}

END_TEST_SUITE(constbuffer_array_int)