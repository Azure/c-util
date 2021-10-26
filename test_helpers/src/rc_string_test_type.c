// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "macro_utils/macro_utils.h"

#include "c_util/rc_string.h"
#include "c_util/thandle.h"

//#include "c_pal/gballoc_hl.h" hmmmm... umock_c seems to free some memory allocated here. So this cannot call into gballoc thingies. This is fine, as this file should be part of umock.c anyway (and nothing there calls gballoc...)
//#include "c_pal/gballoc_hl_redirect.h"

#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_charptr.h"

#include "umock_c/umock_log.h"

#include "real_rc_string.h"

#include "c_util_test_helpers/rc_string_test_type.h"

void TEST_THANDLE_RC_STRING_ToString(char* string, size_t bufferSize, THANDLE(RC_STRING) val)
{
    (void)snprintf(string, bufferSize, "%" PRI_RC_STRING, RC_STRING_VALUE(val));
}

int TEST_THANDLE_RC_STRING_Compare(THANDLE(RC_STRING) left, THANDLE(RC_STRING) right)
{
    if ((left == NULL) &&
        (right == NULL))
    {
        return 0;
    }
    else if (left == NULL)
    {
        return -1;
    }
    else if (right == NULL)
    {
        return 1;
    }
    else
    {
        if ((left->string == NULL) &&
            (right->string == NULL))
        {
            return 0;
        }
        else if (left->string == NULL)
        {
            return -1;
        }
        else if (right->string == NULL)
        {
            return 1;
        }
        else
        {
            return strcmp(left->string, right->string);
        }
    }
}

int umocktypes_THANDLE_RC_STRING_register_types(void)
{
    int result;

    if ((REGISTER_TYPE(THANDLE(RC_STRING), THANDLE_RC_STRING) != 0))
    {
        UMOCK_LOG("umocktypes_THANDLE_RC_STRING_register_types: Cannot register types.");
        result = MU_FAILURE;
    }
    else
    {
        result = 0;
    }

    return result;
}

char* umocktypes_stringify_THANDLE_RC_STRING(THANDLE(RC_STRING)* value)
{
    char* result;

    if (value == NULL)
    {
        UMOCK_LOG("umocktypes_stringify_THANDLE_RC_STRING: NULL value.");
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
            return umocktypes_stringify_const_charptr((const char**)&(*value)->string);
        }
    }

    return result;
}

int umocktypes_are_equal_THANDLE_RC_STRING(THANDLE(RC_STRING)* left, THANDLE(RC_STRING)* right)
{
    int result;

    if ((left == NULL) || (right == NULL))
    {
        UMOCK_LOG("umocktypes_are_equal_THANDLE_RC_STRING: Bad arguments:left = %p, right = %p.", left, right);
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
    else if (((*left)->string == NULL) || ((*right)->string == NULL))
    {
        result = 0;
    }
    else
    {
        result = umocktypes_are_equal_const_charptr((const char**)&(*left)->string, (const char**)&(*right)->string);
    }

    return result;
}

int umocktypes_copy_THANDLE_RC_STRING(THANDLE(RC_STRING)* destination, THANDLE(RC_STRING)* source)
{
    int result;

    if ((destination == NULL) || (source == NULL))
    {
        UMOCK_LOG("umocktypes_copy_THANDLE_RC_STRING: Bad arguments: destination = %p, source = %p.",
            destination, source);
        result = MU_FAILURE;
    }
    else
    {
        THANDLE_INITIALIZE(real_RC_STRING)(destination, *source);
        result = 0;
    }

    return result;
}

void umocktypes_free_THANDLE_RC_STRING(THANDLE(RC_STRING)* value)
{
    if (value != NULL && *value != NULL)
    {
        THANDLE_ASSIGN(real_RC_STRING)(value, NULL);
    }
}