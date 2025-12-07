// Copyright (C) Microsoft Corporation. All rights reserved.

#include <string.h>                   // for NULL, strlen, size_t
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"  // for MU_FAILURE, MU_P_OR_NULL

#include "c_pal/uuid.h"

#include "real_gballoc_hl.h"
#include "real_uuid.h"
#include "real_uuid_string.h"

#include "c_logging/logger.h"

#include "c_util_test_helpers/guid_as_string.h"

const char* getGuidAsString(void)
{
    UUID_T u;
    char* result;
    if (real_uuid_produce(&u) != 0)
    {
        LogError("real_uuid_produce failed");
        result = NULL;
    }
    else
    {
        result = real_uuid_to_string(u);

        if(result == NULL)
        {
            LogError("real_uuid_to_string failed");
            result = NULL;
            /*return as is*/
        }
    }
    return result;
}

void freeGuidAsString(const char* s)
{
    real_gballoc_hl_free((void*)s);
}

int getGuidFromString(const char* guidAsString, GUID* guid)
{
    int result;
    if (
        (guidAsString == NULL) ||
        (guid == NULL)
        )
    {
        LogError("invalid argument const char* guidAsString=%s, GUID* guid=%p", MU_P_OR_NULL(guidAsString), guid);
        result = MU_FAILURE;
    }
    else
    {
        size_t guidAsStringLength = strlen(guidAsString);
        if (guidAsStringLength != 36)
        {
            LogError("const char* guidAsString=%s has length=%zu which is not expected for a guid. Guids need 36 characters.", guidAsString, guidAsStringLength);
            result = MU_FAILURE;
        }
        else
        {
            /*parse it as UUID, convert UUID to GUID*/
            UUID_T u;
            if (real_uuid_from_string(guidAsString, &u) != UUID_FROM_STRING_RESULT_OK)
            {
                LogError("failure in real_uuid_from_string");
                result = MU_FAILURE;
            }
            else
            {
                if(GUID_from_uuid(guid, u)!=0)
                {
                    LogError("failure in GUID_from_uuid(guid=%p, u=%" PRI_UUID_T ")", guid, UUID_T_VALUES(u));
                    result = MU_FAILURE;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}

