// Copyright (C) Microsoft Corporation. All rights reserved.

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/string_utils.h"

#include "for_each_in_sub_folder.h"

typedef struct FOR_EACH_IN_SUBFOLDER_TAG
{
    void* user_context;
    ON_EACH_IN_FOLDER user_on_each_in_folder;
}FOR_EACH_IN_SUBFOLDER;

static int on_subfolder_folder(const char* folder, const WIN32_FIND_DATAA* findData, void* context, bool* enumerationShouldContinue)
{
    int result;
    
    /*see if findData is about a folder*/
    /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_006: [ If findData does not have FILE_ATTRIBUTE_DIRECTORY flag set then on_subfolder_folder shall set enumerationShouldContinue to true, succeed, and return 0. ]*/
    if (!(findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        result = 0;
        *enumerationShouldContinue = true;
    }
    else
    {
        if (
            /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_007: [ If findData->cFileName is "." then on_subfolder_folder shall set enumerationShouldContinue to true, succeed, and return 0. ]*/
            (strcmp(findData->cFileName, ".") == 0) ||
            /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_013: [ If findData->cFileName is ".." then on_subfolder_folder shall set enumerationShouldContinue to true, succeed, and return 0. ]*/
            (strcmp(findData->cFileName, "..") == 0)
            )
        {
            result = 0;
            *enumerationShouldContinue = true;
        }
        else
        {
            /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_008: [ on_subfolder_folder shall assemble a string folder\findData->cFileName. ]*/
            char* subfolder = sprintf_char("%s\\%s", folder, findData->cFileName);
            if (subfolder == NULL)
            {
                /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_010: [ If there are any failures then on_subfolder_folder shall fail and return a non-zero value. ]*/
                LogError("failure in sprintf_char");
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_009: [ on_subfolder_folder shall call for_each_in_folder with folder set to the previous string, on_each_in_folder set to the original on_each_in_folder passed to for_each_in_sub_folder and context set to the original context passed to for_each_in_sub_folder. ]*/
                FOR_EACH_IN_SUBFOLDER* forEachInSubfolder = context;
                if (for_each_in_folder(subfolder, forEachInSubfolder->user_on_each_in_folder, forEachInSubfolder->user_context) != 0)
                {
                    /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_010: [ If there are any failures then on_subfolder_folder shall fail and return a non-zero value. ]*/
                    LogError("failure in for_each_in_folder folder=%s, subfolder=%s", folder, subfolder);
                    result = MU_FAILURE;
                }
                else
                {
                    /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_011: [ on_subfolder_folder shall set enumerationShouldContinue to true, succeed and return 0. ]*/
                    result = 0;
                    *enumerationShouldContinue = true;
                }
                free(subfolder);
            }
        }
    }
    
    return result;
}

int for_each_in_sub_folder(const char* folder, ON_EACH_IN_FOLDER on_each_in_folder, void* context)
{
    int result;
    if (
        /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_001: [ If folder is NULL then for_each_in_sub_folder shall fail and return a non-zero value. ]*/
        (folder == NULL) ||
        /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_002: [ If on_each_in_folder is NULL then for_each_in_sub_folder shall fail and return a non-zero value. ]*/
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
        /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_003: [ for_each_in_sub_folder shall construct a new context that contains on_each_in_folder and context. ]*/
        FOR_EACH_IN_SUBFOLDER forEachInSubfolder;
        forEachInSubfolder.user_context = context;
        forEachInSubfolder.user_on_each_in_folder = on_each_in_folder;
        /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_004: [ for_each_in_sub_folder shall call for_each_in_folder with folder set to the same folder, on_each_in_folder set to on_subfolder_folder and context set the previous constructed context. ]*/
        if (for_each_in_folder(folder, on_subfolder_folder, &forEachInSubfolder) != 0)
        {
            /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_012: [ If there are any failure then for_each_in_sub_folder shall fail and return a non-zero value. ]*/
            LogError("failure in for_each_in_folder folder=%s", folder);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_FOR_EACH_IN_SUBFOLDER_02_005: [ for_each_in_sub_folder shall succeed and return 0. ]*/
            result = 0;
        }
    }
    return result;
}
