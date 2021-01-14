// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstdint>
#else
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"

static void* my_gballoc_malloc(size_t size)
{
    return real_gballoc_ll_malloc(size);
}

static void* my_gballoc_realloc(void* ptr, size_t size)
{
    return real_gballoc_ll_realloc(ptr, size);
}

static void my_gballoc_free(void* ptr)
{
     real_gballoc_ll_free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

#include "c_logging/xlogging.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_util/rc_string.h"
#include "c_util/rc_string_array.h"

#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, int, mocked_pclose,
    FILE*, stream);

MOCKABLE_FUNCTION(, FILE*, mocked_popen,
    const char*, command,
    const char*, mode);

#ifdef __cplusplus
}
#endif

#undef ENABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in real_rc_string
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_rc_string.h"
#include "real_rc_string_array.h"

#include "c_util/external_command_helper.h"


#if _WIN32
#define POPEN_MODE "rt"
#define PCLOSE_RETURN_SHIFT 0
#else
#define POPEN_MODE "r"
#define PCLOSE_RETURN_SHIFT 8
#endif

static TEST_MUTEX_HANDLE test_serialize_mutex;
MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static const char* test_command = "my_program.exe -arg1 -arg2";

#define test_line1 "line1"
#define test_line2 "another line"
#define test_line3 "something else"
#define test_line4 "42"
#define test_line5 "**&(#(*$&#@(*$"

TEST_DEFINE_ENUM_TYPE(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_RESULT_VALUES);

static char temp_file_name[L_tmpnam];
static FILE* last_opened_file_handle;
static bool pclose_is_pending;

static int pclose_override_return;
static int hook_pclose(FILE* stream)
{
    ASSERT_IS_TRUE(pclose_is_pending, "mocked version of pclose should not have been called without a matching popen");

    // Close and delete the temp file
    ASSERT_ARE_EQUAL(int, 0, fclose(stream));

    remove(temp_file_name);
    LogInfo("Deleted temp file: %s", temp_file_name);

    last_opened_file_handle = NULL;
    pclose_is_pending = false;

    return pclose_override_return;
}

static char* test_data_to_report_as_output;

static FILE* hook_popen(
    char const* command,
    char const* mode)
{
    FILE* result;

    (void)command;
    (void)mode;

    ASSERT_IS_FALSE(pclose_is_pending, "mocked version of popen only supports one call at a time");
    pclose_is_pending = true;

    // Pick a file name
    ASSERT_IS_NOT_NULL(tmpnam(temp_file_name));

    // Open file to fill with test data
    FILE* temp = fopen(temp_file_name, "w");

    ASSERT_IS_TRUE(fputs(test_data_to_report_as_output, temp) >= 0);

    ASSERT_ARE_EQUAL(int, 0, fclose(temp));

    // Open file for caller
    result = fopen(temp_file_name, mode);
    last_opened_file_handle = result;

    LogInfo("Created temp file: %s", temp_file_name);

    return result;
}

static void expect_run_command(FILE** captured_file_handle)
{
    STRICT_EXPECTED_CALL(mocked_popen(test_command, POPEN_MODE))
        .CaptureReturn(captured_file_handle);
}

