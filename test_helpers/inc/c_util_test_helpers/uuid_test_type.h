// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef UUID_TEST_TYPE_H
#define UUID_TEST_TYPE_H

#include "ctest.h"

#include "c_util/uuid.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef UUID_T* uuid_ptr;

    void uuid_ptr_ToString(char* string, size_t bufferSize, const UUID_T* val);
    int uuid_ptr_Compare(const UUID_T* left, const UUID_T* right);

    void UUID_T_ToString(char* string, size_t bufferSize, const UUID_T val);
    int UUID_T_Compare(const UUID_T left, const UUID_T right);

    extern int umocktypes_uuid_register_types(void);

    extern char* umocktypes_stringify_uuid(const UUID_T** value);
    extern int umocktypes_are_equal_uuid(const UUID_T** left, const UUID_T** right);
    extern int umocktypes_copy_uuid(UUID_T** destination, const UUID_T** source);
    extern void umocktypes_free_uuid(UUID_T** value);

    CTEST_DECLARE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(uuid_ptr);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifdef CPP_UNITTEST
#include "CppUnitTestAssert.h"
#include <exception>

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework {
    template<> std::wstring ToString<UUID_T>(const UUID_T& t)
    {
        wchar_t temp[sizeof(UUID_T) * 2 + 4 + 1];
        if (swprintf_s(temp, sizeof(temp)/sizeof(temp[0]), L"%" PRI_UUID, UUID_FORMAT_VALUES(t)) != (sizeof(temp)/sizeof(temp[0]) - 1))
        {
            // Only happens if the string format has changed
            throw std::exception("swprintf_s failed");
        }
        return std::wstring(temp);
    }

    template<> std::wstring ToString<UUID_T>(const UUID_T* t)
    {
        if (nullptr == t)
        {
            return std::wstring(L"(NULL)");
        }
        return ToString(*t);
    }

    template<> std::wstring ToString<UUID_T>(UUID_T* t)
    {
        if (nullptr == t)
        {
            return std::wstring(L"(NULL)");
        }
        return ToString<UUID_T>(*t);
    }

    template<> std::wstring ToString<uuid_ptr>(const uuid_ptr& t)
    {
        return ToString<UUID_T>(*t);
    }

    template<> std::wstring ToString<uuid_ptr>(const uuid_ptr* t)
    {
        return ToString<UUID_T>(**t);
    }

    template<> std::wstring ToString<uuid_ptr>(uuid_ptr* t)
    {
        return ToString<UUID_T>(**t);
    }

    template<> void Assert::AreEqual<uuid_ptr>(const uuid_ptr& expected, const uuid_ptr& actual, const wchar_t* message, const __LineInfo* pLineInfo)
    {
        FailOnCondition((uuid_ptr_Compare(expected, actual) == 0), EQUALS_MESSAGE(expected, actual, message), pLineInfo);
    }
}}}
#endif
#endif

#endif /* UUID_TEST_TYPE_H */
