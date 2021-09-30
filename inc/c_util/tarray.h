// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TARRAY_H
#define TARRAY_H

#ifdef __cplusplus
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "thandle.h"

#include "umock_c/umock_c_prod.h"

/*TARRAY is backed by a THANDLE build on the structure below*/
#define TARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TARRAY_TYPEDEF_NAME(T), _TAG)
#define TARRAY_TYPEDEF_NAME(T) MU_C2(TARRAY_STRUCT_, T)

/*TARRAY_TYPE(T) introduces the base type that holds an array of T*/
#define TARRAY_DEFINE_STRUCT_TYPE(T)                                                \
typedef struct TARRAY_STRUCT_TYPE_NAME_TAG(T)                                       \
{                                                                                   \
    uint32_t capacity;                                                              \
    T* arr;                                                                         \
} TARRAY_TYPEDEF_NAME(T);                                                           \

/*TARRAY is-a THANDLE*/
/*given a type "T" TARRAY(T) expands to the name of the type. */
#define TARRAY(T) THANDLE(TARRAY_TYPEDEF_NAME(T))

/*because TARRAY is a THANDLE, all THANDLE's macro APIs are useable with TARRAY.*/
/*the below are just shortcuts of THANDLE's public ones*/
#define TARRAY_INITIALIZE(T) THANDLE_INITIALIZE(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_ASSIGN(T) THANDLE_ASSIGN(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_DEC_REF(T) THANDLE_DEC_REF(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_INC_REF(T) THANDLE_INC_REF(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_MOVE(T) THANDLE_MOVE(TARRAY_TYPEDEF_NAME(T))
#define TARRAY_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(TARRAY_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a TARRAY(T)*/
#define TARRAY_CREATE_NAME(T) MU_C2(TARRAY_CREATE_, T)
#define TARRAY_CREATE(T) TARRAY_CREATE_NAME(T)

/*introduces a function declaration for tarray_create*/
#define TARRAY_CREATE_DECLARE(T) MOCKABLE_FUNCTION(, TARRAY(T), TARRAY_CREATE(T));

/*introduces a name for the function that free's a TARRAY when it's ref count got to 0*/
#define TARRAY_FREE_NAME(T) MU_C2(TARRAY_FREE_, T) 

/*introduces a function definition for freeing the allocated resources for a TARRAY*/
#define TARRAY_FREE_DEFINE(T) \
static void TARRAY_FREE_NAME(T)(TARRAY_TYPEDEF_NAME(T)* tarray)                                     \
{                                                                                                   \
    if (tarray == NULL)                                                                             \
    {                                                                                               \
        LogError("invalid arguments " MU_TOSTRING(TARRAY_TYPEDEF_NAME(T)) " * tarray=%p",           \
            tarray);                                                                                \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        free((void*)tarray->arr);                                                                   \
    }                                                                                               \
}                                                                                                   \

