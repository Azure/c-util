// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_LL_H
#define THANDLE_LL_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "c_logging/xlogging.h"

#include "c_pal/interlocked.h"

#include "c_util/containing_record.h"

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
        /*then use whatever malloc and free expand to*/
    #endif
#endif

/*the incomplete unassignable type*/
#define THANDLE(T) MU_C2(CONST_P2_CONST_,T)

#define THANDLE_MACRO(T)                                \
    typedef const T* const volatile THANDLE(T);

#define THANDLE_MAX_NAME_CHAR 32 /*maximum number of characters in a captured THANDLE type name*/

/*THANDLE carries a name field on debug builds only. Can be inspected with INSPECT function here in a debug environment.*/
#if defined(_DEBUG) || defined(DEBUG)
#define THANDLE_DEBUG_EXTRA_FIELDS_NAME char, name[THANDLE_MAX_NAME_CHAR],
/*destination is intended to be the field "name" from above*/
#define THANDLE_DEBUG_COPY_NAME(T, destination) (void)snprintf(destination, THANDLE_MAX_NAME_CHAR, "%s", MU_TOSTRING(T));
#else
#define THANDLE_DEBUG_EXTRA_FIELDS_NAME  /*well - nothing*/
#define THANDLE_DEBUG_COPY_NAME(T, destination) /*well - nothing*/
#endif

#define THANDLE_EXTRA_FIELDS(type) \
    THANDLE_DEBUG_EXTRA_FIELDS_NAME \
    volatile_atomic int32_t, refCount, \
    THANDLE_LL_FREE_FUNCTION_POINTER_T, free_function, \
    void(*dispose)(type*) , \


/*thandle_ll_malloc field in the structure below is the name of the function that allocates memory (has the same signature as malloc from stdlib*/
/*thandle_ll_malloc can come from 3 sources from lowest to highest priority:
1. globally: when the user defines it prior to THANDLE_LL_TYPE_DEFINE with #define THANDLE_MALLOC_FUNCTION (largely maintained momentarily due to backwards compatibility, soon to be deprecated and removed)
2. at type definition time: when the user uses THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS
3. at instance creation time: when the user decides to use at instance level of THANDLE specific allocation function by calling THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS / THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS / THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS
*/

typedef void* (*THANDLE_LL_MALLOC_FUNCTION_POINTER_T)(size_t);
typedef void* (*THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T)(size_t, size_t, size_t);
typedef void (*THANDLE_LL_FREE_FUNCTION_POINTER_T)(void*);

/*given a previous type T, THANDLE_LL_TYPE_STRUCT_TYPE(_TAG) introduces a name for a structure that captures 3 pointers(malloc, malloc_flex, free) for the type T*/
#define THANDLE_LL_TYPE_STRUCT_TYPE_TAG(T) MU_C3(THANDLE_LL_TYPE_STRUCT_TYPE_, T, _TAG)
#define THANDLE_LL_TYPE_STRUCT_TYPE(T) MU_C2(THANDLE_LL_TYPE_STRUCT_TYPE_, T)

#define THANDLE_LL_TYPE_STRUCT_TYPE_DEFINE(T)                                       \
typedef struct THANDLE_LL_TYPE_STRUCT_TYPE_TAG(T)                                   \
{                                                                                   \
    THANDLE_LL_MALLOC_FUNCTION_POINTER_T thandle_ll_malloc;                         \
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T thandle_ll_malloc_flex;               \
    THANDLE_LL_FREE_FUNCTION_POINTER_T thandle_ll_free;                             \
    /*TO DO: move here THANDLE_DEBUG_EXTRA_FIELDS_NAME*/                            \
}THANDLE_LL_TYPE_STRUCT_TYPE(T);                                                    \

/*given a previous type T, this is the name of the type that has T wrapped*/
#define THANDLE_WRAPPER_TYPE_NAME(T) MU_C2(T, _WRAPPER)

/*given a previous type T, THANDLE_MALLOC introduces a new name that mimics "malloc for T"*/
/*the new name is used to define the name of a static function that allocates memory*/
#define THANDLE_MALLOC(C) MU_C2(THANDLE_MALLOC_, C)

/*given a previous type T, THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS introduces a new name that mimics "malloc for T" but uses the specified malloc/free functions*/
/*the new name is used to define the name of a static function that allocates memory using specified malloc functions*/
#define THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS(C) MU_C2(THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_, C)

