// Copyright (c) Microsoft. All rights reserved.

#ifndef ASYNC_RETRY_WRAPPER_H
#define ASYNC_RETRY_WRAPPER_H

#ifdef __cplusplus
#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#else
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/threadapi.h"
#include "c_pal/threadpool.h"
#include "c_pal/timer.h"
#include "c_pal/thandle.h"
#include "c_pal/log_critical_and_terminate.h"

#include "c_util/async_type_helper.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ASYNC_RETRY_WRAPPER_RESULT_VALUES \
    ASYNC_RETRY_WRAPPER_OK, \
    ASYNC_RETRY_WRAPPER_ERROR, \
    ASYNC_RETRY_WRAPPER_INVALID_ARGS, \
    ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR, \
    ASYNC_RETRY_WRAPPER_CALL_ERROR, \
    ASYNC_RETRY_WRAPPER_TIMEOUT \

MU_DEFINE_ENUM(ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_RESULT_VALUES)

// Maximum back-off for retries is 10 seconds
#define ASYNC_RETRY_WRAPPER_MAX_BACKOFF_MS 10000

/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_015: [ ASYNC_RETRY_WRAPPER shall expand async_function_name to the name of the asynchronous retry wrapper around async_function_name. ]*/
#define ASYNC_RETRY_WRAPPER(async_function_name) \
    MU_C2A(async_function_name, _async_retry_wrapper)

/*Codes_*/
#define ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(async_function_name) \
    MU_C2A(async_function_name, _async_retry_wrapper_with_timeout)

#define ASYNC_RETRY_WRAPPER_CONTEXT(async_function_name) \
    MU_C2(ASYNC_RETRY_WRAPPER(async_function_name), _CONTEXT)

#define ASYNC_RETRY_WRAPPER_CALLBACK(async_function_name) \
    MU_C3(on_, async_function_name, _complete)

#define ASYNC_RETRY_WRAPPER_RETRY_FUNC(async_function_name) \
    MU_C2(async_function_name, _do_retry)

// Context

#define ASYNC_RETRY_WRAPPER_STRUCT_FIELD_ARG(arg_type, arg_name) ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type) arg_name;
#define ASYNC_RETRY_WRAPPER_STRUCT_FIELD_ARG_EX(arg_type, arg_name, ...) ASYNC_RETRY_WRAPPER_STRUCT_FIELD_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_STRUCT_FIELD_ARG_CB(arg_type, arg_name) arg_type user_captured_callback;
#define ASYNC_RETRY_WRAPPER_STRUCT_FIELD_ARG_CONTEXT(arg_type, arg_name) arg_type user_captured_callback_context;

#define ASYNC_RETRY_WRAPPER_STRUCT_FIELD_PROXY(arg) \
    MU_C2B(ASYNC_RETRY_WRAPPER_STRUCT_FIELD_, arg)

#define ASYNC_RETRY_WRAPPER_STRUCT_FIELD_IN_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_STRUCT_FIELD_PROXY, __VA_ARGS__)

#define GENERATE_ASYNC_RETRY_WRAPPER_CONTEXT(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    typedef struct MU_C2(ASYNC_RETRY_WRAPPER_CONTEXT(async_function_name), _TAG) \
    { \
        async_handle_type handle; \
        THANDLE(THREADPOOL) threadpool; \
        double start_time;  /*start_time records the time (as returned by timer_global_get_elapsed_ms) when the first call to async_function_name has been made*/ \
        uint32_t timeout_ms; /*timeout_ms is the maximum time in milliseconds that the asynchronous function is allowed to run*/ \
        unsigned int backoff; \
        MU_C2A(ASYNC_RETRY_WRAPPER_STRUCT_FIELD_, in_args) \
    } ASYNC_RETRY_WRAPPER_CONTEXT(async_function_name);

// Args for callback
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_DECLARATION_ARG(arg_type, arg_name, error_value, ...) , arg_type arg_name

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_DECLARATION_ENUM(arg_type, arg_name, error_value, ...) , arg_type arg_name

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_DECLARATION_PROXY(out_arg) \
    MU_C2B(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_DECLARATION_, out_arg)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_DECLARATION_OUT_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_DECLARATION_PROXY, __VA_ARGS__)) , MU_NOEXPAND()))

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_DECLARATION_PROXY(out_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_DECLARATION_, out_args)

// Args for calling callback to user
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_ARG(arg_type, arg_name, error_value) , arg_name
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_ENUM(arg_type, arg_name, error_value, ...) ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_ARG(arg_type, arg_name, error_value)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_PROXY(out_arg) \
    MU_C2(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_, out_arg)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_OUT_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_PROXY, __VA_ARGS__)), MU_NOEXPAND()))

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_PROXY(out_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_, out_args)

// Args for calling callback to user but override the enum value
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_ARG(arg_type, arg_name, error_value) , error_value


