// Copyright(C) Microsoft Corporation.All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>


#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"
#include "c_logging/xlogging.h"
#include "c_util/rc_string_array.h"

#include "c_util/external_command_helper.h"

TEST_DEFINE_ENUM_TYPE(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_RESULT_VALUES);

#ifndef EXTERNAL_COMMAND_PATH
#error EXTERNAL_COMMAND_PATH must be set!
#endif

#define EXE_STRING EXTERNAL_COMMAND_PATH

// If we are building with VLD, then the test tool also has VLD
// VLD is set to be disabled for the exe, but that still prints two lines:
//   Visual Leak Detector read settings from: PATH_TO_CMAKE_BUILD\external_command_helper_int_exe\ext\vld.ini
//   Visual Leak Detector is turned off.
// Just ignore those lines
#if defined _DEBUG && defined VLD_OPT_REPORT_TO_STDOUT
#define ADDITIONAL_LINE_COUNT 2
#else
#define ADDITIONAL_LINE_COUNT 0
#endif

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_initialize)
{
}

TEST_FUNCTION(external_command_helper_runs_with_no_output_returns_0)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int exit_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(EXE_STRING, &lines, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 0, exit_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 0 + ADDITIONAL_LINE_COUNT, lines->count);

    // cleanup
    rc_string_array_destroy(lines);
}

TEST_FUNCTION(external_command_helper_runs_with_no_output_returns_42)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int exit_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(EXE_STRING " 42", &lines, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 42, exit_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 0 + ADDITIONAL_LINE_COUNT, lines->count);

    // cleanup
    rc_string_array_destroy(lines);
}

TEST_FUNCTION(external_command_helper_runs_with_1_line_output_returns_0)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int exit_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(EXE_STRING " hello_world_line_1", &lines, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 0, exit_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 1 + ADDITIONAL_LINE_COUNT, lines->count);
    ASSERT_ARE_EQUAL(char_ptr, "hello_world_line_1", lines->string_array[0]->string);

    // cleanup
    rc_string_array_destroy(lines);
}

TEST_FUNCTION(external_command_helper_runs_with_3_lines_output_returns_0)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int exit_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(EXE_STRING " hello_world_line_1 the_next_line zzz", &lines, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 0, exit_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 3 + ADDITIONAL_LINE_COUNT, lines->count);
    ASSERT_ARE_EQUAL(char_ptr, "hello_world_line_1", lines->string_array[0]->string);
    ASSERT_ARE_EQUAL(char_ptr, "the_next_line", lines->string_array[1]->string);
    ASSERT_ARE_EQUAL(char_ptr, "zzz", lines->string_array[2]->string);

    // cleanup
    rc_string_array_destroy(lines);
}

TEST_FUNCTION(external_command_helper_runs_with_3_lines_output_returns_1)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int exit_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(EXE_STRING " hello_world_line_1 the_next_line zzz 1", &lines, &exit_code);

    // assert
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 1, exit_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 3 + ADDITIONAL_LINE_COUNT, lines->count);
    ASSERT_ARE_EQUAL(char_ptr, "hello_world_line_1", lines->string_array[0]->string);
    ASSERT_ARE_EQUAL(char_ptr, "the_next_line", lines->string_array[1]->string);
    ASSERT_ARE_EQUAL(char_ptr, "zzz", lines->string_array[2]->string);

    // cleanup
    rc_string_array_destroy(lines);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