/*captures THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS' malloc_function, malloc_flex_function, malloc_free_function parameters for type T*/
#define THANDLE_LL_TYPE_STRUCT_VAR(T) MU_C2(THANDLE_LL_TYPE_FROM_MACRO_, T)
#define THANDLE_LL_TYPE_STRUCT_VAR_DEFINE(T, malloc_function, malloc_flex_function, malloc_free_function) static const THANDLE_LL_TYPE_STRUCT_TYPE(T) THANDLE_LL_TYPE_STRUCT_VAR(T) = {malloc_function, malloc_flex_function, malloc_free_function};

/*given a previous type T, THANDLE_INSPECT introduces a new name that if a function that returns the THANDLE_WRAPPER_TYPE_NAME(T) - useful in debugging. The function has no side-effects.*/
#define THANDLE_INSPECT(T) MU_C2(T,_INSPECT)

/*given a previous type T, THANDLE_MALLOC_WITH_EXTRA_SIZE introduces a new name that mimics "malloc for T, where T is a flex type"*/
/*the new name is used to define the name of a static function that allocates memory*/
#define THANDLE_MALLOC_WITH_EXTRA_SIZE(T) MU_C2(T,_MALLOC_WITH_EXTRA_SIZE)

/*given a previous type T, THANDLE_CREATE_FROM_CONTENT introduces a new name for the function that makes of copy of T and returns a THANDLE to the caller*/
#define THANDLE_CREATE_FROM_CONTENT(T) MU_C2(T,_CREATE_FROM_CONTENT)

/*given a previous type T (usually a struct with a flexible array member), THANDLE_CREATE_FROM_CONTENT_FLEX introduces a new name for the function that makes of copy of T and returns a THANDLE to the caller*/
#define THANDLE_CREATE_FROM_CONTENT_FLEX(T) MU_C2(T,_CREATE_FROM_CONTENT_FLEX)

/*given a previous type T (usually a struct with a flexible array member), THANDLE_GET_SIZEOF introduces a new name for a user function that returns the sizeof of T */
#define THANDLE_GET_SIZEOF(T) MU_C2(T,_GET_SIZE_OF)

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

/*given a previous type T (and its THANDLE(T)), THANDLE_MOVE introduces a new name for a function that moves a handle to another handle. Move does not increment the ref count, and NULLs the source*/
#define THANDLE_MOVE(T) MU_C2(T,_MOVE)

/*given a previous type T (and its THANDLE(T)), THANDLE_INITIALIZE_MOVE introduces a new name for a function that moves a handle to another handle.
INITIALIZE_MOVE does not increment the ref count, and NULLs the source.
INITIALIZE_MOVE assumes that destination is not initialized and thus it does not decrement the destination ref count */
#define THANDLE_INITIALIZE_MOVE(T) MU_C2(T,_INITIALIZE_MOVE)