#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_ENUM_0(arg_type, arg_name, error_value) , error_value
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_ENUM_1(arg_type, arg_name, error_value, timeout_value) , error_value

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_ENUM(arg_type, arg_name, error_value, ...) \
    MU_C2C(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_ENUM_, MU_COUNT_ARG(__VA_ARGS__))(arg_type, arg_name, error_value, ##__VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_ARG(arg_type, arg_name, error_value) , error_value

/*Codes_SRS_ASYNC_RETRY_WRAPPER_02_004: [ If there's no timeout_error_value specified by ENUM(...) macro then the error_value shall be used instead. ]*/
/*Codes_SRS_ASYNC_RETRY_WRAPPER_02_002: [ If there's no timeout_error_value for ENUM(...) argument then the error_value shall be instead passed. ]*/
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_ENUM_0(arg_type, arg_name, error_value) , error_value
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_ENUM_1(arg_type, arg_name, error_value, timeout_value) , timeout_value

/*Codes_SRS_ASYNC_RETRY_WRAPPER_02_002: [ If there's no timeout_error_value for ENUM(...) argument then the error_value shall be instead passed. ]*/
/*Codes_SRS_ASYNC_RETRY_WRAPPER_02_004: [ If there's no timeout_error_value specified by ENUM(...) macro then the error_value shall be used instead. ]*/
#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_ENUM(arg_type, arg_name, error_value, ...) \
    MU_C2C(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_ENUM_, MU_COUNT_ARG(__VA_ARGS__))(arg_type, arg_name, error_value, ##__VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_PROXY(out_arg) \
    MU_C2B(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_, out_arg)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_PROXY(out_arg) \
    MU_C2B(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_, out_arg)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_ERROR_OUT_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_ERROR_PROXY, __VA_ARGS__)), MU_NOEXPAND()))

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_ERROR_PROXY(out_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_ERROR_, out_args)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_TIMEOUT_PROXY(out_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_TIMEOUT_, out_args)

#define ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_TIMEOUT_OUT_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CALLBACK_ARG_CALL_WITH_TIMEOUT_PROXY, __VA_ARGS__)), MU_NOEXPAND()))

// Retry test (sync)
#define ASYNC_RETRY_WRAPPER_RETRY_CONDITION_SYNC(value) || temp_async_function_result == value

#define ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_RETRY_ON_SYNC(...) \
    false MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_RETRY_CONDITION_SYNC, __VA_ARGS__)), MU_NOEXPAND()))

// Retry test (async)
#define ASYNC_RETRY_WRAPPER_RETRY_CONDITION_ASYNC(value) || enum_to_check == value

#define ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_RETRY_ON_ASYNC(...) \
    false MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_RETRY_CONDITION_ASYNC, __VA_ARGS__)),MU_NOEXPAND()))

#define ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_PROXY(retry_enums) \
    MU_C2A(ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_, retry_enums)

#define ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_OUT_ARGS_ENUM(arg_type, arg_name, error_value, ...) \
    int enum_to_check_already_defined_only_one_ENUM_allowed_in_out_args; \
    (void)enum_to_check_already_defined_only_one_ENUM_allowed_in_out_args; \
    arg_type enum_to_check = arg_name;
#define ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_OUT_ARGS_ARG(arg_type, arg_name, error_value)

#define ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_OUT_ARG_PROXY(arg) \
    MU_C2(ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_OUT_ARGS_, arg)

#define ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_OUT_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_OUT_ARG_PROXY, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_PROXY(out_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_, out_args)

#define ASYNC_RETRY_WRAPPER_COUNT_RETRY_ON_ASYNC(...) MU_COUNT_ARG(__VA_ARGS__)
#define ASYNC_RETRY_WRAPPER_HAS_ASYNC_RETRY(retry_enums) \
    MU_C2A(ASYNC_RETRY_WRAPPER_COUNT_, retry_enums)

// In args
#define ASYNC_RETRY_WRAPPER_ARG_ARG(arg_type, arg_name) , arg_type, arg_name
#define ASYNC_RETRY_WRAPPER_ARG_ARG_EX(arg_type, arg_name, ...) ASYNC_RETRY_WRAPPER_ARG_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_ARG_ARG_CB(arg_type, arg_name) ASYNC_RETRY_WRAPPER_ARG_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_ARG_ARG_CONTEXT(arg_type, arg_name) ASYNC_RETRY_WRAPPER_ARG_ARG(arg_type, arg_name)

#define ASYNC_RETRY_WRAPPER_ARG_PROXY(arg) MU_EXPAND(MU_NOEXPAND(MU_C2(ASYNC_RETRY_WRAPPER_ARG_, arg)))

#define ASYNC_RETRY_WRAPPER_ARGS_IN_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_ARG_PROXY, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_ARGS_IN_DECLARATION(in_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_ARGS_, in_args)

// In args for static (no comma after type)
#define ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_ARG(arg_type, arg_name) , arg_type arg_name
#define ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_ARG_EX(arg_type, arg_name, ...) ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_ARG_CB(arg_type, arg_name) ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_ARG_CONTEXT(arg_type, arg_name) ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_ARG(arg_type, arg_name)

#define ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_PROXY(arg) MU_EXPAND(MU_NOEXPAND(MU_C2(ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_, arg)))

#define ASYNC_RETRY_WRAPPER_ARGS_NO_COMMA_IN_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_ARG_NO_COMMA_PROXY, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_ARGS_IN_STATIC_DECLARATION(in_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_ARGS_NO_COMMA_, in_args)

// In args for calling static (forward all args as-is)
#define ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_ARG(arg_type, arg_name) , arg_name
#define ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_ARG_EX(arg_type, arg_name, ...) ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_ARG_CB(arg_type, arg_name) ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_ARG_CONTEXT(arg_type, arg_name) ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_ARG(arg_type, arg_name)

#define ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_PROXY(arg) MU_EXPAND(MU_NOEXPAND(MU_C2(ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_, arg)))