/*introduces a function definition for tarray_create*/
#define TARRAY_CREATE_DEFINE(T)                                                                                             \
TARRAY(T) TARRAY_CREATE(T)(void)                                                                                            \
{                                                                                                                           \
    TARRAY_TYPEDEF_NAME(T)* result;                                                                                         \
    /*Codes_SRS_TARRAY_02_001: [ TARRAY_CREATE(T) shall call THANDLE_MALLOC to allocate the result. ]*/                     \
    result = THANDLE_MALLOC(TARRAY_TYPEDEF_NAME(T))(TARRAY_FREE_NAME(T));                                                   \
    if(result == NULL)                                                                                                      \
    {                                                                                                                       \
        LogError("failure in malloc(" MU_TOSTRING(TARRAY_TYPEDEF_NAME(T)) "=%zu", sizeof(TARRAY_TYPEDEF_NAME(T)));          \
        /*return as is*/                                                                                                    \
    }                                                                                                                       \
    else                                                                                                                    \
    {                                                                                                                       \
        /*Codes_SRS_TARRAY_02_002: [ TARRAY_CREATE(T) shall call malloc to allocate result->arr. ]*/                        \
        result->arr = malloc(1 * sizeof(T));                                                                                \
        if(result->arr == NULL)                                                                                             \
        {                                                                                                                   \
            /*Codes_SRS_TARRAY_02_004: [ If there are any failures then TARRAY_CREATE(T) shall fail and return NULL ]*/     \
            LogError("failure in malloc(1 * sizeof(" MU_TOSTRING(T) ")=%zu)",                                               \
                sizeof(T));                                                                                                 \
        }                                                                                                                   \
        else                                                                                                                \
        {                                                                                                                   \
            /*Codes_SRS_TARRAY_02_003: [ TARRAY_CREATE(T) shall succeed and return a non-NULL value. ]*/                    \
            result->capacity = 1;                                                                                           \
            goto allok;                                                                                                     \
        }                                                                                                                   \
        THANDLE_FREE(TARRAY_TYPEDEF_NAME(T))(result);                                                                       \
        result = NULL;                                                                                                      \
    }                                                                                                                       \
    allok:;                                                                                                                 \
    return result;                                                                                                          \
}

/*introduces a name for "what function to call to ensure capacity"  */
#define TARRAY_ENSURE_CAPACITY(T) MU_C2(TARRAY_ENSURE_CAPACITY_, T)

/*introduces the declaration of the function that grows the capacity*/
#define TARRAY_ENSURE_CAPACITY_DECLARE(T) \
MOCKABLE_FUNCTION(, int, TARRAY_ENSURE_CAPACITY(T), TARRAY(T), tarray, uint32_t, capacity);

#define TARRAY_ENSURE_CAPACITY_DEFINE(T)                                                                                                                        \
int TARRAY_ENSURE_CAPACITY(T)(TARRAY(T) tarray, uint32_t capacity)                                                                                              \
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
            if(capacity > (((uint32_t)1)<<31))                                                                                                                  \
            {                                                                                                                                                   \
                /*Codes_SRS_TARRAY_02_009: [ If there are any failures then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/               \
                LogError("capacity (%" PRIu32 ")cannot be improved by doubling", capacity);                                                                     \
                result = MU_FAILURE;                                                                                                                            \
            }                                                                                                                                                   \
            else                                                                                                                                                \
            {                                                                                                                                                   \
                /*Codes_SRS_TARRAY_02_007: [ TARRAY_ENSURE_CAPACITY(T) shall realloc arr to the next multiple of 2 greater than or equal to capacity. ]*/       \
                uint32_t new_capacity = tarray->capacity;                                                                                                       \
                do{                                                                                                                                             \
                    new_capacity *= 2;                                                                                                                          \
                }while(new_capacity<capacity);                                                                                                                  \
                T* temp = realloc((void*)tarray->arr, new_capacity * sizeof(T));                                                                                \
                if (temp == NULL)                                                                                                                               \
                {                                                                                                                                               \
                    /*Codes_SRS_TARRAY_02_009: [ If there are any failures then TARRAY_ENSURE_CAPACITY(T) shall fail and return a non-zero value. ]*/           \
                    LogError("failure in realloc(tarray->arr=%p, new_capacity=%" PRIu32 " * sizeof(T)=%zu)",                                                    \
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
#define TARRAY_TYPE_DECLARE(T)                                                                                      \
    TARRAY_DEFINE_STRUCT_TYPE(T)                                                                                    \
    THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(T))                                                                    \
    TARRAY_CREATE_DECLARE(T)                                                                                        \
    TARRAY_ENSURE_CAPACITY_DECLARE(T)                                                                               \

#define TARRAY_TYPE_DEFINE(T)                                                                                       \
    THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(T))                                                                     \
    TARRAY_FREE_DEFINE(T)                                                                                           \
    TARRAY_CREATE_DEFINE(T)                                                                                         \
    TARRAY_ENSURE_CAPACITY_DEFINE(T)                                                                                \

#endif  /*TARRAY_H*/
