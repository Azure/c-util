// Copyright (C) Microsoft Corporation. All rights reserved.

#include <wchar.h>
#include <stdbool.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"
#include "c_pal/string_utils.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util_test_helpers/leak_log_checker.h"
#include "c_util/for_each_in_folder.h"

static const char expected_vld_warning[] = "WARNING: Visual Leak Detector detected memory leaks!";
static const char expected_no_leaks_vld_warning[] = "No memory leaks detected.";

#define COUNT_CHARS(A) (sizeof(A)/sizeof(char) - 1)

static int check_log_file(const char* folder, const WIN32_FIND_DATAA* findData, void* context, bool* enumerationShouldContinue)
{
    // open the file
    bool* leaks_found_flag_ptr = context;
    int result;

    if ((strcmp(findData->cFileName, ".") == 0) ||
        (strcmp(findData->cFileName, "..") == 0))
    {
        // nothing to do for these
        *enumerationShouldContinue = true;
        result = 0;
    }
    else
    {
        char* full_filename = sprintf_char("%s/%s", folder, findData->cFileName);
        if (full_filename == NULL)
        {
            LogError("sprintf_char failed");
            result = MU_FAILURE;
        }
        else
        {
            // this function opens the file as text file and checks whether there is a warning from VLD on the first line.
            // if there is then it prints the file and marks that leaks were detected by setting to true the flag passed
            // in the context
            FILE* log_file = fopen(full_filename, "rt");
            if (log_file == NULL)
            {
                LogInfo("Cannot open file %s, must be in use", full_filename);
            }
            else
            {
                char text_buffer[256];
                size_t total_file_size = 0;

                size_t bytes_read = fread(text_buffer, 1, sizeof(text_buffer), log_file);
                if (bytes_read < COUNT_CHARS(expected_vld_warning))
                {
                    LogInfo("Most likely not a leak log, unexpected file: %s", full_filename);
                }
                else
                {
                    if (strncmp(text_buffer, expected_no_leaks_vld_warning, COUNT_CHARS(expected_no_leaks_vld_warning)) == 0)
                    {
                        LogInfo("No leaks found in file: %s", full_filename);
                    }
                    else
                    {
                        LogInfo("Leaks found in file: %s", full_filename);

                        if (strncmp(text_buffer, expected_vld_warning, COUNT_CHARS(expected_vld_warning)) != 0)
                        {
                            LogInfo("Most likely not a leak log, first line does not match for file: %s", full_filename);
                            LogInfo("%*.*s", (int)bytes_read, (int)bytes_read, text_buffer);
                        }
                        else
                        {
                            while (bytes_read > 0)
                            {
                                LogInfo("%*.*s", (int)bytes_read, (int)bytes_read, text_buffer);
                                total_file_size += bytes_read;

                                // read next chunk
                                bytes_read = fread(text_buffer, 1, sizeof(text_buffer), log_file);
                            };

                            // signal we got leaks
                            *leaks_found_flag_ptr = true;
                        }
                    }
                }

                fclose(log_file);
            }

            free(full_filename);

            // we want to go through all files to actually log the leaks
            *enumerationShouldContinue = true;
            result = 0;
        }
    }

    return result;
}

LEAK_LOG_CHECKER_CHECK_RESULT leak_log_checker_check_folder(const wchar_t* folder_to_check)
{
    LEAK_LOG_CHECKER_CHECK_RESULT result;

    if (folder_to_check == NULL)
    {
        LogError("Invalid arguments: const wchar_t* folder_to_check=%ls", MU_WP_OR_NULL(folder_to_check));
        result = LEAK_LOG_CHECKER_CHECK_ERROR;
    }
    else
    {
        // go through each file in the folder
        char* folder_name = sprintf_char("%ls", folder_to_check);
        if (folder_name == NULL)
        {
            LogError("sprintf_char failed");
            result = LEAK_LOG_CHECKER_CHECK_ERROR;
        }
        else
        {
            bool leaks_found_flag = false;

            if (for_each_in_folder(folder_name, check_log_file, &leaks_found_flag) != 0)
            {
                LogError("for_each_in_folder failed");
                result = LEAK_LOG_CHECKER_CHECK_ERROR;
            }
            else
            {
                if (leaks_found_flag)
                {
                    LogError("Leaks found!");
                    result = LEAK_LOG_CHECKER_CHECK_LEAKS_FOUND;
                }
                else
                {
                    result = LEAK_LOG_CHECKER_CHECK_OK;
                }
            }

            free(folder_name);
        }
    }

    return result;
}
