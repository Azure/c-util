// Copyright (C) Microsoft Corporation. All rights reserved.

#include <wchar.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/string_utils.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util_test_helpers/guid_as_string.h"

wchar_t* leak_log_folder_create(const char* leak_log_root_folder_name)
{
    wchar_t* result = NULL;

    const char* guid = getGuidAsString();
    if (guid == NULL)
    {
        LogError("getGuidAsString failed");
    }
    else
    {
        char computer_name[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD buffer_size = sizeof(computer_name);

        if (!GetComputerNameA(computer_name, &buffer_size))
        {
            LogLastError("GetComputerNameA failed");
        }
        else
        {
            char* leak_log_folder_full_path = sprintf_char("\\\\%s\\%s\\%s", computer_name, leak_log_root_folder_name, guid);
            if (leak_log_folder_full_path == NULL)
            {
                LogError("sprintf_char failed");
            }
            else
            {
                wchar_t* leak_log_folder_name_wchar = mbs_to_wcs(leak_log_folder_full_path);
                if (leak_log_folder_name_wchar == NULL)
                {
                    LogError("mbs_to_wcs failed");
                }
                else
                {
                    char* leak_log_folder_name_command = sprintf_char("mkdir %s 1>nul 2>nul", leak_log_folder_full_path);
                    if (leak_log_folder_name_command == NULL)
                    {
                        LogError("sprintf_char failed");
                    }
                    else
                    {
                        errno = 0;
                        if ((system(leak_log_folder_name_command) != 0) ||
                            (errno != 0))
                        {
                            LogError("system call failed: %s", leak_log_folder_name_command);
                        }
                        else
                        {
                            result = leak_log_folder_name_wchar;
                            leak_log_folder_name_wchar = NULL;
                        }

                        free(leak_log_folder_name_command);
                    }

                    if (leak_log_folder_name_wchar != NULL)
                    {
                        free(leak_log_folder_name_wchar);
                    }
                }

                free(leak_log_folder_full_path);
            }
        }

        freeGuidAsString(guid);
    }

    return result;
}

void leak_log_folder_destroy(wchar_t* leak_log_folder)
{
    if (leak_log_folder == NULL)
    {
        LogError("Invalid arguments: wchar_t* leak_log_folder=%ls", MU_WP_OR_NULL(leak_log_folder));
    }
    else
    {
        char* leak_log_folder_delete_command = sprintf_char("rmdir /s /q %ls 1>nul 2>nul", leak_log_folder);
        if (leak_log_folder_delete_command != NULL)
        {
            (void)system(leak_log_folder_delete_command);

            free(leak_log_folder_delete_command);
        }

        free(leak_log_folder);
    }
}
