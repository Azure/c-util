// Copyright (C) Microsoft Corporation. All rights reserved.

#include <string.h>

#include "macro_utils/macro_utils.h"  // for MU_P_OR_NULL

#include "c_logging/logger.h"

#include "c_pal/string_utils.h"

#include "c_util/bs_filename_helper.h"

char* bs_filename_append_suffix(const char* filename, const char* suffix)
{
    char* result;
    if (
        /*Codes_SRS_BS_FILENAME_HELPER_42_001: [ If filename is NULL then bs_filename_append_suffix shall fail and return NULL. ]*/
        filename == NULL ||
        /*Codes_SRS_BS_FILENAME_HELPER_42_002: [ If suffix is NULL then bs_filename_append_suffix shall fail and return NULL. ]*/
        suffix == NULL
        )
    {
        LogError("invalid arguments const char* filename=%s, const char* suffix=%s",
            MU_P_OR_NULL(filename), MU_P_OR_NULL(suffix));
        result = NULL;
    }
    else
    {
        const char* last_dot_character_location = strrchr(filename, '.');
        const char* last_backslash_character_location = strrchr(filename, '\\');

        /*Codes_SRS_BS_FILENAME_HELPER_42_003: [ If filename does not contain a . then bs_filename_append_suffix shall return filename + suffix. ]*/
        /*Codes_SRS_BS_FILENAME_HELPER_42_005: [ If filename contains last . before last \ then bs_filename_append_suffix shall return filename + suffix. ]*/
        if ((last_dot_character_location == NULL) || ((last_backslash_character_location != NULL) && (last_dot_character_location < last_backslash_character_location)))
        {
            result = sprintf_char("%s%s", filename, suffix);
            if (result == NULL)
            {
                /*Codes_SRS_BS_FILENAME_HELPER_42_007: [ If there are any failures then bs_filename_append_suffix shall return NULL. ]*/
                LogError("failure in sprintf_char(\"%%s%%s\", filename=%s, suffix=%s",
                    filename, suffix);
                /*return as is*/
            }
            else
            {
                /*return as is*/
            }
        }
        else
        {
            /*Codes_SRS_BS_FILENAME_HELPER_42_004: [ If filename does not contain a \ then bs_filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
            /*Codes_SRS_BS_FILENAME_HELPER_42_006: [ If filename contains last . after last \ then bs_filename_append_suffix shall return the name of the file + suffix + . + extension. ]*/
            result = sprintf_char("%.*s%s%s", (int)(last_dot_character_location - filename), filename, suffix, last_dot_character_location);
            if (result == NULL)
            {
                /*Codes_SRS_BS_FILENAME_HELPER_42_007: [ If there are any failures then bs_filename_append_suffix shall return NULL. ]*/
                LogError("failure in sprintf_char(\"%%.*s%%s%%s\", (int)(last_dot_character_location - filename)=%d, filename=%s, suffix=%s, dot_character=%s",
                    (int)(last_dot_character_location - filename), filename, suffix, last_dot_character_location);
                /*return as is*/
            }
            else
            {
                /*return as is*/
            }
        }
    }
    return result;
}