static void expect_read_line(const char* line)
{
    STRICT_EXPECTED_CALL(realloc(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(rc_string_create(line));
    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE_MOVE(RC_STRING)(IGNORED_ARG, IGNORED_ARG));
}

static void expect_end_command(FILE** captured_file_handle, int exit_code)
{
    pclose_override_return = exit_code << PCLOSE_RETURN_SHIFT;
    STRICT_EXPECTED_CALL(mocked_pclose(IGNORED_ARG))
        .ValidateArgumentValue_stream(captured_file_handle)
        .CallCannotFail();
}

static void expect_store_lines(uint32_t line_count)
{
    STRICT_EXPECTED_CALL(rc_string_array_create(line_count));
    for (uint32_t i = 0; i < line_count; ++i)
    {
        STRICT_EXPECTED_CALL(THANDLE_MOVE(RC_STRING)(IGNORED_ARG, IGNORED_ARG));
    }
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
}

BEGIN_TEST_SUITE(external_command_helper_unittests)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();
    REGISTER_RC_STRING_ARRAY_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_HOOK(realloc, my_gballoc_realloc);
    REGISTER_GLOBAL_MOCK_HOOK(free, my_gballoc_free);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc, NULL);

    REGISTER_GLOBAL_MOCK_HOOK(mocked_pclose, hook_pclose);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pclose, -1);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_popen, hook_popen);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_popen, NULL);

    REGISTER_UMOCK_ALIAS_TYPE(FILE*, void*);
    REGISTER_UMOCK_ALIAS_TYPE(THANDLE(RC_STRING), void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(test_serialize_mutex);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    test_data_to_report_as_output = "";
    pclose_override_return = 0;

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_001: [ If command is NULL then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
TEST_FUNCTION(external_command_helper_execute_with_NULL_command_fails)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(NULL, &lines, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_INVALID_ARGS, result);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_002: [ If command is empty string then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
TEST_FUNCTION(external_command_helper_execute_with_empty_command_fails)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute("", &lines, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_INVALID_ARGS, result);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_003: [ If lines is NULL then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
TEST_FUNCTION(external_command_helper_execute_with_NULL_lines_fails)
{
    // arrange
    int return_code;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, NULL, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_INVALID_ARGS, result);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_004: [ If return_code is NULL then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
TEST_FUNCTION(external_command_helper_execute_with_NULL_return_code_fails)
{
    // arrange
    RC_STRING_ARRAY* lines;

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, &lines, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_INVALID_ARGS, result);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_005: [ external_command_helper_execute shall call popen to execute the command and open a read pipe. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_006: [ external_command_helper_execute shall read each line of output into a 2048 byte buffer. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_008: [ external_command_helper_execute shall remove the trailing new line. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_009: [ external_command_helper_execute shall allocate an array of THANDLE(RC_STRING) or grow the existing array to fit the new line. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_010: [ external_command_helper_execute shall allocate a THANDLE(RC_STRING) of the line by calling rc_string_create and store it in the array. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_011: [ external_command_helper_execute shall call rc_string_array_create with the count of output lines returned. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_012: [ external_command_helper_execute shall move all of the strings into the allocated RC_STRING_ARRAY. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_013: [ external_command_helper_execute shall call pclose to close the pipe and get the exit code of the command. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_014: [ external_command_helper_execute shall store the exit code of the command in return_code. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_015: [ external_command_helper_execute shall store the allocated RC_STRING_ARRAY in lines. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_016: [ external_command_helper_execute shall succeed and return EXTERNAL_COMMAND_OK. ]*/
TEST_FUNCTION(external_command_helper_execute_succeeds_returns_1_line)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    test_data_to_report_as_output = test_line1 "\n";

    FILE* captured_file;
    expect_run_command(&captured_file);
    expect_read_line(test_line1);
    expect_end_command(&captured_file, 0);
    expect_store_lines(1);

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, &lines, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 0, return_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 1, lines->count);
    ASSERT_ARE_EQUAL(char_ptr, test_line1, lines->string_array[0]->string);

    // cleanup
    real_rc_string_array_destroy(lines);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_005: [ external_command_helper_execute shall call popen to execute the command and open a read pipe. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_006: [ external_command_helper_execute shall read each line of output into a 2048 byte buffer. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_008: [ external_command_helper_execute shall remove the trailing new line. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_009: [ external_command_helper_execute shall allocate an array of THANDLE(RC_STRING) or grow the existing array to fit the new line. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_010: [ external_command_helper_execute shall allocate a THANDLE(RC_STRING) of the line by calling rc_string_create and store it in the array. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_011: [ external_command_helper_execute shall call rc_string_array_create with the count of output lines returned. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_012: [ external_command_helper_execute shall move all of the strings into the allocated RC_STRING_ARRAY. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_013: [ external_command_helper_execute shall call pclose to close the pipe and get the exit code of the command. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_014: [ external_command_helper_execute shall store the exit code of the command in return_code. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_015: [ external_command_helper_execute shall store the allocated RC_STRING_ARRAY in lines. ]*/
/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_016: [ external_command_helper_execute shall succeed and return EXTERNAL_COMMAND_OK. ]*/
TEST_FUNCTION(external_command_helper_execute_succeeds_returns_5_lines)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    test_data_to_report_as_output = test_line1 "\n" test_line2 "\n" test_line3 "\n" test_line4 "\n" test_line5 "\n";

    FILE* captured_file;
    expect_run_command(&captured_file);
    expect_read_line(test_line1);
    expect_read_line(test_line2);
    expect_read_line(test_line3);
    expect_read_line(test_line4);
    expect_read_line(test_line5);
    expect_end_command(&captured_file, 0);
    expect_store_lines(5);

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, &lines, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 0, return_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 5, lines->count);
    ASSERT_ARE_EQUAL(char_ptr, test_line1, lines->string_array[0]->string);
    ASSERT_ARE_EQUAL(char_ptr, test_line2, lines->string_array[1]->string);
    ASSERT_ARE_EQUAL(char_ptr, test_line3, lines->string_array[2]->string);
    ASSERT_ARE_EQUAL(char_ptr, test_line4, lines->string_array[3]->string);
    ASSERT_ARE_EQUAL(char_ptr, test_line5, lines->string_array[4]->string);

    // cleanup
    real_rc_string_array_destroy(lines);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_007: [ If a line of output exceeds 2048 bytes then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
TEST_FUNCTION(external_command_helper_execute_with_line_too_long_fails)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    char* long_line = (char*)my_gballoc_malloc(2048 + 2);
    ASSERT_IS_NOT_NULL(long_line);
    for (uint32_t i = 0; i < 2048; ++i)
    {
        long_line[i] = 'a';
    }
    long_line[2048] = '\n';
    long_line[2049] = '\0';

    test_data_to_report_as_output = long_line;

    FILE* captured_file;
    expect_run_command(&captured_file);
    expect_end_command(&captured_file, 0);

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, &lines, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_ERROR, result);

    // cleanup
    my_gballoc_free(long_line);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_007: [ If a line of output exceeds 2048 bytes then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
TEST_FUNCTION(external_command_helper_execute_with_second_line_too_long_fails)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    char* long_line = (char*)my_gballoc_malloc(2 + 2048 + 2);
    ASSERT_IS_NOT_NULL(long_line);
    long_line[0] = 'z';
    long_line[1] = '\n';
    for (uint32_t i = 2; i < 2 + 2048; ++i)
    {
        long_line[i] = 'a';
    }
    long_line[2 + 2048] = '\n';
    long_line[2 + 2049] = '\0';

    test_data_to_report_as_output = long_line;

    FILE* captured_file;
    expect_run_command(&captured_file);
    expect_read_line("z");
    expect_end_command(&captured_file, 0);
    STRICT_EXPECTED_CALL(THANDLE_DEC_REF(RC_STRING)(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, &lines, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_ERROR, result);

    // cleanup
    my_gballoc_free(long_line);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_014: [ external_command_helper_execute shall store the exit code of the command in return_code. ]*/
TEST_FUNCTION(external_command_helper_execute_succeeds_with_3_lines_bad_exit_code)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    test_data_to_report_as_output = test_line1 "\n" test_line2 "\n" test_line3 "\n";

    FILE* captured_file;
    expect_run_command(&captured_file);
    expect_read_line(test_line1);
    expect_read_line(test_line2);
    expect_read_line(test_line3);
    expect_end_command(&captured_file, 42);
    expect_store_lines(3);

    // act
    EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, &lines, &return_code);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_OK, result);
    ASSERT_ARE_EQUAL(int, 42, return_code);
    ASSERT_IS_NOT_NULL(lines);
    ASSERT_ARE_EQUAL(uint32_t, 3, lines->count);
    ASSERT_ARE_EQUAL(char_ptr, test_line1, lines->string_array[0]->string);
    ASSERT_ARE_EQUAL(char_ptr, test_line2, lines->string_array[1]->string);
    ASSERT_ARE_EQUAL(char_ptr, test_line3, lines->string_array[2]->string);

    // cleanup
    real_rc_string_array_destroy(lines);
}

/*Tests_SRS_EXTERNAL_COMMAND_HELPER_42_017: [ If there are any other errors then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
TEST_FUNCTION(external_command_helper_execute_with_5_lines_fails_when_underlying_functions_fail)
{
    // arrange
    RC_STRING_ARRAY* lines;
    int return_code;

    test_data_to_report_as_output = test_line1 "\n" test_line2 "\n" test_line3 "\n" test_line4 "\n" test_line5 "\n";

    FILE* captured_file;
    expect_run_command(&captured_file);
    expect_read_line(test_line1);
    expect_read_line(test_line2);
    expect_read_line(test_line3);
    expect_read_line(test_line4);
    expect_read_line(test_line5);
    expect_end_command(&captured_file, 0);
    expect_store_lines(5);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            EXTERNAL_COMMAND_RESULT result = external_command_helper_execute(test_command, &lines, &return_code);

            // assert
            ASSERT_ARE_EQUAL(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_ERROR, result, "On failed call %zu", i);

            // If we fail _popen, our mock hook actually requires some cleanup
            if (pclose_is_pending)
            {
                hook_pclose(last_opened_file_handle);
            }
        }
    }
}

END_TEST_SUITE(external_command_helper_unittests)
