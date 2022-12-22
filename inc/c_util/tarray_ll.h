// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TARRAY_LL_H
#define TARRAY_LL_H

#ifdef __cplusplus
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "c_pal/interlocked.h"

#include "c_util/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

/*TARRAY is backed by a THANDLE build on the structure below*/
#define TARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TARRAY_TYPEDEF_NAME(T), _TAG)
#define TARRAY_TYPEDEF_NAME(T) MU_C2(TARRAY_STRUCT_, T)

/*TARRAY_CLEANUP_FUNCTION_TYPE_NAME(T) introduces a new name for the type of the function that is called when the TARRAY(T) is about to be deallocated.*/
#define TARRAY_LL_CLEANUP_FUNCTION_TYPE_NAME(T) MU_C2(TARRAY_TYPEDEF_NAME(T), _CLEANUP)

/*TARRAY_CLEANUP_FUNCTION_TYPE(T) introduces a new function pointer type for the cleanup function - the function to be called before the TARRAY's memory is free()'d*/
#define TARRAY_LL_CLEANUP_FUNCTION_TYPEDEF(T) typedef void(*TARRAY_LL_CLEANUP_FUNCTION_TYPE_NAME(T))(TARRAY_TYPEDEF_NAME(T)* tarray);

/*TARRAY_TYPEDEF_NAME(T) introduces the base type that holds an array of T*/
#define TARRAY_DEFINE_STRUCT_TYPE(T)                                                                            \
/*forward define the typedef of the TARRAY struct so that it can be used for a function pointer definition*/    \
typedef struct TARRAY_STRUCT_TYPE_NAME_TAG(T) TARRAY_TYPEDEF_NAME(T);                                           \
TARRAY_LL_CLEANUP_FUNCTION_TYPEDEF(T)                                                                           \
struct TARRAY_STRUCT_TYPE_NAME_TAG(T)                                                                           \
{                                                                                                               \
    uint32_t capacity;                                                                                          \
    TARRAY_LL_CLEANUP_FUNCTION_TYPE_NAME(T) cleanup;                                                            \
    T* arr;                                                                                                     \
};                                                                                                              \

/*TARRAY is-a THANDLE*/
/*given a type "T" TARRAY_LL(T) expands to the name of the type. */
#define TARRAY_LL(T) THANDLE(TARRAY_TYPEDEF_NAME(T))

/*because TARRAY is a THANDLE, all THANDLE's macro APIs are useable with TARRAY.*/
/*the below are just shortcuts of THANDLE's public ones*/
#define TARRAY_LL_INITIALIZE(T) THANDLE_INITIALIZE(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_LL_ASSIGN(T) THANDLE_ASSIGN(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_LL_MOVE(T) THANDLE_MOVE(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_LL_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(TARRAY_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a TARRAY_LL(T)*/
#define TARRAY_LL_CREATE_NAME(C) MU_C2(TARRAY_LL_CREATE_, C)
#define TARRAY_LL_CREATE(C) TARRAY_LL_CREATE_NAME(C)

#define TARRAY_LL_CREATE_WITH_CAPACITY_NAME(C) MU_C2(TARRAY_LL_CREATE_WITH_CAPACITY_, C)
#define TARRAY_LL_CREATE_WITH_CAPACITY(C) TARRAY_LL_CREATE_WITH_CAPACITY_NAME(C)

/*introduces a name for a function that takes a capacity and a cleanup function pointer*/
#define TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_NAME(C) MU_C2(TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_, C)
#define TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP(C) TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_NAME(C)

/*introduces a function declaration for tarray_create*/
#define TARRAY_LL_CREATE_DECLARE(C, T) MOCKABLE_FUNCTION(, TARRAY_LL(T), TARRAY_LL_CREATE(C));

