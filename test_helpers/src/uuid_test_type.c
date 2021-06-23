// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ctest.h"

#include "c_util/uuid.h"              // for UUID_T, PRI_UUID, UUID_FORMAT_V...

//#include "c_pal/gballoc_hl.h" hmmmm... umock_c seems to free some memory allocated here. So this cannot call into gballoc thingies. This is fine, as this file should be part of umock.c anyway (and nothing there calls gballoc...)
//#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umocktypes.h"

#include "umock_c/umock_log.h"

#include "c_util_test_helpers/uuid_test_type.h"

void uuid_ptr_ToString(char* string, size_t bufferSize, const UUID_T* val)
{
    (void)sprintf_s(string, bufferSize, "%" PRI_UUID, UUID_FORMAT_VALUES(*val));
}

int uuid_ptr_Compare(const UUID_T* left, const UUID_T* right)
{
    return memcmp(*left, *right, sizeof(UUID_T));
}

void UUID_T_ToString(char* string, size_t bufferSize, const UUID_T val)
{
    (void)sprintf_s(string, bufferSize, "%" PRI_UUID, UUID_FORMAT_VALUES(val));
}

int UUID_T_Compare(const UUID_T left, const UUID_T right)
{
    return memcmp(left, right, sizeof(UUID_T));
}

char* umocktypes_stringify_uuid(const UUID_T** value)
{
    char* result;

    if (value == NULL)
    {
        UMOCK_LOG("umocktypes_stringify_uuid: NULL value.");
        result = NULL;
    }
    else
    {
        if (*value == NULL)
        {
            result = malloc(sizeof("NULL"));
            if (result != NULL)
            {
                (void)memcpy(result, "NULL", sizeof("NULL"));
            }
        }
        else
        {
            // 2 characters per byte plus 4 dashes
            size_t length = sizeof(UUID_T) * 2 + 4;
            result = malloc(length + 1);
            if (result == NULL)
            {
                UMOCK_LOG("umocktypes_stringify_uuid: Cannot allocate memory for result.");
            }
            else
            {
                if (sprintf_s(result, length + 1, "%" PRI_UUID, UUID_FORMAT_VALUES(**value)) != (int)length)
                {
                    UMOCK_LOG("umocktypes_stringify_uuid: failed to format UUID.");
                    free(result);
                }
            }
        }
    }

    return result;
}

int umocktypes_are_equal_uuid(const UUID_T** left, const UUID_T** right)
{
    int result;

    if ((left == NULL) || (right == NULL))
    {
        UMOCK_LOG("umocktypes_are_equal_uuid: Bad arguments:left = %p, right = %p.", left, right);
        result = -1;
    }
    else if ((*left == NULL) || (*right == NULL))
    {
        if (*left == *right)
        {
            result = 1;
        }
        else
        {
            result = 0;
        }
    }
    else if (**left == **right)
    {
        result = 1;
    }
    else if ((**left == NULL) || (**right == NULL))
    {
        result = 0;
    }
    else
    {
        result = (memcmp(**left, **right, sizeof(UUID_T)) == 0) ? 1 : 0;
    }

    return result;
}

int umocktypes_copy_uuid(UUID_T** destination, const UUID_T** source)
{
    int result;

    if ((destination == NULL) || (source == NULL))
    {
        UMOCK_LOG("umocktypes_copy_uuid: Bad arguments: destination = %p, source = %p.",
            destination, source);
        result = MU_FAILURE;
    }
    else
    {
        if (*source == NULL)
        {
            *destination = NULL;
            result = 0;
        }
        else
        {
            *destination = malloc(sizeof(UUID_T));
            (void)memcpy(**destination, **source, sizeof(UUID_T));
            result = 0;
        }
    }

    return result;
}

void umocktypes_free_uuid(UUID_T** value)
{
    if (value)
    {
        free(*value);
    }
}

int umocktypes_uuid_register_types()
{
    int result;

    if ((REGISTER_TYPE(const UUID_T, uuid) != 0))
    {
        UMOCK_LOG("umocktypes_uuid_register_types: Cannot register types.");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

CTEST_DEFINE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(uuid_ptr,);