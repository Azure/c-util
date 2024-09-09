// Copyright (c) Microsoft. All rights reserved.

#ifndef SYNC_WRAPPER_H
#define SYNC_WRAPPER_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#else
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/interlocked.h"

#include "c_pal/interlocked_hl.h"
#include "c_pal/log_critical_and_terminate.h"

#include "async_type_helper.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define SYNC_WRAPPER_RESULT_VALUES \
    SYNC_WRAPPER_OK, \
    SYNC_WRAPPER_INVALID_ARGS, \
    SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR, \
    SYNC_WRAPPER_CALL_ERROR, \
    SYNC_WRAPPER_OTHER_ERROR \

MU_DEFINE_ENUM(SYNC_WRAPPER_RESULT, SYNC_WRAPPER_RESULT_VALUES)

#define SYNC_WRAPPER_TEST_1 0

#define SYNC_WRAPPER_DO_NOT_USE_ASSIGN_COPY_OUT_ARG(arg_type, arg_name) ASYNC_TYPE_HELPER_DO_NOT_USE_ASSIGN_COPY(arg_type)
#define SYNC_WRAPPER_DO_NOT_USE_ASSIGN_COPY_OUT_ARG_EX(arg_type, arg_name, ...) SYNC_WRAPPER_DO_NOT_USE_ASSIGN_COPY_OUT_ARG(arg_type, arg_name)


// args in signature
#define SYNC_WRAPPER_ARG_IN_DECLARATION(arg_type, arg_name) , arg_type, arg_name

#define SYNC_WRAPPER_ARGS_IN_ARGS(...) \
    MU_FOR_EACH_2(SYNC_WRAPPER_ARG_IN_DECLARATION, __VA_ARGS__)

#define SYNC_WRAPPER_ARGS_IN_DECLARATION(args) \
    MU_C2A(SYNC_WRAPPER_ARGS_, args)

// const args in signature
#define SYNC_WRAPPER_CONST_ARG_IN_DECLARATION_OUT_ARG(arg_type, arg_name) , ASYNC_TYPE_HELPER_ADD_CONST_TYPE(arg_type) arg_name
#define SYNC_WRAPPER_CONST_ARG_IN_DECLARATION_OUT_ARG_EX(arg_type, arg_name, ...) SYNC_WRAPPER_CONST_ARG_IN_DECLARATION_OUT_ARG(arg_type, arg_name)

#define SYNC_WRAPPER_CONST_OUT_ARG_IN_DECLARATION_PROXY(out_arg) \
    MU_C2A(SYNC_WRAPPER_CONST_ARG_IN_DECLARATION_,out_arg)

#define SYNC_WRAPPER_CONST_OUT_ARGS_IN_DECLARATION(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(SYNC_WRAPPER_CONST_OUT_ARG_IN_DECLARATION_PROXY, __VA_ARGS__)), MU_NOEXPAND()))

// arg pointers in signature

#define SYNC_WRAPPER_POINTER_IN_DECLARATION_OUT_ARG(arg_type, arg_name) \
    , arg_type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(arg_type), arg_name
#define SYNC_WRAPPER_POINTER_IN_DECLARATION_OUT_ARG_EX(arg_type, arg_name, ...) SYNC_WRAPPER_POINTER_IN_DECLARATION_OUT_ARG(arg_type, arg_name)

#define SYNC_WRAPPER_OUT_ARG_POINTER_IN_DECLARATION(out_arg) \
    MU_C2A(SYNC_WRAPPER_POINTER_IN_DECLARATION_, out_arg)

#define SYNC_WRAPPER_OUT_ARG_POINTERS_IN_DECLARATION(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(SYNC_WRAPPER_OUT_ARG_POINTER_IN_DECLARATION, __VA_ARGS__)), MU_NOEXPAND()))

// arg values in calls
#define SYNC_WRAPPER_ARG_VALUE(arg_type, arg_name) , arg_name

#define EXPAND_ARG_VALUES_IN_ARGS(...) \
    MU_FOR_EACH_2(SYNC_WRAPPER_ARG_VALUE, __VA_ARGS__)

#define SYNC_WRAPPER_ARG_VALUES_IN_DECLARATION(in_args) \
    MU_C2A(EXPAND_ARG_VALUES_, in_args)