/*introduces a function declaration for tarray_create_with_capacity*/
#define TARRAY_LL_CREATE_WITH_CAPACITY_DECLARE(C, T) MOCKABLE_FUNCTION(, TARRAY_LL(T), TARRAY_LL_CREATE_WITH_CAPACITY(C), uint32_t, capacity);

/*introduces a function declaration for tarray_create_with_capacity_and_cleanup*/
#define TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_DECLARE(C, T) MOCKABLE_FUNCTION(, TARRAY_LL(T), TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP(C), uint32_t, capacity, TARRAY_LL_CLEANUP_FUNCTION_TYPE_NAME(T), cleanup);

/*introduces a name for the function that free's a TARRAY when it's ref count got to 0*/
#define TARRAY_LL_FREE_NAME(C) MU_C2(TARRAY_LL_FREE_, C)

/*introduces a function definition for freeing the allocated resources for a TARRAY*/
#define TARRAY_LL_FREE_DEFINE(C, T) \
static void TARRAY_LL_FREE_NAME(C)(TARRAY_TYPEDEF_NAME(T)* tarray)                                  \
{                                                                                                   \
    if (tarray == NULL)                                                                             \
    {                                                                                               \
        LogError("invalid arguments " MU_TOSTRING(TARRAY_TYPEDEF_NAME(T)) "* tarray=%p",            \
            tarray);                                                                                \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        if(tarray->cleanup!=NULL)                                                                   \
        {                                                                                           \
            tarray->cleanup(tarray);                                                                \
        }                                                                                           \
        free((void*)tarray->arr);                                                                   \
    }                                                                                               \
}                                                                                                   \

/*introduces a function definition for tarray_create*/
/*Codes_SRS_TARRAY_02_001: [ TARRAY_CREATE(T) shall call THANDLE_MALLOC to allocate the result. ]*/
/*Codes_SRS_TARRAY_02_002: [ TARRAY_CREATE(T) shall call malloc to allocate result->arr. ]*/
/*Codes_SRS_TARRAY_02_004: [ If there are any failures then TARRAY_CREATE(T) shall fail and return NULL. ]*/
/*Codes_SRS_TARRAY_02_003: [ TARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/
#define TARRAY_LL_CREATE_DEFINE(C, T)                                                                                       \
TARRAY_LL(T) TARRAY_LL_CREATE(C)(void)                                                                                      \
{                                                                                                                           \
    return TARRAY_LL_CREATE_WITH_CAPACITY(C)(1);                                                                            \
}

/*introduces a function definition for tarray_create_with_capacity_and_cleanup*/
#define TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_DEFINE(C, T)                                                                     \
TARRAY_LL(T) TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP(C)(uint32_t capacity, TARRAY_LL_CLEANUP_FUNCTION_TYPE_NAME(T) cleanup)      \
{                                                                                                                                   \
    TARRAY_TYPEDEF_NAME(T)* result;                                                                                                 \
    if (capacity == 0)                                                                                                              \
    {                                                                                                                               \
        /* Codes_SRS_TARRAY_01_001: [ If capacity is 0, TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T) shall fail and return NULL. ]*/  \
        LogError("Invalid arguments: uint32_t capacity=%" PRIu32 "", capacity);                                                     \
        result = NULL;                                                                                                              \
    }                                                                                                                               \
    else                                                                                                                            \
    {                                                                                                                               \
        /* Codes_SRS_TARRAY_01_002: [ TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T) shall call THANDLE_MALLOC to allocate the result. ]*/  \
        result = THANDLE_MALLOC(TARRAY_TYPEDEF_NAME(C))(TARRAY_LL_FREE_NAME(C));                                                    \
        if(result == NULL)                                                                                                          \
        {                                                                                                                           \
            /* Codes_SRS_TARRAY_01_005: [ If there are any failures then TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T) shall fail and return NULL. ]*/ \
            LogError("failure in " MU_TOSTRING(THANDLE_MALLOC) "(" MU_TOSTRING(TARRAY_TYPEDEF_NAME(T)) "=%zu)", sizeof(TARRAY_TYPEDEF_NAME(T))); \
            /*return as is*/                                                                                                        \
        }                                                                                                                           \
        else                                                                                                                        \
        {                                                                                                                           \
            /* Codes_SRS_TARRAY_01_003: [ TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T) shall call malloc_2 to allocate capacity entries for result->arr. ]*/ \
            result->arr = malloc_2(capacity,sizeof(T));                                                                             \
            if(result->arr == NULL)                                                                                                 \
            {                                                                                                                       \
                /* Codes_SRS_TARRAY_01_005: [ If there are any failures then TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T) shall fail and return NULL. ]*/ \
                LogError("failure in malloc_2(capacity=%" PRIu32 ", sizeof(" MU_TOSTRING(T) ")=%zu)",                               \
                    capacity, sizeof(T));                                                                                           \
            }                                                                                                                       \
            else                                                                                                                    \
            {                                                                                                                       \
                result->cleanup = cleanup;                                                                                          \
                result->capacity = capacity;                                                                                        \
                /* Codes_SRS_TARRAY_01_004: [ TARRAY_CREATE_WITH_CAPACITY_AND_CLEANUP(T) shall succeed and return a non-NULL value. ]*/         \
                goto allok;                                                                                                         \
            }                                                                                                                       \
            THANDLE_FREE(TARRAY_TYPEDEF_NAME(C))(result);                                                                           \
            result = NULL;                                                                                                          \
        }                                                                                                                           \
    }                                                                                                                               \
    allok:;                                                                                                                         \
    return result;                                                                                                                  \
}

