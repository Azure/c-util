// Copyright (C) Microsoft Corporation. All rights reserved.

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_util/trc_ptr.h"

typedef struct STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_TAG
{
    void_ptr ptr;
} STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC;

typedef STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC* STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE;
static STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE custom_create_func_for_struct_that_has_its_own_free_func()
{
    STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE result = malloc(sizeof(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC));
    ASSERT_IS_NOT_NULL(result);
    result->ptr = malloc(1);
    ASSERT_IS_NOT_NULL(result->ptr);
    return result;
}

static bool custom_free_function_is_called = false;
static void custom_free_func_for_struct_that_has_its_own_free_func(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE ptr)
{
    custom_free_function_is_called = true;
    free(ptr->ptr);
    free(ptr);
}

TRC_PTR_DECLARE(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE);
TRC_PTR_DEFINE(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE);

typedef int* INTPTR;
TRC_PTR_DECLARE(INTPTR);
TRC_PTR_DEFINE(INTPTR);

static TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) setup_create()
{
    STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE x = custom_create_func_for_struct_that_has_its_own_free_func();
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(x, custom_free_func_for_struct_that_has_its_own_free_func);
    return rc_ptr;
}

typedef struct STRUCT_WITH_TRPC_PTR_TAG
{
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr;
} STRUCT_WITH_TRPC_PTR;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    custom_free_function_is_called = false;
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}


TEST_FUNCTION(trc_ptr_move_does_not_increment_reference)
{
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr = setup_create();;
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr2 = NULL;

    // act
    TRC_PTR_MOVE(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr2, &rc_ptr);
    ASSERT_IS_NOT_NULL(rc_ptr2);

    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr2, NULL);

    // assert
    ASSERT_IS_TRUE(custom_free_function_is_called);

    // cleanup
}

TEST_FUNCTION(trc_ptr_initialize_move_does_not_increment_reference)
{
    // arrange
    STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE x = custom_create_func_for_struct_that_has_its_own_free_func();
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(x, custom_free_func_for_struct_that_has_its_own_free_func);
    STRUCT_WITH_TRPC_PTR cool_new_struct;
    
    TRC_PTR_INITIALIZE_MOVE(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&cool_new_struct.rc_ptr, &rc_ptr);

    // act
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&cool_new_struct.rc_ptr, NULL);

    // assert
    ASSERT_IS_TRUE(custom_free_function_is_called);

    // cleanup
}

TEST_FUNCTION(trc_ptr_initialize_increments_reference)
{
    // arrange
    STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE x = custom_create_func_for_struct_that_has_its_own_free_func();
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(x, custom_free_func_for_struct_that_has_its_own_free_func);
    STRUCT_WITH_TRPC_PTR cool_new_struct;
    TRC_PTR_INITIALIZE(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&cool_new_struct.rc_ptr, rc_ptr);

    // act
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr, NULL);
    ASSERT_IS_FALSE(custom_free_function_is_called);
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&cool_new_struct.rc_ptr, NULL);

    // assert
    ASSERT_IS_TRUE(custom_free_function_is_called);

    // cleanup
}

TEST_FUNCTION(trc_ptr_assign_null_decrements_reference)
{
    // arrange
    STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE x = custom_create_func_for_struct_that_has_its_own_free_func();
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(x, custom_free_func_for_struct_that_has_its_own_free_func);
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr2 = NULL;
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr2, rc_ptr);

    // act
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr, NULL);
    ASSERT_IS_FALSE(custom_free_function_is_called);
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr2, NULL);

    // assert
    ASSERT_IS_TRUE(custom_free_function_is_called);

    // cleanup
}

TEST_FUNCTION(trc_ptr_assign_increments_reference)
{
    // arrange
    STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE x = custom_create_func_for_struct_that_has_its_own_free_func();
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(x, custom_free_func_for_struct_that_has_its_own_free_func);
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr2 = NULL;

    // act
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr2, rc_ptr);

    // assert
    ASSERT_IS_NOT_NULL(rc_ptr2);
    ASSERT_ARE_EQUAL(void_ptr, TRC_PTR_VALUE(rc_ptr), TRC_PTR_VALUE(rc_ptr2));

    // cleanup
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr, NULL);
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr2, NULL);
}

TEST_FUNCTION(trc_ptr_create_with_move_succeeds_with_custom_free)
{
    // arrange
    STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE x = custom_create_func_for_struct_that_has_its_own_free_func();

    // act
    TRC_PTR(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(x, custom_free_func_for_struct_that_has_its_own_free_func);

    // assert
    ASSERT_IS_NOT_NULL(rc_ptr);
    ASSERT_ARE_EQUAL(void_ptr, x->ptr, TRC_PTR_VALUE(rc_ptr)->ptr);

    // cleanup
    TRC_PTR_ASSIGN(STRUCT_THAT_HAS_ITS_OWN_FREE_FUNC_HANDLE)(&rc_ptr, NULL);
}

TEST_FUNCTION(trc_ptr_create_with_move_pointer_succeeds)
{
    // arrange
    int x = 42;
    int* x_ptr = &x;

    // act
    TRC_PTR(INTPTR) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(INTPTR)(x_ptr, NULL);

    // assert
    ASSERT_IS_NOT_NULL(rc_ptr);
    ASSERT_ARE_EQUAL(int, x, *TRC_PTR_VALUE(rc_ptr));

    // cleanup
    TRC_PTR_ASSIGN(INTPTR)(&rc_ptr, NULL);
}

TEST_FUNCTION(trc_ptr_create_with_move_pointer_with_null_ptr_fails)
{
    // arrange

    // act
    TRC_PTR(INTPTR) rc_ptr = TRC_PTR_CREATE_WITH_MOVE_POINTER(INTPTR)(NULL, NULL);

    // assert
    ASSERT_IS_NULL(rc_ptr);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