// create fields in struct for out args values
#define FIELD_IN_STRUCT_VALUE_OUT_ARG(arg_type, arg_name) \
    arg_type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(arg_type) arg_name;
#define FIELD_IN_STRUCT_VALUE_OUT_ARG_EX(arg_type, arg_name, ...) FIELD_IN_STRUCT_VALUE_OUT_ARG(arg_type, arg_name)

#define FIELD_IN_STRUCT_OUT_ARG_VALUE_PROXY(out_arg) \
    MU_C2A(FIELD_IN_STRUCT_VALUE_, out_arg)

/* Codes_SRS_SYNC_WRAPPER_01_026: [ OUT_ARG shall be expanded to the appropriate argument wherever needed by using arg_type and arg_name. ]*/
#define SYNC_WRAPPER_FIELDS_IN_STRUCT_OUT_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(FIELD_IN_STRUCT_OUT_ARG_VALUE_PROXY, __VA_ARGS__)), MU_NOEXPAND()))

/* Codes_SRS_SYNC_WRAPPER_01_019: [ If ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} is not defined, on_{async_function_name}_complete shall call a copy function with the following declaration: ]*/
#define EXECUTE_CUSTOM_SYNC_WRAPPER_COPY_OUT_ARG(arg_type, arg_name) \
    bool MU_C2B(arg_name, _free_needed) = false; \
    if (!async_call_context->error_copying_out_args) \
    { \
        if (ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type)(async_call_context->arg_name, arg_name) != 0) \
        { \
            LogError("Copying argument " MU_TOSTRING(arg_name) " of type " MU_TOSTRING(arg_type)); \
            async_call_context->error_copying_out_args = true; \
        } else \
        { \
            MU_C2B(arg_name, _free_needed) = true; \
        } \
    }

/* Codes_SRS_SYNC_WRAPPER_01_025: [ copy_function shall be used as a function with the following declaration ]*/
#define EXECUTE_CUSTOM_SYNC_WRAPPER_COPY_OUT_ARG_EX(arg_type, arg_name, copy_function, free_function, ...) \
    bool MU_C2B(arg_name, _free_needed) = false; \
    if (!async_call_context->error_copying_out_args) \
    { \
        if (copy_function(async_call_context->arg_name, arg_name MU_IFCOMMALOGIC(MU_COUNT_ARG(__VA_ARGS__)) __VA_ARGS__) != 0) \
        { \
            LogError("Copying argument " MU_TOSTRING(arg_name) " of type " MU_TOSTRING(arg_type)); \
            async_call_context->error_copying_out_args = true; \
        } else \
        { \
            MU_C2B(arg_name, _free_needed) = true; \
        } \
    }

/* Codes_SRS_SYNC_WRAPPER_01_018: [ If ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} is defined as 1, on_{async_function_name}_complete shall copy the argument value by assigning it. ]*/
#define EXECUTE_DEFAULT_SYNC_WRAPPER_COPY_OUT_ARG(arg_type, arg_name) \
    *async_call_context->arg_name = arg_name;
#define EXECUTE_DEFAULT_SYNC_WRAPPER_COPY_OUT_ARG_EX(arg_type, arg_name, ...) EXECUTE_DEFAULT_SYNC_WRAPPER_COPY_OUT_ARG(arg_type, arg_name)

#define SYNC_WRAPPER_CALL_COPY_OUT_ARG(out_arg) \
    MU_EXPAND(MU_IF(MU_C2A(SYNC_WRAPPER_DO_NOT_USE_ASSIGN_COPY_,out_arg), MU_NOEXPAND(MU_C2A(EXECUTE_CUSTOM_SYNC_WRAPPER_COPY_,out_arg)), MU_NOEXPAND(MU_C2A(EXECUTE_DEFAULT_SYNC_WRAPPER_COPY_, out_arg))))

// copy callback out args values
#define SYNC_WRAPPER_COPY_CALLBACK_OUT_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(SYNC_WRAPPER_CALL_COPY_OUT_ARG, __VA_ARGS__)), MU_NOEXPAND()))

