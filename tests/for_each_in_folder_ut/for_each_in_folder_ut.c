// Copyright (c) Microsoft. All rights reserved.



#include "for_each_in_folder_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
    MOCKABLE_FUNCTION(, DWORD, mocked_GetLastError);

    MOCKABLE_FUNCTION(, HANDLE, mocked_FindFirstFileA,
        LPCSTR             ,lpFileName,
        LPWIN32_FIND_DATAA ,lpFindFileData
    );

    MOCKABLE_FUNCTION(, BOOL, mocked_FindNextFileA,
        HANDLE             ,hFindFile,
        LPWIN32_FIND_DATAA ,lpFindFileData
    );

    MOCKABLE_FUNCTION(, BOOL, mocked_FindClose, 
        HANDLE, hFindFile
    );

    MOCKABLE_FUNCTION(, int, TEST_ON_EACH_IN_FOLDER, const char*, folder, const WIN32_FIND_DATAA*, findData, void*, context, bool*, enumerationShouldContinue)
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

static HANDLE hook_FindFirstFileA(
    LPCSTR lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
)
{
    (void)lpFileName;
    (void)lpFindFileData;
    return (HANDLE)real_gballoc_hl_malloc(1);
}

static BOOL hook_FindClose(
    HANDLE hFindFile
)
{
    ASSERT_IS_NOT_NULL(hFindFile);
    real_gballoc_hl_free(hFindFile);
    return TRUE;
}

#define TEST_FOLDER_DEFINE "c:\folder"
static const char* TEST_FOLDER = TEST_FOLDER_DEFINE;

#define TEST_CONTEXT_DEFINE (void*)0x42
static void* TEST_CONTEXT = TEST_CONTEXT_DEFINE;

static const bool realTrue = true;
static const bool realFalse = false;

