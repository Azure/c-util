// Copyright (c) Microsoft. All rights reserved.


#include <stdlib.h>
#include <stdint.h>


#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_bool.h"

#include "c_pal/interlocked.h" /*included for mocking reasons - it will prohibit creation of mocks belonging to interlocked.h - at the moment verified through int tests - this is porting legacy code, temporary solution*/

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "for_each_in_folder.h"

MOCKABLE_FUNCTION(, int, TEST_ON_EACH_IN_FOLDER, const char*, folder, const WIN32_FIND_DATAA*, findData, void*, context, bool*, enumerationShouldContinue)

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"



#include "for_each_in_sub_folder.h"

#define TEST_FOLDER_DEFINE "c:\\folder"
static const char* TEST_FOLDER = TEST_FOLDER_DEFINE;

#define TEST_FOLDER_1_DEFINE "131962010795189575"
static const char* TEST_FOLDER_1 = TEST_FOLDER_1_DEFINE;

#define TEST_CONTEXT_DEFINE (void*)0x42
static void* TEST_CONTEXT = TEST_CONTEXT_DEFINE;

static const bool realTrue = true;
static const bool realFalse = false;

/*below values as "as the debugger caught them on a real run"*/
static const WIN32_FIND_DATAA findDot = 
{

    /*dwFileAttributes*/0x00000010,
    /*ftCreationTime*/
    {
        /*dwLowDateTime*/0xc4f9ac84,
        /*dwHighDateTime*/0x01d4d2bf 
    },
    /*ftLastAccessTime*/
    {
        /*dwLowDateTime*/0xbdbec18b,
        /*dwHighDateTime*/0x01d4d39b
    },
    /*ftLastWriteTime*/
    {
        /*dwLowDateTime*/0xbdbec18b,
        /*dwHighDateTime*/0x01d4d39b
    },
    /*nFileSizeHigh*/0x00000000,
    /*nFileSizeLow*/0x00000000,
    /*dwReserved0*/0x0000007d,
    /*dwReserved1*/0x00000000,
    /*cFileName*/".",
    /*cAlternateFileName*/""
};

static const WIN32_FIND_DATAA findDotDot =
{
    /*dwFileAttributes*/0x00000010,
    /*ftCreationTime*/
    {
    /*dwLowDateTime*/0xc4f9ac84,
    /*dwHighDateTime*/ 0x01d4d2bf
    },
    /*ftLastAccessTime*/
    {
        /*dwLowDateTime*/0xbdbec18b,
        /*dwHighDateTime*/0x01d4d39b
    },
    /*ftLastWriteTime*/
    {
        /*dwLowDateTime*/0xbdbec18b,
        /*dwHighDateTime*/0x01d4d39b
    },
    /*nFileSizeHigh*/0x00000000,
    /*nFileSizeLow*/0x00000000,
    /*dwReserved0*/0x00000000,
    /*dwReserved1*/0x858128b9,
    /*cFileName*/"..",
    /*cAlternateFileName*/""
};

static const WIN32_FIND_DATAA findFile1 =
{
    /*dwFileAttributes*/0x00000020,
    /*ftCreationTime*/
    { 
        /*dwLowDateTime*/0xd11e1292,
        /*dwHighDateTime*/ 0x01d4d2bf 
    },
    /*ftLastAccessTime*/
    { 
        /*dwLowDateTime*/0x9f2eae8a,
        /*dwHighDateTime*/0x01d4d544 
    },
    /*ftLastWriteTime*/
    { 
        /*dwLowDateTime*/0x25483033,
        /*dwHighDateTime*/0x01d4cfab 
    },
    /*nFileSizeHigh*/0x00000000,
    /*nFileSizeLow*/0x00000003,
    /*dwReserved*/0x00000000,
    /*dwReserved1*/0x82f228b9,
    /*cFileName*/"x_subroot_file_1.txt",
    /*cAlternateFileName*/"X_SUBR~1.TXT"
};

