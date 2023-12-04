// Copyright (C) Microsoft Corporation. All rights reserved.

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/string_utils.h"

#include "for_each_in_folder.h"

int for_each_in_folder(const char* folder, ON_EACH_IN_FOLDER on_each_in_folder, void* context)
{
    int result;
    if (
        /*Codes_SRS_FOR_EACH_IN_FOLDER_02_001: [ If folder is NULL then for_each_in_folder shall fail and return a non-zero value. ]*/
        (folder == NULL) ||
        /*Codes_SRS_FOR_EACH_IN_FOLDER_02_002: [ If on_each_in_folder is NULL then for_each_in_folder shall fail and return a non-zero value. ]*/
        (on_each_in_folder == NULL)
        )
    {
        LogError("invalid arguments const char* folder=%s, ON_EACH_IN_FOLDER on_each_in_folder=%p, void* context=%p",
            MU_P_OR_NULL(folder),
            on_each_in_folder,
            context);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_FOR_EACH_IN_FOLDER_02_003: [ for_each_in_folder shall assemble the string folder\\* to enumerate all constituents in folder. ]*/
        char* getEverything = sprintf_char("%s\\*", folder);
        if (getEverything == NULL)
        {
            /*Codes_SRS_FOR_EACH_IN_FOLDER_02_016: [ If there are any failures then for_each_in_folder shall fail and return a non-zero value. ]*/
            LogError("failure in sprintf_char");
            result = MU_FAILURE;
        }
        else
        {
            WIN32_FIND_DATAA findData;
            /*Codes_SRS_FOR_EACH_IN_FOLDER_02_004: [ for_each_in_folder shall call FindFirstFileA with lpFileName set to the previously build string. ]*/
            HANDLE hFind = FindFirstFileA(getEverything, &findData);
            if (hFind == INVALID_HANDLE_VALUE)
            {
                DWORD lastError = GetLastError();
                if (
                    (lastError == ERROR_FILE_NOT_FOUND) ||
                    (lastError == ERROR_PATH_NOT_FOUND)
                    )
                {
                    /*Codes_/*Codes_SRS_FOR_EACH_IN_FOLDER_02_005: [ If FindFirstFileA fails and GetLastError indicates either ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND then for_each_in_folder shall succeed and return 0. ]*/
                    result = 0;
                }
                else
                {
                    /*Codes_SRS_FOR_EACH_IN_FOLDER_02_006: [ If GetLastError indicates any other error then shall fail and return a non-zero value. ]*/
                    LogLastError("failure in FindFirstFileA folder=%s", folder);
                    result = MU_FAILURE;
                }
            }
            else
            {
                bool wasError = false; /*stops calls to FindNext because we failed*/
                bool enumerationShouldContinue; /*stops calls to FindNext because "on_each" says it should stop*/
                bool newFilesAreFound = false; /*stops calls to FindNext because there are no more files*/
                do
                {
                    /*Codes_SRS_FOR_EACH_IN_FOLDER_02_007: [ for_each_in_folder shall call on_each_in_folder passing folder, the discovered WIN32_FIND_DATAA and context. ]*/
                    /*Codes_SRS_FOR_EACH_IN_FOLDER_02_013: [ for_each_in_folder shall call on_each_in_folder passing folder, WIN32_FIND_DATAA discovered by FindNextFileA and context. ]*/
                    if (on_each_in_folder(folder, &findData, context, &enumerationShouldContinue) != 0)
                    {
                        /*Codes_SRS_FOR_EACH_IN_FOLDER_02_008: [ If on_each_in_folder fails then for_each_in_folder shall fail and return a non-zero value. ]*/
                        LogError("failure in on_each_in_folder folder=%s", folder);
                        wasError = true;
                    }
                    else
                    {
                        
                        if (!enumerationShouldContinue)
                        {
                            /*Codes_SRS_FOR_EACH_IN_FOLDER_02_009: [ If on_each_in_folder indicates that the enumeration should stop then for_each_in_folder shall stop further call to FindNextFileA. ]*/
                        }
                        else
                        {
                            /*Codes_SRS_FOR_EACH_IN_FOLDER_02_010: [ for_each_in_folder shall discover the next item in folder by a call to FindNextFileA. ]*/
                            if (FindNextFileA(hFind, &findData) != 0)
                            {
                                newFilesAreFound = true;
                            }
                            else
                            {
                                if (GetLastError() == ERROR_NO_MORE_FILES)
                                {
                                    /*Codes_SRS_FOR_EACH_IN_FOLDER_02_011: [ If FindNextFileA fails and GetLastError returns ERROR_NO_MORE_FILES then for_each_in_folder shall stop further call to FindNextFileA. ]*/
                                    newFilesAreFound = false;
                                }
                                else
                                {
                                    /*Codes_SRS_FOR_EACH_IN_FOLDER_02_012: [ If FindNextFileA fails and GetLastError returns any other value then for_each_in_folder shall fail and return a non-zero value. ]*/
                                    LogLastError("failure in FindNextFileA folder=%s", folder);
                                    wasError = true;
                                }
                            }
                        }
                    }
                } while ((!wasError) && enumerationShouldContinue && newFilesAreFound);

                /*Codes_SRS_FOR_EACH_IN_FOLDER_02_014: [ for_each_in_folder shall call FindClose. ]*/
                if (!FindClose(hFind))
                {
                    LogLastError("failure in FindClose folder=%s", folder);
                }

                /*Codes_SRS_FOR_EACH_IN_FOLDER_02_015: [ for_each_in_folder shall succeed and return 0. ]*/
                result = wasError ? MU_FAILURE : 0;
            }
            free(getEverything);
        }
    }
    
    return result;
}