/*introduces a function definition for tarray_create_with_capacity*/
#define TARRAY_LL_CREATE_WITH_CAPACITY_DEFINE(C, T)                                                                         \
TARRAY_LL(T) TARRAY_LL_CREATE_WITH_CAPACITY(C)(uint32_t capacity)                                                           \
{                                                                                                                           \
    return TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP(C)(capacity, NULL);                                                   \
}                                                                                                                           \

/*introduces a name for "what function to call to ensure capacity"  */
#define TARRAY_LL_ENSURE_CAPACITY(C) MU_C2(TARRAY_ENSURE_CAPACITY_, C)

/*introduces the declaration of the function that grows the capacity*/
#define TARRAY_LL_ENSURE_CAPACITY_DECLARE(C, T) \
MOCKABLE_FUNCTION(, int, TARRAY_LL_ENSURE_CAPACITY(C), TARRAY_LL(T), tarray, uint32_t, capacity);

#define TARRAY_LL_ENSURE_CAPACITY_DEFINE(C, T)                                                                                                                  \
int TARRAY_LL_ENSURE_CAPACITY(C)(TARRAY_LL(T) tarray, uint32_t capacity)                                                                                        \
{                                                                                                                                                               \
    int result;                                                                                                                                                 \
    if (tarray == NULL)                                                                                                                                         \
    {                                                                                                                                                           \
        /*Codes_SRS_TARRAY_02_005: [ If tarray is NULL then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/                               \
        LogError("invalid argument TARRAY(" MU_TOSTRING(T) ") tarray=%p, uint32_t capacity=%" PRIu32 "",                                                        \
            tarray, capacity);                                                                                                                                  \
        result = MU_FAILURE;                                                                                                                                    \
    }                                                                                                                                                           \
    else                                                                                                                                                        \
    {                                                                                                                                                           \
        /*Codes_SRS_TARRAY_02_006: [ If tarray->capacity is greater than or equal to capacity then TARRAY_ENSURE_CAPACITY(T) shall succeed and return 0. ]*/    \
        if (capacity <= tarray->capacity)                                                                                                                       \
        {                                                                                                                                                       \
            /*we already have the capacity*/                                                                                                                    \
            result = 0;                                                                                                                                         \
        }                                                                                                                                                       \
        else                                                                                                                                                    \
        {                                                                                                                                                       \
            /*Codes_SRS_TARRAY_02_010: [ If capacity is greater than 2147483648 then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/      \
            if(capacity > (((uint32_t)1)<<31))                                                                                                                  \
            {                                                                                                                                                   \
                /*Codes_SRS_TARRAY_02_009: [ If there are any failures then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/               \
                LogError("capacity (%" PRIu32 ") cannot be improved by doubling", capacity);                                                                    \
                result = MU_FAILURE;                                                                                                                            \
            }                                                                                                                                                   \
            else                                                                                                                                                \
            {                                                                                                                                                   \
                /*Codes_SRS_TARRAY_02_007: [ TARRAY_ENSURE_CAPACITY(T) shall shall call realloc_2 to resize arr to the next multiple of 2 greater than or equal to capacity. ]*/       \
                uint32_t new_capacity = tarray->capacity;                                                                                                       \
                do{                                                                                                                                             \
                    new_capacity *= 2;                                                                                                                          \
                }while(new_capacity<capacity);                                                                                                                  \
                T* temp = realloc_2((void*)tarray->arr, new_capacity, sizeof(T));                                                                               \
                if (temp == NULL)                                                                                                                               \
                {                                                                                                                                               \
                    /*Codes_SRS_TARRAY_02_009: [ If there are any failures then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/           \
                    LogError("failure in realloc_2(tarray->arr=%p, new_capacity=%" PRIu32 ", sizeof(T)=%zu)",                                                   \
                        tarray->arr, new_capacity, sizeof(T));                                                                                                  \
                    result = MU_FAILURE;                                                                                                                        \
                }                                                                                                                                               \
                else                                                                                                                                            \
                {                                                                                                                                               \
                    /*Codes_SRS_TARRAY_02_008: [ TARRAY_ENSURE_CAPACITY(T) shall succeed, record the new computed capacity, and return 0. ]*/                   \
                    ((TARRAY_TYPEDEF_NAME(T)*)tarray)->capacity = new_capacity;                                                                                 \
                    ((TARRAY_TYPEDEF_NAME(T)*)tarray)->arr = temp;                                                                                              \
                    result = 0;                                                                                                                                 \
                }                                                                                                                                               \
            }                                                                                                                                                   \
        }                                                                                                                                                       \
    }                                                                                                                                                           \
    return result;                                                                                                                                              \
}