#define EXECUTE_CUSTOM_SYNC_WRAPPER_FREE_FAILED_OUT_ARG(arg_type, arg_name) \
    if (async_call_context->error_copying_out_args && (MU_C2B(arg_name, _free_needed))) \
    { \
        ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type)(ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(arg_type) async_call_context->arg_name); \
    }

/* Codes_SRS_SYNC_WRAPPER_01_027: [ free_function shall be used as a function with the following declaration ]*/
#define EXECUTE_CUSTOM_SYNC_WRAPPER_FREE_FAILED_OUT_ARG_EX(arg_type, arg_name, copy_function, free_function, ...) \
    if (async_call_context->error_copying_out_args && (MU_C2B(arg_name, _free_needed))) \
    { \
        free_function(*async_call_context->arg_name MU_IFCOMMALOGIC(MU_COUNT_ARG(__VA_ARGS__)) __VA_ARGS__); \
    }

// do nothing
#define EXECUTE_DEFAULT_SYNC_WRAPPER_FREE_FAILED_OUT_ARG(arg_type, arg_name)
#define EXECUTE_DEFAULT_SYNC_WRAPPER_FREE_FAILED_OUT_ARG_EX(arg_type, arg_name, ...) EXECUTE_DEFAULT_SYNC_WRAPPER_FREE_FAILED_OUT_ARG(arg_type, arg_name)

#define SYNC_WRAPPER_CALL_FREE_FAILED_OUT_ARG(out_arg) \
    MU_EXPAND(MU_IF(MU_C2A(SYNC_WRAPPER_DO_NOT_USE_ASSIGN_COPY_,out_arg), MU_NOEXPAND(MU_C2A(EXECUTE_CUSTOM_SYNC_WRAPPER_FREE_FAILED_,out_arg)), MU_NOEXPAND(MU_C2A(EXECUTE_DEFAULT_SYNC_WRAPPER_FREE_FAILED_,out_arg))))

// copy callback out args values
#define SYNC_WRAPPER_FREE_FAILED_CALLBACK_OUT_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(SYNC_WRAPPER_CALL_FREE_FAILED_OUT_ARG, __VA_ARGS__)), MU_NOEXPAND()))

// store out arg pointers
#define STORE_OUT_ARG_PTR_OUT_ARG(arg_type, arg_name) \
    async_call_context.arg_name = arg_name;
#define STORE_OUT_ARG_PTR_OUT_ARG_EX(arg_type, arg_name, ...) STORE_OUT_ARG_PTR_OUT_ARG(arg_type, arg_name)

#define STORE_OUT_ARG_PTR_PROXY(out_arg) \
    MU_C2(STORE_OUT_ARG_PTR_, out_arg)

#define SYNC_WRAPPER_STORE_OUT_ARG_PTRS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(STORE_OUT_ARG_PTR_PROXY, __VA_ARGS__)), MU_NOEXPAND()))

// check out arg pointers
/* Codes_SRS_SYNC_WRAPPER_01_004: [ If any of the out arguments pointers is NULL, the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
#define CHECK_OUT_ARG_PTR_OUT_ARG(arg_type, arg_name) \
    else if (arg_name == NULL) \
    { \
        LogError("NULL " MU_TOSTRING(arg_name) " of type " MU_TOSTRING(arg_type)); \
        sync_wrapper_result = SYNC_WRAPPER_INVALID_ARGS; \
    }

#define CHECK_OUT_ARG_PTR_OUT_ARG_EX(arg_type, arg_name, ...) CHECK_OUT_ARG_PTR_OUT_ARG(arg_type, arg_name)

#define CHECK_OUT_ARG_PTR_PROXY(out_arg) \
    MU_C2A(CHECK_OUT_ARG_PTR_, out_arg)

#define SYNC_WRAPPER_CHECK_OUT_ARG_PTRS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(CHECK_OUT_ARG_PTR_PROXY, __VA_ARGS__)), MU_NOEXPAND()))

/* Codes_SRS_SYNC_WRAPPER_01_016: [ SYNC_WRAPPER shall expand async_function_name to the name of the synchronous wrapper around async_function_name. ]*/
#define SYNC_WRAPPER(async_function_name) \
    MU_C2(async_function_name, _sync_wrapper) \