/*THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS returns a new T* using malloc_function (if not NULL) or THANDLE_LL_TYPE_STRUCT_VAR (if not NULL) or THANDLE_MALLOC_FUNCTION*/
#define THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_MACRO(C, T) \
static T* THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS(C)(void(*dispose)(T*), THANDLE_LL_MALLOC_FUNCTION_POINTER_T malloc_function, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function) \
{                                                                                                                                                                   \
    T* result;                                                                                                                                                      \
    THANDLE_LL_MALLOC_FUNCTION_POINTER_T malloc_function_used;                                                                                                      \
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;                                                                                                          \
    if(malloc_function != NULL)                                                                                                                                     \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_039: [ If malloc_function is not NULL then malloc_function and free_function shall be used to allocate/free memory. ]*/              \
        malloc_function_used = malloc_function;                                                                                                                     \
        free_function_used = free_function;                                                                                                                         \
    }                                                                                                                                                               \
    else if(THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_malloc!=NULL)                                                                                                  \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_040: [ If malloc_function from THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS is not NULL then THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS's malloc_function and free_function shall be used to allocate/free memory. ]*/ \
        malloc_function_used = THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_malloc;                                                                                     \
        free_function_used = THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_free;                                                                                         \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_041: [ If THANDLE_MALLOC_FUNCTION is not NULL then THANDLE_MALLOC_FUNCTION / THANDLE_FREE_FUNCTION shall be used to allocate/free memory. ]*/ \
        malloc_function_used = THANDLE_MALLOC_FUNCTION;                                                                                                             \
        free_function_used = THANDLE_FREE_FUNCTION;                                                                                                                 \
    }                                                                                                                                                               \
    if((malloc_function_used == NULL)  || (free_function_used == NULL))                                                                                             \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_042: [ If no function can be found to allocate/free memory then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall return NULL. ]*/          \
        LogError("something is wrong with code config, cannot find a malloc/free function");                                                                        \
        result = NULL;                                                                                                                                              \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_043: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/                                                               \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = (THANDLE_WRAPPER_TYPE_NAME(T)*)malloc_function_used(sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                      \
        if (handle_impl == NULL)                                                                                                                                    \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_045: [ If allocating memory fails then THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/                      \
            LogError("error in malloc_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME(" MU_TOSTRING(T) "))=%zu)",                                                 \
                malloc_function_used, sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                                                                                        \
            result = NULL;                                                                                                                                          \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_044: [ THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function and return a T* ]*/ \
            THANDLE_DEBUG_COPY_NAME(T, handle_impl->name);                                                                                                          \
            handle_impl->dispose = dispose;                                                                                                                         \
            handle_impl->free_function = free_function_used;                                                                                                        \
            (void)interlocked_exchange(&handle_impl->refCount,1);                                                                                                   \
            result = &(handle_impl->data);                                                                                                                          \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

/*given a previous type T, this introduces THANDLE_MALLOC macro to create its wrapper, initialize refCount to 1, and remember the dispose function*/
#define THANDLE_LL_MALLOC_MACRO(C, T) \
static T* THANDLE_MALLOC(C)(void(*dispose)(T*))                                                                                                                     \
{                                                                                                                                                                   \
    return THANDLE_MALLOC_WITH_MALLOC_FUNCTIONS(C)(dispose, NULL, NULL);                                                                                            \
}                                                                                                                                                                   \

/*this is only useful during debugging: from a THANDLE(T) it returns THANDLE_WRAPPER_TYPE_NAME(T) - which can be viewed in debugger - useful for seeing refCount.
Example: write REAL_BSDL_LOG_STRUCTURE_INSPECT(temp) in the Visual Studio watch window, where REAL_BSDL_LOG_STRUCTURE is a previously THANDLE'd type               */
#define THANDLE_LL_INSPECT_MACRO(C, T) \
const THANDLE_WRAPPER_TYPE_NAME(T)* const THANDLE_INSPECT(C)(THANDLE(T) t)                                                                                          \
{                                                                                                                                                                   \
    return CONTAINING_RECORD(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                                                                \
}                                                                                                                                                                   \

#define THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS(C) MU_C2(THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO_WITH_MALLOC_FUNCTIONS_, C)

/*THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS returns a new T* with extra size using malloc_function (if not NULL) or THANDLE_LL_TYPE_STRUCT_VAR (if not NULL) or THANDLE_MALLOC_FUNCTION*/
#define THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS_MACRO(C, T)                                                     \
static T* THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS(C)(void(*dispose)(T*), size_t extra_size, THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function)        \
{                                                                                                                                                                   \
    T* result;                                                                                                                                                      \
    THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function_used;                                                                                            \
    THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;                                                                                                          \
    if(malloc_flex_function != NULL)                                                                                                                                \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_046: [ If malloc_flex_function is not NULL then malloc_flex_function and free_function shall be used to allocate memory. ]*/         \
        malloc_flex_function_used = malloc_flex_function;                                                                                                           \
        free_function_used = free_function;                                                                                                                         \
    }                                                                                                                                                               \
    else if(THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_malloc_flex!=NULL)                                                                                             \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_047: [ If malloc_flex_function from THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS is not NULL then THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS's malloc_flex_function and free_function shall be used to allocate/free memory. ]*/ \
        malloc_flex_function_used = THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_malloc_flex;                                                                           \
        free_function_used = THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_free;                                                                                         \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_048: [ If THANDLE_MALLOC_FLEX_FUNCTION is not NULL then THANDLE_MALLOC_FLEX_FUNCTION / THANDLE_FREE_FUNCTION shall be used to allocate/free memory. ]*/ \
        malloc_flex_function_used = THANDLE_MALLOC_FLEX_FUNCTION;                                                                                                   \
        free_function_used = THANDLE_FREE_FUNCTION;                                                                                                                 \
    }                                                                                                                                                               \
    if((malloc_flex_function_used == NULL)  || (free_function_used == NULL))                                                                                        \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_049: [ If no function can be found to allocate/free memory then THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/ \
        LogError("something is wrong with code config, cannot find a malloc/free function");                                                                        \
        result = NULL;                                                                                                                                              \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_050: [ THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/                                               \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = malloc_flex_function_used(sizeof(THANDLE_WRAPPER_TYPE_NAME(T)), extra_size, 1);                                 \
        if (handle_impl == NULL)                                                                                                                                    \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_052: [ If allocating memory fails then THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/      \
            LogError("error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME(" MU_TOSTRING(T) "))=%zu)",                                            \
                malloc_flex_function_used, sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                                                                                   \
            result = NULL;                                                                                                                                          \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_051: [ THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS shall initialize the reference count to 1, store dispose and free_function succeed and return a non-NULL T*. ]*/ \
            THANDLE_DEBUG_COPY_NAME(T, handle_impl->name);                                                                                                          \
            handle_impl->dispose = dispose;                                                                                                                         \
            handle_impl->free_function = free_function_used;                                                                                                        \
            (void)interlocked_exchange(&handle_impl->refCount,1);                                                                                                   \
            result = &(handle_impl->data);                                                                                                                          \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

#define THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO(C, T)                                                                                                               \
static T* THANDLE_MALLOC_WITH_EXTRA_SIZE(C)(void(*dispose)(T*), size_t extra_size)                                                                                  \
{                                                                                                                                                                   \
    return THANDLE_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS(C)(dispose, extra_size, NULL, NULL);                                                             \
}                                                                                                                                                                   \

#define THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS(C) MU_C2(THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_, C)

/*THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS returns a new T* from some content using malloc_flex_function (if not NULL) or THANDLE_LL_TYPE_STRUCT_VAR (if not NULL) or THANDLE_MALLOC_FUNCTION*/
#define THANDLE_LL_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_MACRO(C, T)                                                                                                                   \
static T* THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS(C)(const T* source, void(*dispose)(T*), int(*copy)(T* destination, const T* source), size_t(*get_sizeof)(const T* source), THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function) \
{                                                                                                                                                                   \
    T* result;                                                                                                                                                      \
    if (                                                                                                                                                            \
        /*Codes_SRS_THANDLE_02_053: [ If source is NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/                 \
        (source == NULL) ||                                                                                                                                         \
        /*Codes_SRS_THANDLE_02_054: [ If get_sizeof is NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/             \
        (get_sizeof == NULL)                                                                                                                                        \
        )                                                                                                                                                           \
    {                                                                                                                                                               \
        LogError("invalid arguments const T* source=%p, void(*dispose)(T*)=%p, int(*copy)(T* destination, const T* source)=%p, size_t(*get_sizeof)(const T* source)=%p, THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function=%p, THANDLE_LL_FREE_FUNCTION_POINTER_T free_function=%p", \
            source, dispose, copy, get_sizeof, malloc_flex_function, free_function);                                                                                \
        result = NULL;                                                                                                                                              \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        THANDLE_LL_MALLOC_FLEX_FUNCTION_POINTER_T malloc_flex_function_used;                                                                                        \
        THANDLE_LL_FREE_FUNCTION_POINTER_T free_function_used;                                                                                                      \
        if(malloc_flex_function != NULL)                                                                                                                            \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_056: [ If malloc_flex_function is not NULL then malloc_flex_function and free_function shall be used to allocate memory. ]*/     \
            malloc_flex_function_used = malloc_flex_function;                                                                                                       \
            free_function_used = free_function;                                                                                                                     \
        }                                                                                                                                                           \
        else if(THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_malloc_flex!=NULL)                                                                                         \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_057: [ If malloc_flex_function from THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS is not NULL then THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS's malloc_flex_function and free_function shall be used to allocate/free memory. ]*/ \
            malloc_flex_function_used = THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_malloc_flex;                                                                       \
            free_function_used = THANDLE_LL_TYPE_STRUCT_VAR(T).thandle_ll_free;                                                                                     \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_058: [ If THANDLE_MALLOC_FLEX_FUNCTION is not NULL then THANDLE_MALLOC_FLEX_FUNCTION / THANDLE_FREE_FUNCTION shall be used to allocate/free memory. ]*/ \
            malloc_flex_function_used = THANDLE_MALLOC_FLEX_FUNCTION;                                                                                               \
            free_function_used = THANDLE_FREE_FUNCTION;                                                                                                             \
        }                                                                                                                                                           \
        if((malloc_flex_function_used == NULL) || (free_function_used == NULL))                                                                                     \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_059: [ If no function can be found to allocate/free memory then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/ \
            LogError("something is wrong with code config, cannot find a malloc/free function");                                                                    \
            result = NULL;                                                                                                                                          \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_064: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall call get_sizeof to get the needed size to store T ]*/        \
            size_t sizeof_source = get_sizeof(source);                                                                                                              \
            /*Codes_SRS_THANDLE_02_060: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall allocate memory. ]*/                                         \
            THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = (THANDLE_WRAPPER_TYPE_NAME(T)*)malloc_flex_function_used(sizeof(THANDLE_WRAPPER_TYPE_NAME(T)) - sizeof(T), 1, sizeof_source); \
            if (handle_impl == NULL)                                                                                                                                        \
            {                                                                                                                                                               \
                /*Codes_SRS_THANDLE_02_063: [ If there are any failures then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/         \
                LogError("error in malloc_flex_function_used=%p(sizeof(THANDLE_WRAPPER_TYPE_NAME(T))=%zu - sizeof(T)=%zu, 1, sizeof_source=%zu)",                           \
                    malloc_flex_function_used, sizeof(THANDLE_WRAPPER_TYPE_NAME(T)), sizeof(T), sizeof_source);                                                             \
                result = NULL;                                                                                                                                              \
            }                                                                                                                                                               \
            else                                                                                                                                                            \
            {                                                                                                                                                               \
                if (copy==NULL)                                                                                                                                             \
                {                                                                                                                                                           \
                    /*Codes_SRS_THANDLE_02_055: [ If copy is NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall memcpy the content of source in allocated memory. ]*/ \
                    THANDLE_DEBUG_COPY_NAME(T, handle_impl->name);                                                                                                          \
                    (void)memcpy(&(handle_impl->data), source, sizeof_source);                                                                                              \
                    handle_impl->dispose = dispose;                                                                                                                         \
                    handle_impl->free_function = free_function_used;                                                                                                        \
                    /*Codes_SRS_THANDLE_02_062: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall initialize the ref count to 1, succeed and return a non-NULL value. ]*/  \
                    (void)interlocked_exchange(&handle_impl->refCount,1);                                                                                                   \
                    result = &(handle_impl->data);                                                                                                                          \
                }                                                                                                                                                           \
                else                                                                                                                                                        \
                {                                                                                                                                                           \
                    /*Codes_SRS_THANDLE_02_061: [ If copy is not NULL then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall call copy to copy source into allocated memory. ]*/ \
                    if (copy(&handle_impl->data, source) != 0)                                                                                                              \
                    {                                                                                                                                                       \
                        /*Codes_SRS_THANDLE_02_063: [ If there are any failures then THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall fail and return NULL. ]*/ \
                        LogError("failure in copy(&handle_impl->data=%p, source=%p)", &handle_impl->data, source);                                                          \
                        free_function_used(handle_impl);                                                                                                                    \
                        result = NULL;                                                                                                                                      \
                    }                                                                                                                                                       \
                    else                                                                                                                                                    \
                    {                                                                                                                                                       \
                        handle_impl->dispose = dispose;                                                                                                                     \
                        handle_impl->free_function = free_function_used;                                                                                                    \
                        /*Codes_SRS_THANDLE_02_062: [ THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS shall initialize the ref count to 1, succeed and return a non-NULL value. ]*/  \
                        (void)interlocked_exchange(&handle_impl->refCount,1);                                                                                               \
                        result = &(handle_impl->data);                                                                                                                      \
                    }                                                                                                                                                       \
                }                                                                                                                                                           \
            }                                                                                                                                                               \
        }                                                                                                                                                                   \
    }                                                                                                                                                                       \
    return result;                                                                                                                                                          \
}

#define THANDLE_LL_CREATE_FROM_CONTENT_FLEX_MACRO(C, T)                                                                                                                         \
static T* THANDLE_CREATE_FROM_CONTENT_FLEX(C)(const T* source, void(*dispose)(T*), int(*copy)(T* destination, const T* source), size_t(*get_sizeof)(const T* source))   \
{                                                                                                                                                                               \
    return THANDLE_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS(C)(source, dispose, copy, get_sizeof, NULL, NULL);                                                            \
}

#define THANDLE_LL_CREATE_FROM_CONTENT_MACRO(C, T)                                                                                                                  \
static size_t THANDLE_GET_SIZEOF(C)(const T* t)                                                                                                                     \
{                                                                                                                                                                   \
    return sizeof(*t);                                                                                                                                              \
}                                                                                                                                                                   \
static T* THANDLE_CREATE_FROM_CONTENT(C)(const T* source, void(*dispose)(T*), int(*copy)(T* destination, const T* source))                                  \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_032: [ THANDLE_CREATE_FROM_CONTENT_FLEX returns what THANDLE_CREATE_FROM_CONTENT_FLEX(T)(source, dispose, copy, THANDLE_GET_SIZEOF(T)); returns. ]*/ \
    return THANDLE_CREATE_FROM_CONTENT_FLEX(C)(source, dispose, copy, THANDLE_GET_SIZEOF(C));                                                                       \
}                                                                                                                                                                   \

/*given a previous type T, this introduces THANDLE_FREE macro to free all used resources*/
#define THANDLE_LL_FREE_MACRO(C, T) \
static void THANDLE_FREE(C)(T* t)                                                                                                                                   \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_016: [ If t is NULL then THANDLE_FREE shall return. ]*/                                                                                  \
    if (t == NULL)                                                                                                                                                  \
    {                                                                                                                                                               \
        LogError("invalid arg " MU_TOSTRING(T) "* t=%p", t);                                                                                                        \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /*Codes_SRS_THANDLE_02_017: [ THANDLE_FREE shall free the allocated memory by THANDLE_MALLOC. ]*/                                                           \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = CONTAINING_RECORD(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                       \
        handle_impl->free_function(handle_impl);                                                                                                                    \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T, this introduces THANDLE_DEC_REF macro to decrement the reference count*/
#define THANDLE_LL_DEC_REF_MACRO(C, T) \
static void THANDLE_DEC_REF(C)(THANDLE(T) t)                                                                                                                               \
{                                                                                                                                                                   \
    THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = CONTAINING_RECORD(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                       \
    if (interlocked_decrement(&handle_impl->refCount) == 0)                                                                                                     \
    {                                                                                                                                                           \
        if (handle_impl->dispose!=NULL)                                                                                                                         \
        {                                                                                                                                                       \
            handle_impl->dispose(&handle_impl->data);                                                                                                           \
        }                                                                                                                                                       \
        handle_impl->free_function(handle_impl);                                                                                                                \
    }                                                                                                                                                           \
}                                                                                                                                                                   \

/*given a previous type T, this introduces THANDLE_INC_REF macro to increment the reference count*/
#define THANDLE_LL_INC_REF_MACRO(C, T)                                                                                                                              \
static void THANDLE_INC_REF(C)(THANDLE(T) t)                                                                                                                               \
{                                                                                                                                                                   \
    THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = CONTAINING_RECORD(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                       \
    (void)interlocked_increment(&handle_impl->refCount);                                                                                                        \
}                                                                                                                                                                   \

/*given a previous type T, this introduces THANDLE_ASSIGN macro to assign a handle to another handle*/
#define THANDLE_LL_ASSIGN_MACRO(C, T)                                                                                                                               \
void THANDLE_ASSIGN(C)(THANDLE(T) * t1, THANDLE(T) t2)                                                                                                              \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_006: [ If t1 is NULL then THANDLE_ASSIGN shall return. ]*/                                                                               \
    if (t1 == NULL)                                                                                                                                                 \
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
                THANDLE_INC_REF(C)(t2);                                                                                                                             \
                *(T const**)t1 = t2;                                                                                                                                \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            if (t2 == NULL)                                                                                                                                         \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_009: [ If *t1 is not NULL and t2 is NULL then THANDLE_ASSIGN shall decrement the reference count of *t1 and store NULL in *t1. ]*/ \
                THANDLE_DEC_REF(C)(*t1);                                                                                                                            \
                *(T const**)t1 = t2;                                                                                                                                \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_010: [ If *t1 is not NULL and t2 is not NULL then THANDLE_ASSIGN shall increment the reference count of t2, shall decrement the reference count of *t1 and store t2 in *t1. ]*/ \
                THANDLE_INC_REF(C)(t2);                                                                                                                             \
                THANDLE_DEC_REF(C)(*t1);                                                                                                                            \
                *(T const**)t1 = t2;                                                                                                                                \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T, this introduces THANDLE_INITIALIZE macro to initialize a handle value*/
#define THANDLE_LL_INITIALIZE_MACRO(C, T)                                                                                                                           \
void THANDLE_INITIALIZE(C)(THANDLE(T) * lvalue, THANDLE(T) rvalue)                                                                                                  \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_011: [ If lvalue is NULL then THANDLE_INITIALIZE shall return. ]*/                                                                       \
    if (lvalue == NULL)                                                                                                                                             \
    {                                                                                                                                                               \
        LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") * lvalue=%p, THANDLE(" MU_TOSTRING(T) ") rvalue=%p", lvalue, rvalue );                               \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        if (rvalue == NULL)                                                                                                                                         \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_018: [ If rvalue is NULL then THANDLE_INITIALIZE shall store NULL in *lvalue. ]*/                                                \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_02_012: [ THANDLE_INITIALIZE shall increment the reference count of rvalue and store it in *lvalue. ]*/                             \
            THANDLE_INC_REF(C)(rvalue);                                                                                                                             \
        }                                                                                                                                                           \
        *(T const**)lvalue = rvalue;                                                                                                                                \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*if THANDLE(T) is previously defined, then this macro returns the T* from under the THANDLE(T) */
#define THANDLE_LL_GET_T_MACRO(C, T)                                                                                                                                \
static T* THANDLE_GET_T(C)(THANDLE(T) t)                                                                                                                            \
{                                                                                                                                                                   \
    /*Codes_SRS_THANDLE_02_023: [ If t is NULL then THANDLE_GET_T(T) shall return NULL. ]*/                                                                         \
    /*Codes_SRS_THANDLE_02_024: [ THANDLE_GET_T(T) shall return the same pointer as THANDLE_MALLOC/THANDLE_MALLOC_WITH_EXTRA_SIZE returned at the handle creation time. ]*/ \
    return (T*)t;                                                                                                                                                   \
}

/*given a previous type T, this introduces THANDLE_MOVE macro to move a handle (*t1=t2, *t2=NULL)*/
#define THANDLE_LL_MOVE_MACRO(C, T)                                                                                                                                 \
void THANDLE_MOVE(C)(THANDLE(T) * t1, THANDLE(T) * t2)                                                                                                              \
{                                                                                                                                                                   \
    if (                                                                                                                                                            \
        /*Codes_SRS_THANDLE_02_033: [ If t1 is NULL then THANDLE_MOVE shall return. ]*/                                                                             \
        (t1 == NULL) ||                                                                                                                                             \
        /*Codes_SRS_THANDLE_02_034: [ If t2 is NULL then THANDLE_MOVE shall return. ]*/                                                                             \
        (t2 == NULL)                                                                                                                                                \
        )                                                                                                                                                           \
    {                                                                                                                                                               \
        LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") * t1=%p, THANDLE(" MU_TOSTRING(T) ") t2=%p", t1, t2 );                                               \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        if (*t1 == NULL)                                                                                                                                            \
        {                                                                                                                                                           \
            if (*t2 == NULL)                                                                                                                                        \
            {                                                                                                                                                       \
                /*so nothing to do, leave them as they are*/                                                                                                        \
                /*Codes_SRS_THANDLE_02_035: [ If *t1 is NULL and *t2 is NULL then THANDLE_MOVE shall return. ]*/                                                    \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_036: [ If *t1 is NULL and *t2 is not NULL then THANDLE_MOVE shall move *t2 under t1, set *t2 to NULL and return. ]*/         \
                *(T const**)t1 = *t2;                                                                                                                               \
                *(T const**)t2 = NULL;                                                                                                                              \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            if (*t2 == NULL)                                                                                                                                        \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_037: [ If *t1 is not NULL and *t2 is NULL then THANDLE_MOVE shall THANDLE_DEC_REF *t1, set *t1 to NULL and return. ]*/       \
                THANDLE_DEC_REF(C)(*t1);                                                                                                                            \
                *(T const**)t1 = NULL;                                                                                                                              \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                /*Codes_SRS_THANDLE_02_038: [ If *t1 is not NULL and *t2 is not NULL then THANDLE_MOVE shall THANDLE_DEC_REF *t1, set *t1 to *t2, set *t2 to NULL and return. ]*/ \
                THANDLE_DEC_REF(C)(*t1);                                                                                                                            \
                *(T const**)t1 = *t2;                                                                                                                               \
                *(T const**)t2 = NULL;                                                                                                                              \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T, this introduces THANDLE_INITIALIZE_MOVE_MACRO macro to move a handle (*t1=t2, *t2=NULL)*/
#define THANDLE_LL_INITIALIZE_MOVE_MACRO(C, T)                                                                                                                      \
void THANDLE_INITIALIZE_MOVE(C)(THANDLE(T) * t1, THANDLE(T) * t2)                                                                                                   \
{                                                                                                                                                                   \
    if (                                                                                                                                                            \
        /*Codes_SRS_THANDLE_01_001: [ If t1 is NULL then THANDLE_INITIALIZE_MOVE shall return. ]*/                                                                  \
        (t1 == NULL) ||                                                                                                                                             \
        /*Codes_SRS_THANDLE_01_002: [ If t2 is NULL then THANDLE_INITIALIZE_MOVE shall return. ]*/                                                                  \
        (t2 == NULL)                                                                                                                                                \
        )                                                                                                                                                           \
    {                                                                                                                                                               \
        LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") * t1=%p, THANDLE(" MU_TOSTRING(T) ") t2=%p", t1, t2 );                                               \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        if (*t2 == NULL)                                                                                                                                            \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_01_003: [ If *t2 is NULL then THANDLE_INITIALIZE_MOVE shall set *t1 to NULL and return. ]*/                                         \
            *(T const**)t1 = NULL;                                                                                                                                  \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /*Codes_SRS_THANDLE_01_004: [ If *t2 is not NULL then THANDLE_INITIALIZE_MOVE shall set *t1 to *t2, set *t2 to NULL and return. ]*/                     \
            *(T const**)t1 = *t2;                                                                                                                                   \
            *(T const**)t2 = NULL;                                                                                                                                  \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*given a previous type T, this introduces a wrapper type that contains T (and other fields) and defines the functions of that type T*/
#define THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(C, T, malloc_function, malloc_flex_function, malloc_free_function)                                                 \
    THANDLE_LL_TYPE_STRUCT_TYPE_DEFINE(T)                                                                                                                               \
    THANDLE_LL_TYPE_STRUCT_VAR_DEFINE(T, malloc_function, malloc_flex_function, malloc_free_function)                                                                   \
                                                                                                                                                                        \
    MU_DEFINE_STRUCT(THANDLE_WRAPPER_TYPE_NAME(T), THANDLE_EXTRA_FIELDS(T), T, data);                                                                                   \
                                                                                                                                                                        \
    THANDLE_LL_MALLOC_WITH_MALLOC_FUNCTIONS_MACRO(C, T)                                                                                                                 \
    THANDLE_LL_MALLOC_MACRO(C, T)                                                                                                                                       \
                                                                                                                                                                        \
    THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_WITH_MALLOC_FUNCTIONS_MACRO(C, T)                                                                                                 \
    THANDLE_LL_MALLOC_WITH_EXTRA_SIZE_MACRO(C, T)                                                                                                                       \
                                                                                                                                                                        \
    THANDLE_LL_CREATE_FROM_CONTENT_FLEX_WITH_MALLOC_FUNCTIONS_MACRO(C, T)                                                                                               \
    THANDLE_LL_CREATE_FROM_CONTENT_FLEX_MACRO(C, T)                                                                                                                     \
                                                                                                                                                                        \
    THANDLE_LL_CREATE_FROM_CONTENT_MACRO(C, T)                                                                                                                          \
                                                                                                                                                                        \
    THANDLE_LL_FREE_MACRO(C, T)                                                                                                                                         \
    THANDLE_LL_DEC_REF_MACRO(C, T)                                                                                                                                      \
    THANDLE_LL_INC_REF_MACRO(C, T)                                                                                                                                      \
    THANDLE_LL_ASSIGN_MACRO(C, T)                                                                                                                                       \
    THANDLE_LL_INITIALIZE_MACRO(C, T)                                                                                                                                   \
    THANDLE_LL_GET_T_MACRO(C, T)                                                                                                                                        \
    THANDLE_LL_INSPECT_MACRO(C, T)                                                                                                                                      \
    THANDLE_LL_MOVE_MACRO(C, T)                                                                                                                                         \
    THANDLE_LL_INITIALIZE_MOVE_MACRO(C, T)                                                                                                                              \


/*given a previous type T, this introduces a wrapper type that contains T (and other fields) and defines the functions of that type T*/
#define THANDLE_LL_TYPE_DEFINE(C,T) THANDLE_LL_TYPE_DEFINE_WITH_MALLOC_FUNCTIONS(C, T, NULL, NULL, NULL)

/*macro to be used in headers*/                                                                                       \
/*introduces an incomplete type based on a MU_DEFINE_STRUCT(T...) previously defined;*/                               \
#define THANDLE_LL_TYPE_DECLARE(C,T)                                                                                  \
    THANDLE_MACRO(T);                                                                                                 \
    MOCKABLE_FUNCTION(, void, THANDLE_ASSIGN(C), THANDLE(T) *, t1, THANDLE(T), t2 );                                  \
    MOCKABLE_FUNCTION(, void, THANDLE_INITIALIZE(C), THANDLE(T) *, t1, THANDLE(T), t2 );                              \
    MOCKABLE_FUNCTION(, void, THANDLE_MOVE(C), THANDLE(T) *, t1, THANDLE(T)*, t2 );                                   \
    MOCKABLE_FUNCTION(, void, THANDLE_INITIALIZE_MOVE(C), THANDLE(T) *, t1, THANDLE(T)*, t2 );                        \

#endif /*THANDLE_LL_H*/