static const WIN32_FIND_DATAA findFolder1 =
{
    /*dwFileAttributes*/0x00000010,
    /*ftCreationTime*/
    {
        /*dwLowDateTime*/0xed05cc73,
        /*dwHighDateTime*/0x01d4d2bf
    },
    /*ftLastAccessTime*/
    {
        /*dwLowDateTime*/0xbca8cea8,
        /*dwHighDateTime*/0x01d4d2da
    },
    /*ftLastWriteTime*/
    {
        /*dwLowDateTime*/0xbca8cea8,
        /*dwHighDateTime*/0x01d4d2da
    },
    /*nFileSizeHigh*/0x00000000,
    /*nFileSizeLow*/0x00000000,
    /*dwReserved0*/0x00000000,
    /*dwReserved1*/0x81cc28b9,
    /*cFileName*/TEST_FOLDER_1_DEFINE,
    /*cAlternateFileName*/"131962~1"
};




#define MAX_PRETEND_FIND 5

typedef struct PRETEND_FIND_TAG /*essentially a wrapper around an array of WIN32_FIND_DATAA*/ /*each for_each_in_folder pretends to find what is in "finds"*/
{
    size_t nFinds;
    const WIN32_FIND_DATAA** finds;
}PRETEND_FIND;

typedef struct PRETEND_FINDS_TAG /*feeds multiple of PRETEND_FIND to hook_for_each_in_folder. Each call of hook_for_each_in_folder consumes 1 from pretendFinds*/
{
    size_t nPretendFind;
    const PRETEND_FIND* pretendFinds[MAX_PRETEND_FIND];
}PRETEND_FINDS;

static const WIN32_FIND_DATAA* findJustDotAddresses[] = { &findDot };
static const PRETEND_FIND findJustDot = { sizeof(findJustDotAddresses) / sizeof(findJustDotAddresses[0]), findJustDotAddresses };

static const WIN32_FIND_DATAA* findJustDotDotAddresses[] = { &findDotDot };
static const PRETEND_FIND findJustDotDot = { sizeof(findJustDotDotAddresses) / sizeof(findJustDotDotAddresses[0]), findJustDotDotAddresses };

static const WIN32_FIND_DATAA* findJustFileAddresses[] = { &findFile1};
static const PRETEND_FIND findJustFile = { sizeof(findJustFileAddresses) / sizeof(findJustFileAddresses[0]), findJustFileAddresses };

static const WIN32_FIND_DATAA* findJustFolder1Addresses[] = { &findFolder1};
static const PRETEND_FIND findJustFolder1 = { sizeof(findJustFolder1Addresses) / sizeof(findJustFolder1Addresses[0]), findJustFolder1Addresses };

static const PRETEND_FINDS findsDot = 
{

    1,
    {&findJustDot}
};

static const PRETEND_FINDS findsDotDot =
{
    1,
    {&findJustDotDot}
};

static const PRETEND_FINDS findsFile =
{
    1,
    {&findJustFile}
};

static const PRETEND_FINDS findsFolder1File1=
{
    2,
    {&findJustFolder1, &findJustFile}
};

static const PRETEND_FINDS* g_pretendFinds = NULL;