/* Codes_SRS_SYNC_WRAPPER_01_001: [ DECLARE_SYNC_WRAPPER shall expand to the function declaration: ]*/
#define DECLARE_SYNC_WRAPPER(async_handle_type, async_function_name, return_type, in_args, ...) \
    MOCKABLE_FUNCTION(, SYNC_WRAPPER_RESULT, SYNC_WRAPPER(async_function_name), async_handle_type, async_handle SYNC_WRAPPER_ARGS_IN_DECLARATION(in_args), return_type*, async_function_call_result SYNC_WRAPPER_OUT_ARG_POINTERS_IN_DECLARATION(__VA_ARGS__)); \

#define GENERATE_SYNC_WRAPPER_CONTEXT(async_handle_type, async_function_name, return_type, expected_return, in_args, ...) \
    typedef struct MU_C2(SYNC_WRAPPER(async_function_name), _CONTEXT_TAG) \
    { \
        volatile_atomic int32_t complete; \
        bool error_copying_out_args; \
        SYNC_WRAPPER_FIELDS_IN_STRUCT_OUT_ARGS(__VA_ARGS__) \
    } MU_C2(SYNC_WRAPPER(async_function_name), _CONTEXT);

/* Codes_SRS_SYNC_WRAPPER_01_011: [ DEFINE_SYNC_WRAPPER shall generate a callback to be passed to the asynchronous function with the following declaration: ]*/
/* Codes_SRS_SYNC_WRAPPER_01_013: [ Otherwise, on_{async_function_name}_complete shall store the values of the out args into the context created in synchronous wrapper function. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_015: [ on_{async_function_name}_complete shall unblock the synchronous wrapper function call. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_012: [ If context is NULL, on_{async_function_name}_complete shall terminate the process. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_017: [ For each argument: ]*/
#define GENERATE_SYNC_WRAPPER_CALLBACK(async_handle_type, async_function_name, return_type, expected_return, in_args, ...) \
    static void MU_C3(on_, async_function_name, _complete)(void* context SYNC_WRAPPER_CONST_OUT_ARGS_IN_DECLARATION(__VA_ARGS__)) \
    { \
        if (context == NULL) \
        { \
            LogCriticalAndTerminate("NULL context"); \
        } \
        else \
        { \
            MU_C2(SYNC_WRAPPER(async_function_name), _CONTEXT)* async_call_context = (MU_C2(SYNC_WRAPPER(async_function_name), _CONTEXT)*)context; \
            async_call_context->error_copying_out_args = false; \
            SYNC_WRAPPER_COPY_CALLBACK_OUT_ARGS(__VA_ARGS__) \
            SYNC_WRAPPER_FREE_FAILED_CALLBACK_OUT_ARGS(__VA_ARGS__) \
            if (InterlockedHL_SetAndWake(&async_call_context->complete, 1) != INTERLOCKED_HL_OK) \
            { \
                LogError("InterlockedHL_SetAndWake failed"); \
            } \
        } \
    }

#ifdef _MSC_VER
#define SYNC_WRAPPER_WARNING_SUPPRESS(warn_no) \
    __pragma(warning(suppress:warn_no))
#else
#define SYNC_WRAPPER_WARNING_SUPPRESS(warn_no)
#endif

