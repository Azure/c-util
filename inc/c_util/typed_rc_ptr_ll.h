// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TYPED_RC_PTR_LL_H
#define TYPED_RC_PTR_LL_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "c_logging/logger.h"

#include "umock_c/umock_c_prod.h"
#include "c_pal/thandle_ll.h"

#define TYPEDEF_TRC_PTR_LL_FREE_FUNC(C, T) typedef void (*MU_C2(C, _RC_PTR_FREE_FUNC))(T ptr);

#define TRC_PTR_STRUCT_NAME(T) struct MU_C2(T, _RC_PTR_TAG)
#define TRC_PTR_STRUCT(T) MU_C2(T, _RC_PTR)
#define TYPEDEF_TRC_PTR_STRUCT(C, T) \
typedef TRC_PTR_STRUCT_NAME(T) \
{ \
    T ptr; \
    MU_C2(C, _RC_PTR_FREE_FUNC) free_func; \
} TRC_PTR_STRUCT(T); \

#define TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(T) MU_C2(T, _rc_ptr_create_with_move_pointer)

#define TYPED_RC_PTR_LL_DECLARE(C, T) \
    TYPEDEF_TRC_PTR_LL_FREE_FUNC(C, T) /*Declare the free function type*/ \
    TYPEDEF_TRC_PTR_STRUCT(C, T) /*Declare the struct*/ \
    THANDLE_LL_TYPE_DECLARE(MU_C2(C, _RC_PTR), MU_C2(T, _RC_PTR)) /*Declare the THANDLE type for the struct*/ \
    MOCKABLE_FUNCTION(, THANDLE(MU_C2(T, _RC_PTR)), TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(C), T, ptr, MU_C2(C, _RC_PTR_FREE_FUNC), free_func); /*Declare the create_with_move_pointer function*/

#define DEFINE_TRC_PTR_LL_DISPOSE_FUNC(C, T) \
    static void MU_C2(C, _rc_ptr_dispose)( TRC_PTR_STRUCT(T)* context) \
    { \
        if (context->free_func != NULL) \
        { \
            context->free_func(context->ptr); \
        } \
    }

#define DEFINE_TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(C, T) \
    IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(MU_C2(T, _RC_PTR)), TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(C), T, ptr, MU_C2(C, _RC_PTR_FREE_FUNC), free_func) \
    { \
        THANDLE(MU_C2(T, _RC_PTR)) result = NULL; \
        if (ptr == NULL) \
        { \
            LogError("Invalid arguments: void* ptr=%p, " MU_TOSTRING(MU_C2(C, _rc_ptr_free_func)) " free_func = %p", ptr, free_func); \
        } \
        else \
        { \
            THANDLE(TRC_PTR_STRUCT(T)) thandle = THANDLE_MALLOC(TRC_PTR_STRUCT(T))(MU_C2(C, _rc_ptr_dispose)); \
            if (thandle == NULL) \
            { \
                LogError("Failure in THANDLE_MALLOC(" MU_TOSTRING(TRC_PTR_STRUCT(T)) ")(rc_ptr_dispose=%p)", MU_C2(C, _rc_ptr_dispose)); \
            } \
            else \
            { \
                TRC_PTR_STRUCT(T)* rc_ptr = THANDLE_GET_T(TRC_PTR_STRUCT(T))(thandle); \
                rc_ptr->ptr = ptr; \
                rc_ptr->free_func = free_func; \
                THANDLE_INITIALIZE_MOVE(TRC_PTR_STRUCT(T))(&result, &thandle); \
            }\
        }\
        return result;\
    }

#define TYPED_RC_PTR_LL_DEFINE(C, T) \
    DEFINE_TRC_PTR_LL_DISPOSE_FUNC(C, T) /*Define the free function*/ \
    THANDLE_LL_TYPE_DEFINE(MU_C2(C, _RC_PTR), MU_C2(T, _RC_PTR)) /*Define the THANDLE type for the struct*/ \
    DEFINE_TRC_PTR_LL_CREATE_WITH_MOVE_POINTER(C, T) /*Define the create_with_move_pointer function*/

#endif  /* TYPED_RC_PTR_LL_H */
