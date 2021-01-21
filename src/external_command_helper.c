// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>
#include <stdio.h>

#if !_WIN32
#include <errno.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/thandle.h"
#include "c_util/rc_string.h"
#include "c_util/rc_string_array.h"

#include "c_util/external_command_helper.h"

#if _WIN32
#define popen _popen
#define pclose _pclose
#define POPEN_MODE "rt"
#define PCLOSE_RETURN_SHIFT 0
#else
#define POPEN_MODE "r"
#define PCLOSE_RETURN_SHIFT 8
#endif

MU_DEFINE_ENUM_STRINGS(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_RESULT_VALUES);

#define COMMAND_OUTPUT_BUFFER_SIZE 2048

IMPLEMENT_MOCKABLE_FUNCTION(, EXTERNAL_COMMAND_RESULT, external_command_helper_execute, const char*, command, RC_STRING_ARRAY**, lines, int*, return_code)
{
    EXTERNAL_COMMAND_RESULT result;

    if (
        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_001: [ If command is NULL then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
        command == NULL ||
        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_002: [ If command is empty string then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
        command[0] == '\0' ||
        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_003: [ If lines is NULL then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
        lines == NULL ||
        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_004: [ If return_code is NULL then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_INVALID_ARGS. ]*/
        return_code == NULL
        )
    {
        LogError("Invalid args: const char* command = %s, RC_STRING_ARRAY** lines = %p, int* return_code = %p",
            command, lines, return_code);
        result = EXTERNAL_COMMAND_INVALID_ARGS;
    }
    else
    {
        LogInfo("Executing command: '%s'", command);

        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_005: [ external_command_helper_execute shall call popen to execute the command and open a read pipe. ]*/
        FILE* command_pipe = popen(command, POPEN_MODE);

        if (command_pipe == NULL)
        {
            /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_017: [ If there are any other errors then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
            #if _WIN32
            LogError("popen(\"%s\", \"" MU_TOSTRING(POPEN_MODE) "\") failed", command);
            #else
            LogError("popen(\"%s\", \"" MU_TOSTRING(POPEN_MODE) "\") failed errno=%d", command, errno);
            #endif
            result = EXTERNAL_COMMAND_ERROR;
        }
        else
        {
            THANDLE(RC_STRING)* line_array = NULL;
            uint32_t line_count = 0;

            /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_006: [ external_command_helper_execute shall read each line of output into a 2048 byte buffer. ]*/
            char command_output_buffer[COMMAND_OUTPUT_BUFFER_SIZE];
            bool failed = false;

            while (fgets(command_output_buffer, COMMAND_OUTPUT_BUFFER_SIZE, command_pipe) != NULL)
            {
                LogInfo("Command output: %s", command_output_buffer);
                if (failed)
                {
                    // Already failed, but it may be helpful to see the full output for debugging
                    LogError("Skipping over buffer output: %s", command_output_buffer);
                }
                else
                {
                    size_t original_buffer_len = strlen(command_output_buffer);
                    size_t buffer_len = strcspn(command_output_buffer, "\r\n");

                    if (original_buffer_len == buffer_len)
                    {
                        // This means there was no newline at the end, which means the line was longer than our buffer
                        // Just fail because we don't expect that output

                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_007: [ If a line of output exceeds 2048 bytes then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
                        LogError("command (%s) returned unexpected output: %s", command, command_output_buffer);
                        failed = true;
                    }
                    else
                    {
                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_008: [ external_command_helper_execute shall remove the trailing new line. ]*/
                        command_output_buffer[buffer_len] = '\0';

                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_009: [ external_command_helper_execute shall allocate an array of THANDLE(RC_STRING) or grow the existing array to fit the new line. ]*/
                        size_t array_bytes_required = sizeof(THANDLE(RC_STRING)) * (line_count + 1);
                        THANDLE(RC_STRING)* line_array_temp = realloc((void*)line_array, array_bytes_required);

                        if (line_array_temp == NULL)
                        {
                            /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_017: [ If there are any other errors then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
                            LogError("realloc(%zu) failed", array_bytes_required);
                            failed = true;
                        }
                        else
                        {
                            line_array = line_array_temp;
                            ++line_count;

                            /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_010: [ external_command_helper_execute shall allocate a THANDLE(RC_STRING) of the line by calling rc_string_create and store it in the array. ]*/
                            THANDLE(RC_STRING) temp = rc_string_create(command_output_buffer);

                            if (temp == NULL)
                            {
                                /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_017: [ If there are any other errors then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
                                LogError("rc_string_create(%s) failed", command_output_buffer);
                                THANDLE_INITIALIZE(RC_STRING)(&line_array[line_count - 1], NULL);
                                failed = true;
                            }
                            else
                            {
                                THANDLE_INITIALIZE_MOVE(RC_STRING)(&line_array[line_count - 1], &temp);
                            }
                        }
                    }
                }
            }

            if (!failed && feof(command_pipe) == 0)
            {
                /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_017: [ If there are any other errors then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
                LogError("End of stream was not reached! (ferror=%d)", ferror(command_pipe));
                result = EXTERNAL_COMMAND_ERROR;
            }
            else
            {
                /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_013: [ external_command_helper_execute shall call pclose to close the pipe and get the exit code of the command. ]*/
                int command_exit = pclose(command_pipe);

                if (failed)
                {
                    /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_017: [ If there are any other errors then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
                    LogError("Failure while processing output (exit code=%d)", command_exit);
                    result = EXTERNAL_COMMAND_ERROR;
                }
                else
                {
                    /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_011: [ external_command_helper_execute shall call rc_string_array_create with the count of output lines returned. ]*/
                    RC_STRING_ARRAY* rc_string_array_temp = rc_string_array_create(line_count);

                    if (rc_string_array_temp == NULL)
                    {
                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_017: [ If there are any other errors then external_command_helper_execute shall fail and return EXTERNAL_COMMAND_ERROR. ]*/
                        LogError("rc_string_array_create(%" PRIu32 ") failed", line_count);
                        result = EXTERNAL_COMMAND_ERROR;
                    }
                    else
                    {
                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_012: [ external_command_helper_execute shall move all of the strings into the allocated RC_STRING_ARRAY. ]*/
                        for (uint32_t i = 0; i < line_count; ++i)
                        {
                            THANDLE_MOVE(RC_STRING)(&rc_string_array_temp->string_array[i], &line_array[i]);
                        }
                        free((void*)line_array);
                        line_array = NULL;

                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_014: [ external_command_helper_execute shall store the exit code of the command in return_code. ]*/
                        *return_code = command_exit >> PCLOSE_RETURN_SHIFT;

                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_015: [ external_command_helper_execute shall store the allocated RC_STRING_ARRAY in lines. ]*/
                        *lines = rc_string_array_temp;

                        /*Codes_SRS_EXTERNAL_COMMAND_HELPER_42_016: [ external_command_helper_execute shall succeed and return EXTERNAL_COMMAND_OK. ]*/
                        result = EXTERNAL_COMMAND_OK;
                    }
                }
            }

            if (line_array != NULL)
            {
                for (uint32_t i = 0; i < line_count; ++i)
                {
                    THANDLE_DEC_REF(RC_STRING)(line_array[i]);
                }
                free((void*)line_array);
            }
        }
    }

    return result;
}