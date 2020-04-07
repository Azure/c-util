// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_H
#define THANDLE_H

#include <stdlib.h>
#include <stdint.h>

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "azure_c_util/xlogging.h"

#include "azure_c_util/containing_record.h"
#include "refcount_os.h"

#ifdef THANDLE_MALLOC_FUNCTION
    #ifndef THANDLE_FREE_FUNCTION
        #error THANDLE_MALLOC_FUNCTION and THANDLE_FREE_FUNCTION must be both defined or both not defined
    #else
        /*do nothing, the macros here will call whatever THANDLE_MALLOC_FUNCTION/THANDLE_FREE_FUNCTION expands to*/
    #endif
#else
    #ifdef THANDLE_FREE_FUNCTION
        #error THANDLE_MALLOC_FUNCTION and THANDLE_FREE_FUNCTION must be both defined or both not defined
    #else
        /*include the "stock" implementation*/
        #include "azure_c_util/thandle_stdlib.h"
    #endif
#endif

/*the incomplete unassignable type*/
#define THANDLE(T) MU_C2(CONST_P2_CONST_,T)

#define THANDLE_MACRO(T)                                \
    typedef const T* const THANDLE(T);

#define THANDLE_EXTRA_FIELDS(type) \
    volatile COUNT_TYPE, refCount, \
    void(*dispose)(type*) , \

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this is the name of the type that has T wrapped*/
#define THANDLE_WRAPPER_TYPE_NAME(T) MU_C2(T, _WRAPPER)

/*given a previous type T, THANDLE_MALLOC introduces a new name that mimics "malloc for T"*/
/*the new name is used to define the name of a static function that allocates memory*/
#define THANDLE_MALLOC(T) MU_C2(T,_MALLOC)

/*given a previous type T, THANDLE_MALLOC_WITH_EXTRA_SIZE introduces a new name that mimics "malloc for T, where T is a flex type"*/
/*the new name is used to define the name of a static function that allocates memory*/
#define THANDLE_MALLOC_WITH_EXTRA_SIZE(T) MU_C2(T,_MALLOC_WITH_EXTRA_SIZE)

/*given a previous type T, THANDLE_FREE introduces a new name that mimics "free for T"*/
/*the new name is used to define the name of a static function that frees the memory allocated by THANDLE_MALLOC/THANDLE_MALLOC_WITH_EXTRA_SIZE*/
#define THANDLE_FREE(T) MU_C2(T,_FREE)

/*given a previous type T, THANDLE_DEC_REF introduces a new name that mimics "dec_ref for T"*/
/*the new name is used to define the name of a static function that decrements the ref count of T and if 0, releases it*/
#define THANDLE_DEC_REF(T) MU_C2(T,_DEC_REF)

/*given a previous type T, THANDLE_INC_REF introduces a new name that mimics "inc_ref for T"*/
/*the new name is used to define the name of a static function that increments the ref count of T*/
#define THANDLE_INC_REF(T) MU_C2(T,_INC_REF)

/*given a previous type T, THANDLE_ASSIGN introduces a new name for a function that does T1=T2 (with inc/dec refs)*/
#define THANDLE_ASSIGN(T) MU_C2(T,_ASSIGN)

/*given a previous type T, THANDLE_INITIALIZE introduces a new name for a function that does T1=T2 (with inc ref, and considers T1 as uninitialized memory)*/
#define THANDLE_INITIALIZE(T) MU_C2(T,_INITIALIZE)

/*given a previous type T (and its THANDLE(T)), THANDLE_GET_T introduces a new name for a function that returns the T* from under the THANDLE(T)*/
#define THANDLE_GET_T(T) MU_C2(T,_GET_T)

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces THANDLE_MALLOC macro to create its wrapper, initialize refCount to 1, and remember the dispose function*/