static size_t hook_for_each_in_folder_current_call = 0;
static int hook_for_each_in_folder(const char* folder, ON_EACH_IN_FOLDER on_each_in_folder, void* context)
{
    int result;
    /*this function "pretends" to find several folders, based on where g_pretendFinds points to*/
    if (g_pretendFinds == NULL)
    {
        result = 0;
    }
    else
    {
        hook_for_each_in_folder_current_call++; /*because this function is called recursively, it needs to be increments "before". Otherwise it stays at zero.*/

        ASSERT_IS_TRUE(hook_for_each_in_folder_current_call-1 < g_pretendFinds->nPretendFind);
        
        size_t i;
        for(i = 0; i < g_pretendFinds->pretendFinds[hook_for_each_in_folder_current_call-1]->nFinds; i++)
        {
            bool ignored;
            if (on_each_in_folder(folder, g_pretendFinds->pretendFinds[hook_for_each_in_folder_current_call - 1]->finds[i], context, &ignored) != 0)
            {
                break;
            }
        }

        if (i == g_pretendFinds->pretendFinds[hook_for_each_in_folder_current_call - 1]->nFinds)
        {
            result = 0;
        }
        else
        {
            result = MU_FAILURE;
        }
        
    }
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

    REGISTER_GLOBAL_MOCK_HOOK(for_each_in_folder, hook_for_each_in_folder);
    
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_UMOCK_ALIAS_TYPE(LPCSTR, const char*);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, uint32_t);
    REGISTER_UMOCK_ALIAS_TYPE(LPSECURITY_ATTRIBUTES, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PLARGE_INTEGER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PULARGE_INTEGER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(LPWIN32_FIND_DATAA, void*);
    REGISTER_UMOCK_ALIAS_TYPE(ON_EACH_IN_FOLDER, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BOOL, int);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    g_pretendFinds = NULL;
    hook_for_each_in_folder_current_call = 0;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_001: [ If folder is NULL then for_each_in_sub_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_folder_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = for_each_in_sub_folder(NULL, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_002: [ If on_each_in_folder is NULL then for_each_in_sub_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_on_each_in_folder_NULL_fails)
{
    ///arrange
    int result;

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, NULL, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_003: [ for_each_in_sub_folder shall construct a new context that contains on_each_in_folder and context. ]*/
/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_004: [ for_each_in_sub_folder shall call for_each_in_folder with folder set to the same folder, on_each_in_folder set to on_subfolder_folder and context set the previous constructed context. ]*/
/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_005: [ for_each_in_sub_folder shall succeed and return 0. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_0_subfolders_succeeds)
{
    ///arrange
    int result;
    g_pretendFinds = NULL;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG));

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_006: [ If findData does not have FILE_ATTRIBUTE_DIRECTORY flag set then on_subfolder_folder shall set enumerationShouldContinue to true, succeed, and return 0. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_a_file_succeeds)
{
    ///arrange
    int result;
    g_pretendFinds = &findsFile;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG));

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_007: [ If findData->cFileName is "." then on_subfolder_folder shall set enumerationShouldContinue to true, succeed, and return 0. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_dot_succeeds)
{
    ///arrange
    int result;
    g_pretendFinds = &findsDot;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG));

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_013: [ If findData->cFileName is ".." then on_subfolder_folder shall set enumerationShouldContinue to true, succeed, and return 0. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_dotdot_succeeds)
{
    ///arrange
    int result;
    g_pretendFinds = &findsDotDot;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG));

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_008: [ on_subfolder_folder shall assemble a string folder\findData->cFileName. ]*/
/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_009: [ on_subfolder_folder shall call for_each_in_folder with folder set to the previous string, on_each_in_folder set to the original on_each_in_folder passed to for_each_in_sub_folder and context set to the original context passed to for_each_in_sub_folder. ]*/
/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_011: [ on_subfolder_folder shall set enumerationShouldContinue to true, succeed and return 0. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_1_subfolder_succeeds)
{
    ///arrange
    int result;
    g_pretendFinds = &findsFolder1File1;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER_DEFINE "\\" TEST_FOLDER_1_DEFINE, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT));
    STRICT_EXPECTED_CALL(TEST_ON_EACH_IN_FOLDER(TEST_FOLDER_DEFINE "\\" TEST_FOLDER_1_DEFINE, &findFile1, TEST_CONTEXT, IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_012: [ If there are any failure then for_each_in_sub_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_sub_folder_fails_when_for_each_in_folder_fails)
{
    ///arrange
    int result;
    g_pretendFinds = &findsFile;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG))
        .SetReturn(MU_FAILURE);

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_010: [ If there are any failures then on_subfolder_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_1_subfolder_fails_when_for_each_in_folder_fails)
{
    ///arrange
    int result;
    g_pretendFinds = &findsFolder1File1;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER_DEFINE "\\" TEST_FOLDER_1_DEFINE, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT))
        .SetReturn(MU_FAILURE);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_FOR_EACH_IN_SUBFOLDER_02_010: [ If there are any failures then on_subfolder_folder shall fail and return a non-zero value. ]*/
TEST_FUNCTION(for_each_in_sub_folder_with_1_subfolder_fails_when_malloc_fails)
{
    ///arrange
    int result;
    g_pretendFinds = &findsFolder1File1;

    STRICT_EXPECTED_CALL(for_each_in_folder(TEST_FOLDER, IGNORED_ARG, IGNORED_ARG));

    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    result = for_each_in_sub_folder(TEST_FOLDER, TEST_ON_EACH_IN_FOLDER, TEST_CONTEXT);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