/*following function cannot be mocked because of variable number of arguments:( so it is copy&pasted here*/
char* sprintf_char_function(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_char(format, va);
    va_end(va);
    return result;
}

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types(), "umocktypes_charptr_register_types");

    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types(), "umocktypes_bool_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(mocked_FindFirstFileA, hook_FindFirstFileA);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_FindClose, hook_FindClose);

    REGISTER_UMOCK_ALIAS_TYPE(LPCSTR, const char*);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, uint32_t);
    REGISTER_UMOCK_ALIAS_TYPE(LPSECURITY_ATTRIBUTES, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PLARGE_INTEGER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PULARGE_INTEGER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWIN32_FIND_DATAA, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, int);
    REGISTER_UMOCK_ALIAS_TYPE(va_list, void*);

    REGISTER_GLOBAL_MOCK_RETURNS(mocked_GetLastError, ERROR_SUCCESS, ERROR_ACCESS_DENIED);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_001: [ If folder is NULL then for_each_in_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_folder_with_folder_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = for_each_in_folder(NULL, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_002: [ If on_each_in_folder is NULL then for_each_in_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_folder_with_on_each_in_folder_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = for_each_in_folder(TEST_FOLDER, NULL, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_003: [ for_each_in_folder shall assemble the string folder\\* to enumerate all constituents in folder. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_004: [ for_each_in_folder shall call FindFirstFileA with lpFileName set to the previously build string. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_005: [ If FindFirstFileA fails and GetLastError indicates either ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND then for_each_in_folder shall succeed and return 0. ]*/
TEST_FUNCTION(for_each_in_folder_with_with_FindFirstFileA_returning_ERROR_FILE_NOT_FOUND_succeeds)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG))
        .SetReturn(INVALID_HANDLE_VALUE);
    STRICT_EXPECTED_CALL(mocked_GetLastError())
        .SetReturn(ERROR_FILE_NOT_FOUND);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_003: [ for_each_in_folder shall assemble the string folder\\* to enumerate all constituents in folder. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_004: [ for_each_in_folder shall call FindFirstFileA with lpFileName set to the previously build string. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_005: [ If FindFirstFileA fails and GetLastError indicates either ERROR_FILE_NOT_FOUND or ERROR_PATH_NOT_FOUND then for_each_in_folder shall succeed and return 0. ]*/
TEST_FUNCTION(for_each_in_folder_with_with_FindFirstFileA_returning_ERROR_PATH_NOT_FOUND_succeeds)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG))
        .SetReturn(INVALID_HANDLE_VALUE);
    STRICT_EXPECTED_CALL(mocked_GetLastError())
        .SetReturn(ERROR_PATH_NOT_FOUND);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_016: [ If there are any failures then for_each_in_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_folder_when_malloc_fails_it_fails)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_006: [ If GetLastError indicates any other error then shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_folder_with_with_FindFirstFileA_returning_ERROR_ACCESS_DENIED_fails)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG))
        .SetReturn(INVALID_HANDLE_VALUE);
    STRICT_EXPECTED_CALL(mocked_GetLastError())
        .SetReturn(ERROR_ACCESS_DENIED);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_007: [ for_each_in_folder shall call on_each_in_folder passing folder, the discovered WIN32_FIND_DATAA and context. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_010: [ for_each_in_folder shall discover the next item in folder by a call to FindNextFileA. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_011: [ If FindNextFileA fails and GetLastError returns ERROR_NO_MORE_FILES then for_each_in_folder shall stop further call to FindNextFileA. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_013: [ for_each_in_folder shall call on_each_in_folder passing folder, WIN32_FIND_DATAA discovered by FindNextFileA and context. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_014: [ for_each_in_folder shall call FindClose. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_015: [ for_each_in_folder shall succeed and return 0. ]*/
TEST_FUNCTION(for_each_in_folder_with_with_1_file_succeeds)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindNextFileA(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_GetLastError())
        .SetReturn(ERROR_NO_MORE_FILES);
    STRICT_EXPECTED_CALL(mocked_FindClose(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_007: [ for_each_in_folder shall call on_each_in_folder passing folder, the discovered WIN32_FIND_DATAA and context. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_010: [ for_each_in_folder shall discover the next item in folder by a call to FindNextFileA. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_011: [ If FindNextFileA fails and GetLastError returns ERROR_NO_MORE_FILES then for_each_in_folder shall stop further call to FindNextFileA. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_013: [ for_each_in_folder shall call on_each_in_folder passing folder, WIN32_FIND_DATAA discovered by FindNextFileA and context. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_014: [ for_each_in_folder shall call FindClose. ]*/
/*Tests_SRS_FOR_EACH_IN_FOLDER_02_015: [ for_each_in_folder shall succeed and return 0. ]*/
TEST_FUNCTION(for_each_in_folder_with_with_2_files_succeeds)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindNextFileA(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE);
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindNextFileA(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_GetLastError())
        .SetReturn(ERROR_NO_MORE_FILES);
    STRICT_EXPECTED_CALL(mocked_FindClose(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_008: [ If on_each_in_folder fails then for_each_in_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_folder_when_on_each_in_folder_fails_for_second_file_it_fails)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindNextFileA(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE);
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(mocked_FindClose(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_008: [ If on_each_in_folder fails then for_each_in_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_folder_when_on_each_in_folder_fails_for_first_file_it_fails)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(mocked_FindClose(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_009: [ If on_each_in_folder indicates that the enumeration should stop then for_each_in_folder shall stop further call to FindNextFileA. ]*/
TEST_FUNCTION(for_each_in_folder_when_first_on_each_in_folder_returns_enumeration_should_stop_it_succeeds)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realFalse, sizeof(realFalse))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindClose(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_009: [ If on_each_in_folder indicates that the enumeration should stop then for_each_in_folder shall stop further call to FindNextFileA. ]*/
TEST_FUNCTION(for_each_in_folder_when_second_on_each_in_folder_returns_enumeration_should_stop_it_succeeds)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindNextFileA(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE);
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realFalse, sizeof(realFalse))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindClose(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_FOR_EACH_IN_FOLDER_02_012: [ If FindNextFileA fails and GetLastError returns any other value then for_each_in_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_folder_with_with_2_when_FindNextFileA_fails_it_fails)
{
    ///arrange
    int result;

    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_FindFirstFileA(TEST_FOLDER_DEFINE "\\*", IGNORED_ARG));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindNextFileA(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE);
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER, IGNORED_ARG, TEST_CONTEXT, IGNORED_ARG))
        .CopyOutArgumentBuffer_enumerationShouldContinue(&realTrue, sizeof(realTrue))
        .SetReturn(0);
    STRICT_EXPECTED_CALL(mocked_FindNextFileA(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_GetLastError())
        .SetReturn(ERROR_ACCESS_DENIED);
    STRICT_EXPECTED_CALL(mocked_FindClose(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}
END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