#define ASYNC_RETRY_WRAPPER_ARGS_MAKE_CALL_IN_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_ARG_MAKE_CALL_PROXY, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_ARGS_MAKE_STATIC_CALL(in_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_ARGS_MAKE_CALL_, in_args)

// In args for function call
#define ASYNC_RETRY_WRAPPER_CALL_ARG_ARG(arg_type, arg_name) , retry_context->arg_name
#define ASYNC_RETRY_WRAPPER_CALL_ARG_ARG_EX(arg_type, arg_name, ...) ASYNC_RETRY_WRAPPER_CALL_ARG_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_CALL_ARG_ARG_CB(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_CALL_ARG_ARG_CONTEXT(arg_type, arg_name)

#define ASYNC_RETRY_WRAPPER_CALL_ARG_PROXY(arg) MU_C2(ASYNC_RETRY_WRAPPER_CALL_ARG_, arg)

#define ASYNC_RETRY_WRAPPER_CALL_ARGS_IN_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CALL_ARG_PROXY, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_ARGS_IN_CALL(args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_CALL_ARGS_, args)

// In arg validation
#define ASYNC_RETRY_WRAPPER_CHECK_ARG_ARG_CB(arg_type, arg_name) \
    else if (arg_name == NULL) \
    { \
        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_004: [ If the parameter specified in ARG_CB(type, name) is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/ \
        LogError("NULL " MU_TOSTRING(arg_name) " of type " MU_TOSTRING(arg_type)); \
        async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_INVALID_ARGS; \
    }
#define ASYNC_RETRY_WRAPPER_CHECK_ARG_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_CHECK_ARG_ARG_EX(arg_type, arg_name, ...)
#define ASYNC_RETRY_WRAPPER_CHECK_ARG_ARG_CONTEXT(arg_type, arg_name)

#define ASYNC_RETRY_WRAPPER_CHECK_ARG_PROXY(in_arg) \
    MU_C2B(ASYNC_RETRY_WRAPPER_CHECK_ARG_, in_arg)

#define ASYNC_RETRY_WRAPPER_CHECK_IN_ARGS(...) \
    MU_EXPAND(MU_IF(MU_COUNT_ARG(__VA_ARGS__), MU_NOEXPAND(MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CHECK_ARG_PROXY, __VA_ARGS__)),MU_NOEXPAND()))

#define ASYNC_RETRY_WRAPPER_CHECK_ARG_PTRS(in_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_CHECK_, in_args)

// Copy in args for retry
#define ASYNC_RETRY_WRAPPER_CUSTOM_HANDLER_EX_PASS_ARG_FROM_CONTEXT(arg) , retry_context->arg

#define ASYNC_RETRY_WRAPPER_CUSTOM_HANDLER_EX_PASS_ARGS_FROM_CONTEXT(...) \
    MU_FOR_EACH_1B(ASYNC_RETRY_WRAPPER_CUSTOM_HANDLER_EX_PASS_ARG_FROM_CONTEXT, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_EXECUTE_CUSTOM_COPY_ARG(arg_type, arg_name) \
    bool MU_C2C(arg_name, _free_needed) = false; \
    if (!error_copying_args) \
    { \
        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_008: [ If an argument in IN_ARGS(...) has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1, then the asynchronous retry wrapper shall copy the argument by calling async_retry_wrapper_{type}_copy. ]*/ \
        if (ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type)(&retry_context->arg_name, arg_name) != 0) \
        { \
            LogError("Copying argument " MU_TOSTRING(arg_name) " of type " MU_TOSTRING(arg_type) " failed"); \
            error_copying_args = true; \
        } \
        else \
        { \
            MU_C2C(arg_name, _free_needed) = true; \
        } \
    }

/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_035: [ copy_function shall be used as a function with the following declaration: ]*/
#define ASYNC_RETRY_WRAPPER_EXECUTE_CUSTOM_COPY_ARG_EX(arg_type, arg_name, copy_function, free_function, ...) \
    bool MU_C2C(arg_name, _free_needed) = false; \
    if (!error_copying_args) \
    { \
        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_007: [ If an argument in IN_ARGS(...) is specified with ARG_EX(type, name, copy_function, free_function, ...) then the asynchronous retry wrapper shall call copy_function and pass the ... arguments in order to copy to the context. ]*/ \
        if (copy_function(&retry_context->arg_name, arg_name, __VA_ARGS__) != 0) \
        { \
            LogError("Copying argument " MU_TOSTRING(arg_name) " of type " MU_TOSTRING(arg_type) " failed"); \
            error_copying_args = true; \
        } \
        else \
        { \
            MU_C2C(arg_name, _free_needed) = true; \
        } \
    }

#define ASYNC_RETRY_WRAPPER_EXECUTE_CUSTOM_COPY_ARG_CB(arg_type, arg_name) ASYNC_RETRY_WRAPPER_EXECUTE_CUSTOM_COPY_ARG(arg_type, arg_name) _error_undefined_variable_ = "Invalid IN_ARG marked as CB but using a custom copy";
#define ASYNC_RETRY_WRAPPER_EXECUTE_CUSTOM_COPY_ARG_CONTEXT(arg_type, arg_name) ASYNC_RETRY_WRAPPER_EXECUTE_CUSTOM_COPY_ARG(arg_type, arg_name) _error_undefined_variable_ = "Invalid IN_ARG marked as CONTEXT but using a custom copy";