/* Codes_SRS_SYNC_WRAPPER_01_002: [ The generated synchronous wrapper shall have the declaration: ]*/
/* Codes_SRS_SYNC_WRAPPER_01_003: [ If async_handle is NULL, the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
/* Codes_SRS_SYNC_WRAPPER_42_002: [ If async_function_call_result is NULL then the synchronous wrapper shall fail and return SYNC_WRAPPER_INVALID_ARGS. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_006: [ The synchronous wrapper shall call the asynchronous function async_function_name, pass the in_args as arguments together with the generated completion function and a context used to store out argument pointers. ]*/
/* Codes_SRS_SYNC_WRAPPER_42_003: [ If the asynchronous function returns something other than expected_return then the synchronous wrapper shall return SYNC_WRAPPER_CALL_ERROR. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_007: [ The synchronous wrapper shall wait for the callback to be finished by using InterlockedHL_WaitForValue. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_008: [ On success, the synchronous wrapper shall return SYNC_WRAPPER_OK. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_009: [ On success, the synchronous wrapper shall return in the out args the values of the arguments received in the callback. ]*/
/* Codes_SRS_SYNC_WRAPPER_42_001: [ The synchronous wrapper shall store the result of the asynchronous function call in async_function_call_result. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_010: [ If any other error occurs, the synchronous wrapper shall fail and return SYNC_WRAPPER_OTHER_ERROR. ]*/
/* Codes_SRS_SYNC_WRAPPER_01_020: [ If the callback fails to copy any argument value, the synchronous wrapper shall return SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR. ]*/
#define GENERATE_SYNC_WRAPPER_FUNCTION(async_handle_type, async_function_name, return_type, expected_return, in_args, ...) \
    IMPLEMENT_MOCKABLE_FUNCTION(, SYNC_WRAPPER_RESULT, SYNC_WRAPPER(async_function_name), async_handle_type, async_handle SYNC_WRAPPER_ARGS_IN_DECLARATION(in_args), return_type*, async_function_call_result SYNC_WRAPPER_OUT_ARG_POINTERS_IN_DECLARATION(__VA_ARGS__)) \
    { \
        SYNC_WRAPPER_RESULT sync_wrapper_result; \
        if (async_handle == NULL) \
        { \
            LogError("NULL " MU_TOSTRING(async_handle_type) " async_handle for " MU_TOSTRING(SYNC_WRAPPER(async_function_name))); \
            sync_wrapper_result = SYNC_WRAPPER_INVALID_ARGS; \
        } \
        else if (async_function_call_result == NULL) \
        { \
            LogError("NULL " MU_TOSTRING(return_type) " async_function_call_result for " MU_TOSTRING(SYNC_WRAPPER(async_function_name))); \
            sync_wrapper_result = SYNC_WRAPPER_INVALID_ARGS; \
        } \
        SYNC_WRAPPER_CHECK_OUT_ARG_PTRS(__VA_ARGS__) \
        else \
        { \
            MU_C2(SYNC_WRAPPER(async_function_name), _CONTEXT) async_call_context; \
            (void)interlocked_exchange(&async_call_context.complete, 0); \
            SYNC_WRAPPER_STORE_OUT_ARG_PTRS(__VA_ARGS__) \
            *async_function_call_result = async_function_name(async_handle SYNC_WRAPPER_ARG_VALUES_IN_DECLARATION(in_args), MU_C3(on_, async_function_name, _complete), &async_call_context); \
            if (*async_function_call_result != expected_return) \
            { \
                LogError(MU_TOSTRING(SYNC_WRAPPER(async_function_name)) " failed"); \
                sync_wrapper_result = SYNC_WRAPPER_CALL_ERROR; \
            } \
            else \
            { \
                INTERLOCKED_HL_RESULT interlocked_hl_result = InterlockedHL_WaitForValue(&async_call_context.complete, 1, UINT32_MAX); \
                if (interlocked_hl_result != INTERLOCKED_HL_OK) \
                { \
                    LogError("InterlockedHL_WaitForValue failed with interlocked_hl_result=%" PRI_MU_ENUM "", MU_ENUM_VALUE(INTERLOCKED_HL_RESULT, interlocked_hl_result)); \
                    sync_wrapper_result = SYNC_WRAPPER_OTHER_ERROR; \
                } \
                else \
                { \
                    if (async_call_context.error_copying_out_args) \
                    { \
                        sync_wrapper_result = SYNC_WRAPPER_CALLBACK_COPY_ARG_ERROR; \
                    } \
                    else \
                    { \
                        sync_wrapper_result = SYNC_WRAPPER_OK; \
                    } \
                } \
            } \
        } \
        return sync_wrapper_result; \
    }


#define DEFINE_SYNC_WRAPPER(async_handle_type, async_function_name, return_type, expected_return, in_args, ...) \
    GENERATE_SYNC_WRAPPER_CONTEXT(async_handle_type, async_function_name, return_type, expected_return, in_args, __VA_ARGS__) \
    GENERATE_SYNC_WRAPPER_CALLBACK(async_handle_type, async_function_name, return_type, expected_return, in_args, __VA_ARGS__) \
    GENERATE_SYNC_WRAPPER_FUNCTION(async_handle_type, async_function_name, return_type, expected_return, in_args, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // SYNC_WRAPPER_H