#define THANDLE_MALLOC_MACRO(T) \
static T* THANDLE_MALLOC(T)(void(*dispose)(T*))                                                                                                                     \
{                                                                                                                                                                   \
    T* result;                                                                                                                                                      \
    /*Codes_SRS_THANDLE_02_013: [ THANDLE_MALLOC shall allocate memory. ]*/                                                                                         \
    THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = (THANDLE_WRAPPER_TYPE_NAME(T)*)THANDLE_MALLOC_FUNCTION(sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                       \
    if (handle_impl == NULL)                                                                                                                                        \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_015: [ If malloc fails then THANDLE_MALLOC shall fail and return NULL. ]*/                                                           \
        LogError("error in malloc(sizeof(THANDLE_WRAPPER_TYPE_NAME(" MU_TOSTRING(T) "))=%zu)",                                                                      \
            sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                                                                                                                  \
        result = NULL;                                                                                                                                              \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_014: [ THANDLE_MALLOC shall initialize the reference count to 1, store dispose and return a T* . ]*/                                 \
        handle_impl->dispose = dispose;                                                                                                                             \
        INIT_REF_VAR(handle_impl->refCount);                                                                                                                        \
        result = &(handle_impl->data);                                                                                                                              \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

#define THANDLE_MALLOC_WITH_EXTRA_SIZE_MACRO(T)                                                                                                                     \
static T* THANDLE_MALLOC_WITH_EXTRA_SIZE(T)(void(*dispose)(T*), size_t extra_size)                                                                                  \
{                                                                                                                                                                   \
    T* result;                                                                                                                                                      \
    /*Codes_SRS_THANDLE_02_019: [ If extra_size + sizeof(THANDLE_WRAPPER_TYPE_NAME(T)) would exceed SIZE_MAX then THANDLE_MALLOC_WITH_EXTRA_SIZE shall fail and return NULL. ]*/ \
    if (SIZE_MAX - sizeof(THANDLE_WRAPPER_TYPE_NAME(T)) < extra_size)                                                                                               \
    {                                                                                                                                                               \
        LogError("extra_size=%zu produces arithmetic overflows", extra_size);                                                                                       \
        result = NULL;                                                                                                                                              \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_020: [ THANDLE_MALLOC_WITH_EXTRA_SIZE shall allocate memory enough to hold T and extra_size. ]*/                                     \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = (THANDLE_WRAPPER_TYPE_NAME(T)*)THANDLE_MALLOC_FUNCTION(extra_size + sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));      \
        if (handle_impl == NULL)                                                                                                                                    \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_022: [ If malloc fails then THANDLE_MALLOC_WITH_EXTRA_SIZE shall fail and return NULL. ]*/                                       \
            LogError("error in malloc(sizeof(THANDLE_WRAPPER_TYPE_NAME(" MU_TOSTRING(T) "))=%zu)",                                                                  \
                sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                                                                                                              \
            result = NULL;                                                                                                                                          \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_021: [ THANDLE_MALLOC_WITH_EXTRA_SIZE shall initialize the reference count to 1, store dispose and return a T*. ]*/              \
            handle_impl->dispose = dispose;                                                                                                                         \
            INIT_REF_VAR(handle_impl->refCount);                                                                                                                    \
            result = &(handle_impl->data);                                                                                                                          \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces THANDLE_FREE macro to free all used resources*/
#define THANDLE_FREE_MACRO(T) \
static void THANDLE_FREE(T)(T* t)                                                                                                                                   \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_016: [ If t is NULL then THANDLE_FREE shall return. ]*/                                                                                  \
    if (t == NULL)                                                                                                                                                  \
    {                                                                                                                                                               \
        LogError("invalid arg " MU_TOSTRING(T) "* t=%p", t);                                                                                                        \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_017: [ THANDLE_FREE shall free the allocated memory by THANDLE_MALLOC. ]*/                                                           \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = containingRecord(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                        \
        THANDLE_FREE_FUNCTION(handle_impl);                                                                                                                         \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces THANDLE_DEC_REF macro to decrement the reference count*/
#define THANDLE_DEC_REF_MACRO(T) \
void THANDLE_DEC_REF(T)(THANDLE(T) t)                                                                                                                               \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_001: [ If t is NULL then THANDLE_DEC_REF shall return. ]*/                                                                               \
    if(t == NULL)                                                                                                                                                   \
    {                                                                                                                                                               \
        LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") t=%p", t);                                                                                           \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_002: [ THANDLE_DEC_REF shall decrement the ref count of t. ]*/                                                                       \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = containingRecord(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                        \
        if (DEC_REF_VAR(handle_impl->refCount) == DEC_RETURN_ZERO)                                                                                                  \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_003: [ If the ref count of t reaches 0 then THANDLE_DEC_REF shall call dispose (if not NULL) and free the used memory. ]*/       \
            if(handle_impl->dispose!=NULL)                                                                                                                          \
            {                                                                                                                                                       \
                handle_impl->dispose(&handle_impl->data);                                                                                                           \
            }                                                                                                                                                       \
            THANDLE_FREE(T)(&handle_impl->data);                                                                                                                    \
        }                                                                                                                                                           \
                                                                                                                                                                    \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces THANDLE_DEC_REF macro to increment the reference count*/
#define THANDLE_INC_REF_MACRO(T)                                                                                                                                    \
void THANDLE_INC_REF(T)(THANDLE(T) t)                                                                                                                               \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_004: [ If t is NULL then THANDLE_INC_REF shall return. ]*/                                                                               \
    if(t == NULL)                                                                                                                                                   \
    {                                                                                                                                                               \
        LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") t=%p", t);                                                                                           \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_005: [ THANDLE_INC_REF shall increment the reference count of t. ]*/                                                                 \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = containingRecord(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                        \
        INC_REF_VAR(handle_impl->refCount);                                                                                                                         \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces THANDLE_ASSIGN macro to assign a handle to another handle*/
#define THANDLE_ASSIGN_MACRO(T)                                                                                                                                     \
void THANDLE_ASSIGN(T)(THANDLE(T) * t1, THANDLE(T) t2 )                                                                                                             \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_006: [ If t1 is NULL then THANDLE_ASSIGN shall return. ]*/                                                                               \
    if(t1 == NULL)                                                                                                                                                  \
    {                                                                                                                                                               \
        LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") * t1=%p, THANDLE(" MU_TOSTRING(T) ") t2=%p", t1, t2 );                                               \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        if (*t1 == NULL)                                                                                                                                            \
        {                                                                                                                                                           \
            if (t2 == NULL)                                                                                                                                         \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_007: [ If *t1 is NULL and t2 is NULL then THANDLE_ASSIGN shall return. ]*/                                                   \
                /*so nothing to do, leave them as they are*/                                                                                                        \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_008: [ If *t1 is NULL and t2 is not NULL then THANDLE_ASSIGN shall increment the reference count of t2 and store t2 in *t1. ]*/ \
                THANDLE_INC_REF(T)(t2);                                                                                                                             \
                * (T const**)t1 = t2;                                                                                                                               \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            if (t2 == NULL)                                                                                                                                         \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_009: [ If *t1 is not NULL and t2 is NULL then THANDLE_ASSIGN shall decrement the reference count of *t1 and store NULL in *t1. ]*/ \
                THANDLE_DEC_REF(T)(*t1);                                                                                                                            \
                * (T const**)t1 = t2;                                                                                                                               \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_010: [ If *t1 is not NULL and t2 is not NULL then THANDLE_ASSIGN shall increment the reference count of t2, shall decrement the reference count of *t1 and store t2 in *t1. ]*/ \
                THANDLE_INC_REF(T)(t2);                                                                                                                             \
                THANDLE_DEC_REF(T)(*t1);                                                                                                                            \
                * (T const**)t1 = t2;                                                                                                                               \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces THANDLE_INITIALIZE macro to initialize a handle value*/
#define THANDLE_INITIALIZE_MACRO(T)                                                                                                                                 \
void THANDLE_INITIALIZE(T)(THANDLE(T) * lvalue, THANDLE(T) rvalue )                                                                                                 \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_011: [ If lvalue is NULL then THANDLE_INITIALIZE shall return. ]*/                                                                       \
    if(lvalue == NULL)                                                                                                                                              \
    {                                                                                                                                                               \
        LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") * lvalue=%p, THANDLE(" MU_TOSTRING(T) ") rvalue=%p", lvalue, rvalue );                               \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_018: [ If rvalue is NULL then THANDLE_INITIALIZE shall store NULL in *lvalue. ]*/                                                    \
        if(rvalue == NULL)                                                                                                                                          \
        {                                                                                                                                                           \
            LogInfo("THANDLE_INITIALIZE called with rvalue NULL");                                                                                                  \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_012: [ THANDLE_INITIALIZE shall increment the reference count of rvalue and store it in *lvalue. ]*/                             \
            THANDLE_INC_REF(T)(rvalue);                                                                                                                             \
        }                                                                                                                                                           \
        * (T const**)lvalue = rvalue;                                                                                                                               \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*if THANDLE(T) is previously defined, then this macro returns the T* from under the THANDLE(T) */
#define THANDLE_GET_T_MACRO(T)                                                                                                                                      \
static T* THANDLE_GET_T(T)(THANDLE(T) t)                                                                                                                            \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_023: [ If t is NULL then THANDLE_GET_T(T) shall return NULL. ]*/                                                                         \
    /*Codes_SRS_THANDLE_02_024: [ THANDLE_GET_T(T) shall return the same pointer as THANDLE_MALLOC/THANDLE_MALLOC_WITH_EXTRA_SIZE returned at the handle creation time. ]*/ \
    return (T*)t;                                                                                                                                                   \
}

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces a wrapper type that contains T (and other fields) and defines the functions of that type T*/
#define THANDLE_TYPE_DEFINE(T) \
    MU_DEFINE_STRUCT(THANDLE_WRAPPER_TYPE_NAME(T), THANDLE_EXTRA_FIELDS(T), T, data);                                                                               \
    THANDLE_MALLOC_MACRO(T)                                                                                                                                         \
    THANDLE_MALLOC_WITH_EXTRA_SIZE_MACRO(T)                                                                                                                         \
    THANDLE_FREE_MACRO(T)                                                                                                                                           \
    THANDLE_DEC_REF_MACRO(T)                                                                                                                                        \
    THANDLE_INC_REF_MACRO(T)                                                                                                                                        \
    THANDLE_ASSIGN_MACRO(T)                                                                                                                                         \
    THANDLE_INITIALIZE_MACRO(T)                                                                                                                                     \
    THANDLE_GET_T_MACRO(T)

/*macro to be used in headers*/                                                                                       \
/*introduces an incomplete type based on a MU_DEFINE_STRUCT(T...) previously defined;*/                               \
#define THANDLE_TYPE_DECLARE(T)                                                                                       \
    THANDLE_MACRO(T);                                                                                                 \
    MOCKABLE_FUNCTION(, void, THANDLE_DEC_REF(T), THANDLE(T), t);                                                     \
    MOCKABLE_FUNCTION(, void, THANDLE_INC_REF(T), THANDLE(T), t);                                                     \
    MOCKABLE_FUNCTION(, void, THANDLE_ASSIGN(T), THANDLE(T) *, t1, THANDLE(T), t2 );                                  \
    MOCKABLE_FUNCTION(, void, THANDLE_INITIALIZE(T), THANDLE(T) *, t1, THANDLE(T), t2 );                              \

#endif /*THANDLE_H*/