#define ASYNC_RETRY_WRAPPER_DO_NOT_USE_ASSIGN_COPY_ARG(arg_type, arg_name) ASYNC_TYPE_HELPER_DO_NOT_USE_ASSIGN_COPY(arg_type)
#define ASYNC_RETRY_WRAPPER_DO_NOT_USE_ASSIGN_COPY_ARG_EX(arg_type, arg_name, ...) ASYNC_TYPE_HELPER_DO_NOT_USE_ASSIGN_COPY(arg_type)
#define ASYNC_RETRY_WRAPPER_DO_NOT_USE_ASSIGN_COPY_ARG_CB(arg_type, arg_name) 0
#define ASYNC_RETRY_WRAPPER_DO_NOT_USE_ASSIGN_COPY_ARG_CONTEXT(arg_type, arg_name) 0

/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_009: [ Otherwise, the asynchronous retry wrapper shall copy the argument to the context via assignment. ]*/
#define ASYNC_RETRY_WRAPPER_EXECUTE_DEFAULT_COPY_ARG(arg_type, arg_name) \
    retry_context->arg_name = arg_name;
#define ASYNC_RETRY_WRAPPER_EXECUTE_DEFAULT_COPY_ARG_EX(arg_type, arg_name, ...) ASYNC_RETRY_WRAPPER_EXECUTE_DEFAULT_COPY_ARG(arg_type, arg_name)
#define ASYNC_RETRY_WRAPPER_EXECUTE_DEFAULT_COPY_ARG_CB(arg_type, arg_name) \
    retry_context->user_captured_callback = arg_name;
#define ASYNC_RETRY_WRAPPER_EXECUTE_DEFAULT_COPY_ARG_CONTEXT(arg_type, arg_name) \
    retry_context->user_captured_callback_context = arg_name;

#define ASYNC_RETRY_WRAPPER_CALL_COPY_ARG_PROXY(in_arg) \
    MU_EXPAND(MU_IF(MU_C2B(ASYNC_RETRY_WRAPPER_DO_NOT_USE_ASSIGN_COPY_, in_arg), MU_NOEXPAND(MU_C2B(ASYNC_RETRY_WRAPPER_EXECUTE_CUSTOM_COPY_, in_arg)), MU_NOEXPAND(MU_C2B(ASYNC_RETRY_WRAPPER_EXECUTE_DEFAULT_COPY_, in_arg))))

#define ASYNC_RETRY_WRAPPER_COPY_ARGS_IN_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CALL_COPY_ARG_PROXY, __VA_ARGS__)

/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_006: [ The asynchronous retry wrapper shall copy each argument from IN_ARGS(...) to the allocated context. ]*/
#define ASYNC_RETRY_WRAPPER_COPY_ARGS_PROXY(in_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_COPY_ARGS_, in_args)

// Free in args on failure (check _free_needed because argument may not have been copied)
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FAILED_ARG(arg_type, arg_name) \
    if (MU_C2C(arg_name, _free_needed)) \
    { \
       ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type)(retry_context->arg_name); \
    }
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_036: [ free_function shall be used as a function with the following declaration: ]*/
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FAILED_ARG_EX(arg_type, arg_name, copy_function, free_function, ...) \
    if (MU_C2C(arg_name, _free_needed)) \
    { \
        free_function(retry_context->arg_name, __VA_ARGS__); \
    }
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FAILED_ARG_CB(arg_type, arg_name) _error_undefined_variable_ = "Invalid IN_ARG marked as CB but using a custom free";
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FAILED_ARG_CONTEXT(arg_type, arg_name) _error_undefined_variable_ = "Invalid IN_ARG marked as CONTEXT but using a custom free";

#define ASYNC_RETRY_WRAPPER_CALL_FREE_FAILED_IN_ARG(in_arg) \
    MU_EXPAND(MU_IF(MU_C2B(ASYNC_RETRY_WRAPPER_DO_NOT_USE_ASSIGN_COPY_, in_arg), MU_NOEXPAND(MU_C2B(ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FAILED_,in_arg)), MU_NOEXPAND())) \

#define ASYNC_RETRY_WRAPPER_FREE_FAILED_IN_ARGS(...) \
    MU_FOR_EACH_1(ASYNC_RETRY_WRAPPER_CALL_FREE_FAILED_IN_ARG, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_FREE_FAILED_PROXY(in_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_FREE_FAILED_, in_args)

// Free in args for normal cleanup, cleanup all fields
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_020: [ on_{async_function_name}_complete shall free each argument in IN_ARGS(...) that has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1 by calling async_retry_wrapper_{type}_free. ]*/
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_027: [ {async_function_name}_do_retry shall free each argument in IN_ARGS(...) that has a type that does not define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type} as 1 by calling async_retry_wrapper_{type}_free. ]*/
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FIELD_ARG(arg_type, arg_name) \
    ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type)(retry_context->arg_name);
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_019: [ on_{async_function_name}_complete shall free each argument in IN_ARGS(...) that is specified with ARG_EX(type, name, copy_function, free_function, ...) by calling free_function and passing the ... arguments. ]*/
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_026: [ {async_function_name}_do_retry shall free each argument in IN_ARGS(...) that is specified with ARG_EX(type, name, copy_function, free_function, ...) by calling free_function and passing the ... arguments. ]*/
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_036: [ free_function shall be used as a function with the following declaration: ]*/
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FIELD_ARG_EX(arg_type, arg_name, copy_function, free_function, ...) \
    free_function(retry_context->arg_name ASYNC_RETRY_WRAPPER_CUSTOM_HANDLER_EX_PASS_ARGS_FROM_CONTEXT(__VA_ARGS__));
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FIELD_ARG_CB(arg_type, arg_name) _error_undefined_variable_ = "Invalid IN_ARG marked as CB but using a custom free";
#define ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FIELD_ARG_CONTEXT(arg_type, arg_name) _error_undefined_variable_ = "Invalid IN_ARG marked as CONTEXT but using a custom free";