/*macro to be used in headers*/                                                                                     \
#define TARRAY_LL_TYPE_DECLARE(C, T)                                                                                \
    /*hint: have TARRAY_DEFINE_STRUCT_TYPE(T) before TARRAY_LL_TYPE_DECLARE*/                                       \
    THANDLE_LL_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(C), TARRAY_TYPEDEF_NAME(T))                                         \
    TARRAY_LL_CREATE_DECLARE(C, T)                                                                                  \
    TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_DECLARE(C, T)                                                        \
    TARRAY_LL_CREATE_WITH_CAPACITY_DECLARE(C, T)                                                                    \
    TARRAY_LL_ENSURE_CAPACITY_DECLARE(C, T)                                                                         \

#define TARRAY_LL_TYPE_DEFINE(C, T)                                                                                 \
    /*hint: have THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(T)) before TARRAY_LL_TYPE_DEFINE*/                         \
    TARRAY_LL_FREE_DEFINE(C, T)                                                                                     \
    TARRAY_LL_CREATE_DEFINE(C, T)                                                                                   \
    TARRAY_LL_CREATE_WITH_CAPACITY_AND_CLEANUP_DEFINE(C, T)                                                         \
    TARRAY_LL_CREATE_WITH_CAPACITY_DEFINE(C, T)                                                                     \
    TARRAY_LL_ENSURE_CAPACITY_DEFINE(C, T)                                                                          \

#endif  /*TARRAY_LL_H*/
