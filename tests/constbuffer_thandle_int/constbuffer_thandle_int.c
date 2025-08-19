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
#include "c_pal/thandle.h"

#include "c_util/constbuffer_thandle.h"

BEGIN_TEST_SUITE(constbuffer_thandle_int)

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

/* Tests for CONSTBUFFER_THANDLE_Create */

TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_with_valid_data_creates_copy_and_returns_valid_handle)
{
    ///arrange
    unsigned char source_data[] = { 0x11, 0x22, 0x33, 0x44, 0x55 };
    
    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    
    ///assert
    ASSERT_IS_NOT_NULL(handle);
    
    // Verify the content is accessible
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, sizeof(source_data), content->size);
    ASSERT_ARE_EQUAL(int, 0, memcmp(source_data, content->buffer, sizeof(source_data)));
    
    // Verify it's a copy, not the same pointer
    ASSERT_ARE_NOT_EQUAL(void_ptr, source_data, content->buffer);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_with_single_byte_creates_valid_handle)
{
    ///arrange
    unsigned char source_data[] = { 0xAB };
    
    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    
    ///assert
    ASSERT_IS_NOT_NULL(handle);
    
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 1, content->size);
    ASSERT_ARE_EQUAL(uint8_t, 0xAB, content->buffer[0]);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

TEST_FUNCTION(CONSTBUFFER_THANDLE_Create_with_zero_size_creates_empty_handle)
{
    ///arrange
    
    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(NULL, 0);
    
    ///assert
    ASSERT_IS_NOT_NULL(handle);
    
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    ASSERT_IS_NULL(content->buffer);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/* Tests for CONSTBUFFER_THANDLE_CreateWithMoveMemory */

TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_takes_ownership_of_allocated_memory)
{
    ///arrange
    unsigned char* allocated_buffer = gballoc_hl_malloc(4);
    ASSERT_IS_NOT_NULL(allocated_buffer);
    allocated_buffer[0] = 0xDE;
    allocated_buffer[1] = 0xAD;
    allocated_buffer[2] = 0xBE;
    allocated_buffer[3] = 0xEF;
    
    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithMoveMemory(allocated_buffer, 4);
    
    ///assert
    ASSERT_IS_NOT_NULL(handle);
    
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 4, content->size);
    ASSERT_ARE_EQUAL(void_ptr, allocated_buffer, content->buffer);  // Should be the same pointer
    ASSERT_ARE_EQUAL(uint8_t, 0xDE, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 0xAD, content->buffer[1]);
    ASSERT_ARE_EQUAL(uint8_t, 0xBE, content->buffer[2]);
    ASSERT_ARE_EQUAL(uint8_t, 0xEF, content->buffer[3]);
    
    ///cleanup
    // When we release the handle, it should automatically free the moved memory
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
    // Note: allocated_buffer is now freed and should not be accessed
}

TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithMoveMemory_with_zero_size_takes_ownership)
{
    ///arrange
    unsigned char* allocated_buffer = gballoc_hl_malloc(10);  // Allocate some memory
    ASSERT_IS_NOT_NULL(allocated_buffer);
    
    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithMoveMemory(allocated_buffer, 0);
    
    ///assert
    ASSERT_IS_NOT_NULL(handle);
    
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 0, content->size);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/* Tests for CONSTBUFFER_THANDLE_CreateWithCustomFree */

static void test_custom_free_function(void* context)
{
    // This is a real custom free function that actually frees the memory
    gballoc_hl_free(context);
}

TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateWithCustomFree_uses_custom_free_function_on_dispose)
{
    ///arrange
    unsigned char* allocated_buffer = gballoc_hl_malloc(3);
    ASSERT_IS_NOT_NULL(allocated_buffer);
    allocated_buffer[0] = 0x01;
    allocated_buffer[1] = 0x02;
    allocated_buffer[2] = 0x03;
    
    ///act
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_CreateWithCustomFree(allocated_buffer, 3, test_custom_free_function, allocated_buffer);
    
    ///assert
    ASSERT_IS_NOT_NULL(handle);
    
    const CONSTBUFFER_THANDLE* content = CONSTBUFFER_THANDLE_GetContent(handle);
    ASSERT_IS_NOT_NULL(content);
    ASSERT_ARE_EQUAL(uint32_t, 3, content->size);
    ASSERT_ARE_EQUAL(void_ptr, allocated_buffer, content->buffer);  // Should be the same pointer
    ASSERT_ARE_EQUAL(uint8_t, 0x01, content->buffer[0]);
    ASSERT_ARE_EQUAL(uint8_t, 0x02, content->buffer[1]);
    ASSERT_ARE_EQUAL(uint8_t, 0x03, content->buffer[2]);
    
    ///cleanup
    // When we release the handle, it should call test_custom_free_function with allocated_buffer
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

/* Tests for CONSTBUFFER_THANDLE_CreateFromOffsetAndSize */

TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_creates_view_into_original_buffer)
{
    ///arrange
    unsigned char source_data[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60 };
    THANDLE(CONSTBUFFER) original_handle = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    ASSERT_IS_NOT_NULL(original_handle);
    
    ///act
    THANDLE(CONSTBUFFER) offset_handle = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(original_handle, 2, 3);
    
    ///assert
    ASSERT_IS_NOT_NULL(offset_handle);
    
    const CONSTBUFFER_THANDLE* original_content = CONSTBUFFER_THANDLE_GetContent(original_handle);
    const CONSTBUFFER_THANDLE* offset_content = CONSTBUFFER_THANDLE_GetContent(offset_handle);
    
    ASSERT_IS_NOT_NULL(original_content);
    ASSERT_IS_NOT_NULL(offset_content);
    
    // Verify the offset buffer points into the original buffer
    ASSERT_ARE_EQUAL(uint32_t, 3, offset_content->size);
    ASSERT_ARE_EQUAL(void_ptr, original_content->buffer + 2, offset_content->buffer);
    
    // Verify the data content
    ASSERT_ARE_EQUAL(uint8_t, 0x30, offset_content->buffer[0]);  // source_data[2]
    ASSERT_ARE_EQUAL(uint8_t, 0x40, offset_content->buffer[1]);  // source_data[3]
    ASSERT_ARE_EQUAL(uint8_t, 0x50, offset_content->buffer[2]);  // source_data[4]
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&original_handle, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&offset_handle, NULL);
}

TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSize_with_full_range_returns_same_handle)
{
    ///arrange
    unsigned char source_data[] = { 0xAA, 0xBB, 0xCC };
    THANDLE(CONSTBUFFER) original_handle = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    ASSERT_IS_NOT_NULL(original_handle);
    
    ///act
    THANDLE(CONSTBUFFER) full_range_handle = CONSTBUFFER_THANDLE_CreateFromOffsetAndSize(original_handle, 0, 3);
    
    ///assert
    ASSERT_IS_NOT_NULL(full_range_handle);
    // Should return the same handle when requesting the full range
    ASSERT_ARE_EQUAL(void_ptr, original_handle, full_range_handle);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&original_handle, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&full_range_handle, NULL);
}

/* Tests for CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy */

TEST_FUNCTION(CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy_creates_independent_copy)
{
    ///arrange
    unsigned char source_data[] = { 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5 };
    THANDLE(CONSTBUFFER) original_handle = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    ASSERT_IS_NOT_NULL(original_handle);
    
    ///act
    THANDLE(CONSTBUFFER) copy_handle = CONSTBUFFER_THANDLE_CreateFromOffsetAndSizeWithCopy(original_handle, 1, 4);
    
    ///assert
    ASSERT_IS_NOT_NULL(copy_handle);
    
    const CONSTBUFFER_THANDLE* original_content = CONSTBUFFER_THANDLE_GetContent(original_handle);
    const CONSTBUFFER_THANDLE* copy_content = CONSTBUFFER_THANDLE_GetContent(copy_handle);
    
    ASSERT_IS_NOT_NULL(original_content);
    ASSERT_IS_NOT_NULL(copy_content);
    
    // Verify it's a copy, not pointing into the original buffer
    ASSERT_ARE_EQUAL(uint32_t, 4, copy_content->size);
    ASSERT_ARE_NOT_EQUAL(void_ptr, original_content->buffer + 1, copy_content->buffer);
    
    // Verify the data content is correct
    ASSERT_ARE_EQUAL(uint8_t, 0xF1, copy_content->buffer[0]);  // source_data[1]
    ASSERT_ARE_EQUAL(uint8_t, 0xF2, copy_content->buffer[1]);  // source_data[2]
    ASSERT_ARE_EQUAL(uint8_t, 0xF3, copy_content->buffer[2]);  // source_data[3]
    ASSERT_ARE_EQUAL(uint8_t, 0xF4, copy_content->buffer[3]);  // source_data[4]
    
    // Verify the data is the same as the original
    ASSERT_ARE_EQUAL(int, 0, memcmp(copy_content->buffer, original_content->buffer + 1, 4));
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&original_handle, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&copy_handle, NULL);
}

/* Tests for CONSTBUFFER_THANDLE_contain_same */

TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_returns_true_for_identical_content)
{
    ///arrange
    unsigned char data1[] = { 0x11, 0x22, 0x33 };
    unsigned char data2[] = { 0x11, 0x22, 0x33 };
    
    THANDLE(CONSTBUFFER) handle1 = CONSTBUFFER_THANDLE_Create(data1, sizeof(data1));
    THANDLE(CONSTBUFFER) handle2 = CONSTBUFFER_THANDLE_Create(data2, sizeof(data2));
    
    ASSERT_IS_NOT_NULL(handle1);
    ASSERT_IS_NOT_NULL(handle2);
    
    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(handle1, handle2);
    
    ///assert
    ASSERT_IS_TRUE(result);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle1, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&handle2, NULL);
}

TEST_FUNCTION(CONSTBUFFER_THANDLE_contain_same_returns_false_for_different_content)
{
    ///arrange
    unsigned char data1[] = { 0x11, 0x22, 0x33 };
    unsigned char data2[] = { 0x11, 0x22, 0x44 };  // Different last byte
    
    THANDLE(CONSTBUFFER) handle1 = CONSTBUFFER_THANDLE_Create(data1, sizeof(data1));
    THANDLE(CONSTBUFFER) handle2 = CONSTBUFFER_THANDLE_Create(data2, sizeof(data2));
    
    ASSERT_IS_NOT_NULL(handle1);
    ASSERT_IS_NOT_NULL(handle2);
    
    ///act
    bool result = CONSTBUFFER_THANDLE_contain_same(handle1, handle2);
    
    ///assert
    ASSERT_IS_FALSE(result);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle1, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&handle2, NULL);
}

/* Tests for multiple handle references */

TEST_FUNCTION(multiple_handles_to_same_data_share_content_properly)
{
    ///arrange
    unsigned char source_data[] = { 0xCA, 0xFE, 0xBA, 0xBE };
    
    ///act
    THANDLE(CONSTBUFFER) handle1 = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    THANDLE(CONSTBUFFER) handle2 = NULL;
    
    // Create a second handle by assignment (this should increment ref count)
    THANDLE_ASSIGN(CONSTBUFFER)(&handle2, handle1);
    
    ///assert
    ASSERT_IS_NOT_NULL(handle1);
    ASSERT_IS_NOT_NULL(handle2);
    
    const CONSTBUFFER_THANDLE* content1 = CONSTBUFFER_THANDLE_GetContent(handle1);
    const CONSTBUFFER_THANDLE* content2 = CONSTBUFFER_THANDLE_GetContent(handle2);
    
    ASSERT_IS_NOT_NULL(content1);
    ASSERT_IS_NOT_NULL(content2);
    
    // Both handles should point to the same content
    ASSERT_ARE_EQUAL(void_ptr, content1, content2);
    ASSERT_ARE_EQUAL(uint32_t, content1->size, content2->size);
    ASSERT_ARE_EQUAL(void_ptr, content1->buffer, content2->buffer);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle1, NULL);
    THANDLE_ASSIGN(CONSTBUFFER)(&handle2, NULL);
}

/* Tests for serialization functionality */

TEST_FUNCTION(CONSTBUFFER_THANDLE_get_serialization_size_returns_correct_size)
{
    ///arrange
    unsigned char source_data[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    ASSERT_IS_NOT_NULL(handle);
    
    ///act
    uint32_t serialization_size = CONSTBUFFER_THANDLE_get_serialization_size(handle);
    
    ///assert
    // Should be 1 byte for version + 4 bytes for size + data size
    ASSERT_ARE_EQUAL(uint32_t, 1 + 4 + sizeof(source_data), serialization_size);
    
    ///cleanup
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

TEST_FUNCTION(CONSTBUFFER_THANDLE_to_buffer_serializes_correctly)
{
    ///arrange
    unsigned char source_data[] = { 0xAA, 0xBB };
    THANDLE(CONSTBUFFER) handle = CONSTBUFFER_THANDLE_Create(source_data, sizeof(source_data));
    ASSERT_IS_NOT_NULL(handle);
    
    uint32_t serialized_size;
    
    ///act
    unsigned char* serialized_buffer = CONSTBUFFER_THANDLE_to_buffer(handle, NULL, NULL, &serialized_size);
    
    ///assert
    ASSERT_IS_NOT_NULL(serialized_buffer);
    ASSERT_ARE_EQUAL(uint32_t, 1 + 4 + 2, serialized_size);  // version + size + data
    
    // Verify version byte (should be 1)
    ASSERT_ARE_EQUAL(uint8_t, 1, serialized_buffer[0]);
    
    // Verify data content is at the end
    ASSERT_ARE_EQUAL(uint8_t, 0xAA, serialized_buffer[5]);
    ASSERT_ARE_EQUAL(uint8_t, 0xBB, serialized_buffer[6]);
    
    ///cleanup
    gballoc_hl_free(serialized_buffer);
    THANDLE_ASSIGN(CONSTBUFFER)(&handle, NULL);
}

END_TEST_SUITE(constbuffer_thandle_int)