#define ASYNC_RETRY_WRAPPER_CALL_FREE_FIELDS_IN_ARG(in_arg) \
    MU_EXPAND(MU_IF(MU_C2B(ASYNC_RETRY_WRAPPER_DO_NOT_USE_ASSIGN_COPY_, in_arg), MU_NOEXPAND(MU_C2B(ASYNC_RETRY_WRAPPER_CUSTOM_FREE_FIELD_, in_arg)), MU_NOEXPAND())) \

#define ASYNC_RETRY_WRAPPER_FREE_FIELDS_IN_ARGS(...) \
    MU_FOR_EACH_1A(ASYNC_RETRY_WRAPPER_CALL_FREE_FIELDS_IN_ARG, __VA_ARGS__)

#define ASYNC_RETRY_WRAPPER_FREE_FIELDS_PROXY(in_args) \
    MU_C2A(ASYNC_RETRY_WRAPPER_FREE_FIELDS_, in_args)

// Execute retry
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_022: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate a function to retry the asynchronous function with the following declaration: ]*/
#define GENERATE_ASYNC_RETRY_WRAPPER_EXECUTE_RETRY_FUNCTION(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    static void ASYNC_RETRY_WRAPPER_RETRY_FUNC(async_function_name)(void* context) \
    { \
        if (context == NULL) \
        { \
            LogCriticalAndTerminate("NULL context for " MU_TOSTRING(async_function_name) " do retry function"); \
        } \
        else \
        { \
            ASYNC_RETRY_WRAPPER_CONTEXT(async_function_name)* retry_context = context; \
            return_type temp_async_function_result; \
            bool timed_out = false; \
            do \
            { \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_023: [ {async_function_name}_do_retry shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/ \
                temp_async_function_result = async_function_name(retry_context->handle ASYNC_RETRY_WRAPPER_ARGS_IN_CALL(in_args), ASYNC_RETRY_WRAPPER_CALLBACK(async_function_name), retry_context); \
                if (ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_PROXY(retry_sync_enums)) \
                { \
                    if (retry_context->timeout_ms < UINT32_MAX) \
                    { \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_02_001: [ Before each retry of the function, if timeout_ms milliseconds have elapsed since the initial call to ASYNC_RETRY_WRAPPER(async_function_name) then {async_function_name}_async_retry_wrapper_with_timeout shall call shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), the error_value from all of the ARG(type, name, error_value)'s in out_args. and the timeout_error_value for the ENUM(...) argument. ]*/ \
                        double elapsed_time_ms = timer_global_get_elapsed_ms() - retry_context->start_time; \
                        LogInfo("elapsed_time_ms=%lf + retry_context->backoff=%" PRIu32 " > retry_context->timeout_ms=%" PRIu32 "", \
                            elapsed_time_ms, retry_context->backoff, retry_context->timeout_ms); \
                        if (elapsed_time_ms + retry_context->backoff > retry_context->timeout_ms) \
                        { \
                            timed_out = true; \
                            LogError("Retries for " MU_TOSTRING(async_function_name) " timed out after %lf ms (including %" PRIu32 " ms of backoff) (timeout time was %" PRIu32 " ms)", \
                                elapsed_time_ms, retry_context->backoff, retry_context->timeout_ms); \
                            break; \
                        } \
                    } \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_045: [ Before each retry of the function, {async_function_name}_do_retry shall yield execution by a call to ThreadAPI_Sleep. ]*/ \
                    ThreadAPI_Sleep(retry_context->backoff); \
                    retry_context->backoff *= 2; \
                    if (retry_context->backoff > ASYNC_RETRY_WRAPPER_MAX_BACKOFF_MS) \
                    { \
                        retry_context->backoff = ASYNC_RETRY_WRAPPER_MAX_BACKOFF_MS; \
                    } \
                } \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_040: [ While async_function_name returns one of the values from RETRY_ON_SYNC(...), it shall be called again in a loop. ]*/ \
            } while (ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_PROXY(retry_sync_enums)); \
            if (timed_out) \
            { \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_041: [ If async_function_name returns a value other than expected_return then: ]*/ \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_025: [ {async_function_name}_do_retry shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/ \
                retry_context->user_captured_callback(retry_context->user_captured_callback_context ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_TIMEOUT_PROXY(out_args)); \
                ASYNC_RETRY_WRAPPER_FREE_FIELDS_PROXY(in_args); \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_11_001: [ on_{async_function_name}_complete shall assign the threadpool to NULL. ]*/ \
                THANDLE_ASSIGN(THREADPOOL)(&retry_context->threadpool, NULL); \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_028: [ {async_function_name}_do_retry shall free the allocated context. ]*/ \
                free(retry_context); \
            } \
            else \
            { \
                if (temp_async_function_result != expected_return) \
                { \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_041: [ If async_function_name returns a value other than expected_return then: ]*/ \
                    LogError(MU_TOSTRING(async_function_name) " failed during retry"); \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_025: [ {async_function_name}_do_retry shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/ \
                    retry_context->user_captured_callback(retry_context->user_captured_callback_context ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_ERROR_PROXY(out_args)); \
                    ASYNC_RETRY_WRAPPER_FREE_FIELDS_PROXY(in_args); \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_11_001: [ on_{async_function_name}_complete shall assign the threadpool to NULL. ]*/ \
                    THANDLE_ASSIGN(THREADPOOL)(&retry_context->threadpool, NULL); \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_028: [ {async_function_name}_do_retry shall free the allocated context. ]*/ \
                    free(retry_context); \
                } \
                else \
                { \
                    /* Retry attempted, callback will be called */ \
                } \
            } \
        } \
    }

// Callback implementation
#define DECLARE_ASYNC_RETRY_WRAPPER_CALLBACK(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    static void ASYNC_RETRY_WRAPPER_CALLBACK(async_function_name)(void* context ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_DECLARATION_PROXY(out_args));

/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_014: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate a callback to be passed to the asynchronous function with the following declaration: ]*/
#define GENERATE_ASYNC_RETRY_WRAPPER_CALLBACK(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    static void ASYNC_RETRY_WRAPPER_CALLBACK(async_function_name)(void* context ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_DECLARATION_PROXY(out_args)) \
    { \
        if (context == NULL) \
        { \
            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_016: [ If context is NULL, on_{async_function_name}_complete shall terminate the process. ]*/ \
            LogCriticalAndTerminate("NULL context for " MU_TOSTRING(async_function_name) " callback"); \
        } \
        else \
        { \
            ASYNC_RETRY_WRAPPER_CONTEXT(async_function_name)* retry_context = context; \
            ASYNC_RETRY_WRAPPER_GET_ENUM_TO_CHECK_PROXY(out_args) \
            if (ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_PROXY(retry_async_enums)) \
            { \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_02_003: [ If the out_arg specified as ENUM(type, name, error_value) has one of the values from RETRY_ON_ASYNC(...) and the the time measured from the initial call to ASYNC_RETRY_WRAPPER() exceeded timeout_ms then the user callback specified in ARG_CB(...) shall be called, passing the context from ARG_CONTEXT(...), and the out_args as they were received by this callback handler with the exception of the ENUM(...) which will have the value specified for timeout_error_value. ]*/ \
                bool timed_out = false; \
                if (retry_context->timeout_ms < UINT32_MAX) \
                { \
                    double elapsed_time_ms = timer_global_get_elapsed_ms() - retry_context->start_time; \
                    if (elapsed_time_ms + retry_context->backoff > retry_context->timeout_ms) \
                    { \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_049: [ Before each retry of the function, If timeout_ms milliseconds have elapsed then {async_function_name}_async_retry_wrapper_with_timeout shall fail and return ASYNC_RETRY_WRAPPER_TIMEOUT. ]*/ \
                        LogError("Retries for " MU_TOSTRING(async_function_name) " timed out after %lf ms (including %" PRIu32 " ms of backoff) (timeout time was %" PRIu32 " ms)", \
                            elapsed_time_ms, retry_context->backoff, retry_context->timeout_ms); \
                            timed_out = true; \
                        /* Codes_SRS_ASYNC_RETRY_WRAPPER_01_001: [ If any error occurs, on_{async_function_name}_complete shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/ \
                        retry_context->user_captured_callback(retry_context->user_captured_callback_context ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_TIMEOUT_PROXY(out_args));\
                        ASYNC_RETRY_WRAPPER_FREE_FIELDS_PROXY(in_args); \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_11_001: [ on_{async_function_name}_complete shall assign the threadpool to NULL. ]*/ \
                        THANDLE_ASSIGN(THREADPOOL)(&retry_context->threadpool, NULL); \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_021: [ on_{async_function_name}_complete shall free the allocated context. ]*/ \
                        free(retry_context); \
                    } \
                } \
                if(!timed_out) \
                { \
                    LogVerbose("Scheduling retry of " MU_TOSTRING(async_function_name)); \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_017: [ If the out_arg specified as ENUM(type, name, error_value) has one of the values from RETRY_ON(...) then on_{async_function_name}_complete shall call threadpool_schedule_work with {async_function_name}_do_retry as the work_function to retry the asynchronous call and return. ]*/ \
                    if (threadpool_schedule_work(retry_context->threadpool, ASYNC_RETRY_WRAPPER_RETRY_FUNC(async_function_name), retry_context) != 0) \
                    { \
                        LogError("threadpool_schedule_work failed for " MU_TOSTRING(async_function_name)); \
                        /* Codes_SRS_ASYNC_RETRY_WRAPPER_01_001: [ If any error occurs, on_{async_function_name}_complete shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the error_value from all of the ARG(type, name, error_value)'s in out_args. ]*/ \
                        retry_context->user_captured_callback(retry_context->user_captured_callback_context ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_WITH_ERROR_PROXY(out_args)); \
                        ASYNC_RETRY_WRAPPER_FREE_FIELDS_PROXY(in_args); \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_11_001: [ on_{async_function_name}_complete shall assign the threadpool to NULL. ]*/ \
                        THANDLE_ASSIGN(THREADPOOL)(&retry_context->threadpool, NULL); \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_021: [ on_{async_function_name}_complete shall free the allocated context. ]*/ \
                        free(retry_context); \
                    } \
                    else \
                    { \
                        /* All OK, will execute on threadpool callback */ \
                    } \
                } \
            } \
            else \
            { \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_018: [ on_{async_function_name}_complete shall call the user callback specified in ARG_CB(...), passing the context from ARG_CONTEXT(...), and the out_args as they were received by this callback handler. ]*/ \
                retry_context->user_captured_callback(retry_context->user_captured_callback_context ASYNC_RETRY_WRAPPER_CALLBACK_ARGS_FOR_CALL_PROXY(out_args)); \
                ASYNC_RETRY_WRAPPER_FREE_FIELDS_PROXY(in_args); \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_11_001: [ on_{async_function_name}_complete shall assign the threadpool to NULL. ]*/ \
                THANDLE_ASSIGN(THREADPOOL)(&retry_context->threadpool, NULL); \
                /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_021: [ on_{async_function_name}_complete shall free the allocated context. ]*/ \
                free(retry_context); \
            } \
        } \
    }

// Call implementation
#define GENERATE_ASYNC_RETRY_WRAPPER_FUNCTION(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    static ASYNC_RETRY_WRAPPER_RESULT MU_C2(ASYNC_RETRY_WRAPPER(async_function_name), _internal)(async_handle_type async_handle, THANDLE(THREADPOOL) threadpool, uint32_t timeout_ms ASYNC_RETRY_WRAPPER_ARGS_IN_STATIC_DECLARATION(in_args), return_type* async_function_result) \
    { \
        ASYNC_RETRY_WRAPPER_RESULT async_retry_wrapper_result; \
        if (async_handle == NULL) \
        { \
            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_003: [ If async_handle is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/ \
            LogError("NULL " MU_TOSTRING(async_handle_type) " async_handle for " MU_TOSTRING(async_function_name)); \
            async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_INVALID_ARGS; \
        } \
        else if (async_function_result == NULL) \
        { \
            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_042: [ If async_function_result is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/ \
            LogError("NULL " MU_TOSTRING(return_type) "* async_function_result for " MU_TOSTRING(async_function_name)); \
            async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_INVALID_ARGS; \
        } \
        else if (threadpool == NULL) \
        { \
            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_037: [ If threadpool is NULL, the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_INVALID_ARGS. ]*/ \
            LogError("NULL THANDLE(THREADPOOL) threadpool for " MU_TOSTRING(async_function_name)); \
            async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_INVALID_ARGS; \
        } \
        ASYNC_RETRY_WRAPPER_CHECK_ARG_PTRS(in_args) \
        else \
        { \
            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_005: [ The asynchronous retry wrapper shall allocate a context for the asynchronous call. ]*/ \
            ASYNC_RETRY_WRAPPER_CONTEXT(async_function_name)* retry_context = malloc(sizeof(ASYNC_RETRY_WRAPPER_CONTEXT(async_function_name))); \
            if (retry_context == NULL) \
            { \
                LogError("Failed to create context for " MU_TOSTRING(async_function_name)); \
                async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_ERROR; \
            } \
            else \
            { \
                if (timeout_ms < UINT32_MAX) \
                { \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_048: [ {async_function_name}_async_retry_wrapper_with_timeout shall get the current time by calling timer_global_get_elapsed_ms. ]*/ \
                    retry_context->start_time = timer_global_get_elapsed_ms(); \
                } \
                retry_context->timeout_ms = timeout_ms; \
                bool error_copying_args = false; \
                ASYNC_RETRY_WRAPPER_COPY_ARGS_PROXY(in_args) \
                if (error_copying_args) \
                { \
                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_010: [ If there are any failures when copying the input arguments then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR. ]*/ \
                    async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_COPY_ARG_ERROR; \
                } \
                else \
                { \
                    retry_context->handle = async_handle; \
                    THANDLE_INITIALIZE(THREADPOOL)(&retry_context->threadpool, threadpool); \
                    return_type temp_async_function_result; \
                    retry_context->backoff = 1; \
                    bool timed_out = false; \
                    do \
                    { \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_011: [ The asynchronous retry wrapper shall call the function async_function_name, passing the in_args that have been copied to the retry context with the exception of ARG_CB(...) and ARG_CONTEXT(...) which are instead passed as the generated callback handler and the allocated context. ]*/ \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_043: [ The asynchronous retry wrapper shall store the result of async_function_name in async_function_result. ]*/ \
                        temp_async_function_result = async_function_name(retry_context->handle ASYNC_RETRY_WRAPPER_ARGS_IN_CALL(in_args), ASYNC_RETRY_WRAPPER_CALLBACK(async_function_name), retry_context); \
                        if (ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_PROXY(retry_sync_enums)) \
                        { \
                            if (retry_context->timeout_ms < UINT32_MAX) \
                            { \
                                double elapsed_time_ms = timer_global_get_elapsed_ms() - retry_context->start_time; \
                                if (elapsed_time_ms + retry_context->backoff > retry_context->timeout_ms) \
                                { \
                                    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_049: [ Before each retry of the function, If timeout_ms milliseconds have elapsed then {async_function_name}_async_retry_wrapper_with_timeout shall fail and return ASYNC_RETRY_WRAPPER_TIMEOUT. ]*/ \
                                    timed_out = true; \
                                    LogWarning("Retries for " MU_TOSTRING(async_function_name) " timed out after %lf ms (including %" PRIu32 " ms of backoff) (timeout time was %" PRIu32 " ms)", \
                                        elapsed_time_ms, retry_context->backoff, retry_context->timeout_ms); \
                                    break; \
                                } \
                            } \
                            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_044: [ Before each retry of the function, the asynchronous retry wrapper shall yield execution by a call to ThreadAPI_Sleep. ]*/ \
                            ThreadAPI_Sleep(retry_context->backoff); \
                            retry_context->backoff *= 2; \
                            if (retry_context->backoff > ASYNC_RETRY_WRAPPER_MAX_BACKOFF_MS) \
                            { \
                                retry_context->backoff = ASYNC_RETRY_WRAPPER_MAX_BACKOFF_MS; \
                            } \
                        } \
                        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_038: [ While async_function_name returns one of the values from RETRY_ON_SYNC(...), it shall be called again in a loop. ]*/ \
                    } while (ASYNC_RETRY_WRAPPER_RETRY_CONDITIONS_PROXY(retry_sync_enums)); \
                    *async_function_result = temp_async_function_result; \
                    if (timed_out) \
                    { \
                        async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_TIMEOUT; \
                    } \
                    else \
                    { \
                        if (temp_async_function_result != expected_return) \
                        { \
                            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_039: [ If async_function_name returns a value other than expected_return then the asynchronous retry wrapper shall fail and return ASYNC_RETRY_WRAPPER_CALL_ERROR. ]*/ \
                            LogInfo(MU_TOSTRING(async_function_name) " failed"); \
                            async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_CALL_ERROR; \
                        } \
                        else \
                        { \
                            /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_013: [ On success, the asynchronous retry wrapper shall return ASYNC_RETRY_WRAPPER_OK. ]*/ \
                            async_retry_wrapper_result = ASYNC_RETRY_WRAPPER_OK; \
                            goto all_ok; \
                        } \
                    } \
                    THANDLE_ASSIGN(THREADPOOL)(&retry_context->threadpool, NULL); \
                } \
                ASYNC_RETRY_WRAPPER_FREE_FAILED_PROXY(in_args); \
                free(retry_context); \
            } \
        } \
    all_ok: \
        return async_retry_wrapper_result; \
    } \
    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_002: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate an asynchronous retry wrapper with the declaration: ]*/ \
    IMPLEMENT_MOCKABLE_FUNCTION(, ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER(async_function_name), async_handle_type, async_handle, THANDLE(THREADPOOL), threadpool ASYNC_RETRY_WRAPPER_ARGS_IN_DECLARATION(in_args), return_type*, async_function_result) \
    { \
        return MU_C2(ASYNC_RETRY_WRAPPER(async_function_name), _internal)(async_handle, threadpool, UINT32_MAX ASYNC_RETRY_WRAPPER_ARGS_MAKE_STATIC_CALL(in_args), async_function_result); \
    } \
    /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_047: [ DEFINE_ASYNC_RETRY_WRAPPER shall generate an asynchronous retry wrapper that supports timeouts with the declaration: ]*/ \
    IMPLEMENT_MOCKABLE_FUNCTION(, ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(async_function_name), async_handle_type, async_handle, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms ASYNC_RETRY_WRAPPER_ARGS_IN_DECLARATION(in_args), return_type*, async_function_result) \
    { \
        /*Codes_SRS_ASYNC_RETRY_WRAPPER_42_050: [ {async_function_name}_async_retry_wrapper_with_timeout shall otherwise behave the same as {async_function_name}_async_retry_wrapper. ]*/ \
        return MU_C2(ASYNC_RETRY_WRAPPER(async_function_name), _internal)(async_handle, threadpool, timeout_ms ASYNC_RETRY_WRAPPER_ARGS_MAKE_STATIC_CALL(in_args), async_function_result); \
    }


/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_001: [ DECLARE_ASYNC_RETRY_WRAPPER shall expand to the function declaration: ]*/
/*Codes_SRS_ASYNC_RETRY_WRAPPER_42_046: [ DECLARE_ASYNC_RETRY_WRAPPER shall also expand to the function declaration: ]*/
#define DECLARE_ASYNC_RETRY_WRAPPER(async_handle_type, async_function_name, return_type, in_args) \
    MOCKABLE_FUNCTION(, ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER(async_function_name), async_handle_type, async_handle, THANDLE(THREADPOOL), threadpool ASYNC_RETRY_WRAPPER_ARGS_IN_DECLARATION(in_args), return_type*, async_function_result) \
    MOCKABLE_FUNCTION(, ASYNC_RETRY_WRAPPER_RESULT, ASYNC_RETRY_WRAPPER_WITH_TIMEOUT(async_function_name), async_handle_type, async_handle, THANDLE(THREADPOOL), threadpool, uint32_t, timeout_ms ASYNC_RETRY_WRAPPER_ARGS_IN_DECLARATION(in_args), return_type*, async_function_result) \

#define DEFINE_ASYNC_RETRY_WRAPPER(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    GENERATE_ASYNC_RETRY_WRAPPER_CONTEXT(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    DECLARE_ASYNC_RETRY_WRAPPER_CALLBACK(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    GENERATE_ASYNC_RETRY_WRAPPER_EXECUTE_RETRY_FUNCTION(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    GENERATE_ASYNC_RETRY_WRAPPER_CALLBACK(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums) \
    GENERATE_ASYNC_RETRY_WRAPPER_FUNCTION(async_handle_type, async_function_name, return_type, expected_return, in_args, out_args, retry_async_enums, retry_sync_enums)


#ifdef __cplusplus
}
#endif

#endif // ASYNC_RETRY_WRAPPER_H
