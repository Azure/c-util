// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef RC_STRING_TEST_TYPE_H
#define RC_STRING_TEST_TYPE_H

#include "c_util/rc_string.h"
#include "c_pal/thandle.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CTEST_THANDLE_NO_CONST(T) MU_C2(NONCONST_P2_CONST_,T)

typedef const RC_STRING* CTEST_THANDLE_NO_CONST(RC_STRING);

typedef CTEST_THANDLE_NO_CONST(RC_STRING) TEST_THANDLE_RC_STRING;

void TEST_THANDLE_RC_STRING_ToString(char* string, size_t bufferSize, THANDLE(RC_STRING) val);
int TEST_THANDLE_RC_STRING_Compare(THANDLE(RC_STRING) left, THANDLE(RC_STRING) right);

int umocktypes_THANDLE_RC_STRING_register_types(void);

char* umocktypes_stringify_THANDLE_RC_STRING(THANDLE(RC_STRING)* value);
int umocktypes_are_equal_THANDLE_RC_STRING(THANDLE(RC_STRING)* left, THANDLE(RC_STRING)* right);
int umocktypes_copy_THANDLE_RC_STRING(THANDLE(RC_STRING)* destination, THANDLE(RC_STRING)* source);
void umocktypes_free_THANDLE_RC_STRING(THANDLE(RC_STRING)* value);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifdef CPP_UNITTEST
#include "CppUnitTestAssert.h"
#include <exception>
#include <sstream>

typedef const RC_STRING* RC_STRING_PTR;

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
    template<> std::wstring ToString<RC_STRING>(const RC_STRING* t)
    {
        if (nullptr == t)
        {
            return std::wstring(L"(NULL)");
        }
        RETURN_WIDE_STRING((t->string));
    }

    template<> std::wstring ToString<RC_STRING>(const RC_STRING& t)
    {
        RETURN_WIDE_STRING((t.string));
    }

    template<> void Assert::AreEqual<RC_STRING_PTR>(const RC_STRING_PTR& expected, const RC_STRING_PTR& actual, const wchar_t* message, const __LineInfo* pLineInfo)
    {
        FailOnCondition((TEST_THANDLE_RC_STRING_Compare(expected, actual) == 0), EQUALS_MESSAGE(expected, actual, message), pLineInfo);
    }
}}}

#endif
#endif

#endif /* RC_STRING_TEST_TYPE_H */
