// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TARRAY_H
#define TARRAY_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "thandle.h"

#include "umock_c/umock_c_prod.h"

/*TARRAYs are a sort-of THANDLEs that contain a well defined data structure with a well defined extra API that works on TARRAYs*/

/*given a type "T" TARRAY(T) expands to the name of the type. */
#define TARRAY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TARRAY_TYPEDEF_NAME(T), _TAG)
#define TARRAY_TYPEDEF_NAME(T) MU_C2(TARRAY_STRUCT_, T)

/*TARRAY_TYPE(T) introduces the base type that holds an array of T*/
#define TARRAY_STRUCT_TYPE(T)                       \
typedef struct TARRAY_STRUCT_TYPE_NAME_TAG(T)       \
{                                                   \
    uint32_t size;                                  \
    uint32_t capacity;                              \
    T* arr;                                         \
} TARRAY_TYPEDEF_NAME(T);                            \

/*TARRAY is-a THANDLE*/
#define TARRAY(T) THANDLE(TARRAY_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a TARRAY(T)*/
#define TARRAY_CREATE_NAME(T)        MU_C2(TARRAY_CREATE_, T)
#define TARRAY_CREATE(T) TARRAY_CREATE_NAME(T)

/*introduces a function declaration for tarray_create*/
#define TARRAY_CREATE_DECLARE(T) MOCKABLE_FUNCTION(, TARRAY(T), TARRAY_CREATE(T));

/*introduces a function definition for tarray_create*/
#define TARRAY_CREATE_DEFINE(T)                                                                                                 \
    TARRAY(T) TARRAY_CREATE_NAME(T)(void)                                                                                       \
    {                                                                                                                           \
        TARRAY_TYPEDEF_NAME(T)* result;                                                                                         \
        result = THANDLE_MALLOC(TARRAY_TYPEDEF_NAME(T))(NULL);                                                                  \
        if(result == NULL)                                                                                                      \
        {                                                                                                                       \
            LogError("failure in malloc(" MU_TOSTRING(TARRAY_TYPEDEF_NAME(T)) "=%zu", sizeof(TARRAY_TYPEDEF_NAME(T)));          \
            /*return as is*/                                                                                                    \
        }                                                                                                                       \
        else                                                                                                                    \
        {                                                                                                                       \
            result->arr = malloc(sizeof(T));                                                                                    \
            if(result->arr == NULL)                                                                                             \
            {                                                                                                                   \
                LogError("failure in malloc(sizeof(" MU_TOSTRING(T) ")=%zu)",                                                   \
                    sizeof(T));                                                                                                 \
            }                                                                                                                   \
            else                                                                                                                \
            {                                                                                                                   \
                result->capacity = 1;                                                                                           \
                result->size = 0;                                                                                               \
                goto allok;                                                                                                     \
            }                                                                                                                   \
            free(result);                                                                                                       \
            result = NULL;                                                                                                      \
        }                                                                                                                       \
        allok:;                                                                                                                 \
        return result;                                                                                                          \
    }

/*introduces a new name for a function that destroys a TARRAY(T)*/
#define TARRAY_DESTROY_NAME(T)        MU_C2(TARRAY_DESTROY_, T)
#define TARRAY_DESTROY(T) TARRAY_DESTROY_NAME(T)

/*introduces a function declaration for tarray_destroy*/
#define TARRAY_DESTROY_DECLARE(T) MOCKABLE_FUNCTION(, void, TARRAY_DESTROY(T), TARRAY(T), tarray)

#define TARRAY_DESTROY_DEFINE(T) \
void TARRAY_DESTROY(T)(TARRAY(T) tarray)                                    \
{                                                                           \
    if(tarray == NULL)                                                      \
    {                                                                       \
        LogError("invalid argument TARRAY(" MU_TOSTRING(T) ") tarray=%p",   \
            tarray);                                                        \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        free(tarray->arr);                                                  \
        THANDLE_ASSIGN(TARRAY_TYPEDEF_NAME(T))(&tarray, NULL);              \
    }                                                                       \
}                                                                           \


/*macro to be used in headers*/                                                                                     \
#define TARRAY_TYPE_DECLARE(T)                                                                                      \
    TARRAY_STRUCT_TYPE(T)                                                                                           \
    THANDLE_TYPE_DECLARE(TARRAY_TYPEDEF_NAME(T))                                                                    \
    TARRAY_CREATE_DECLARE(T)                                                                                        \
    TARRAY_DESTROY_DECLARE(T)                                                                                       \



#define TARRAY_TYPE_DEFINE(T)                                                                                       \
    THANDLE_TYPE_DEFINE(TARRAY_TYPEDEF_NAME(T))                                                                     \
    TARRAY_CREATE_DEFINE(T)                                                                                         \
    TARRAY_DESTROY_DEFINE(T)                                                                                        \

/*given a type "T" TARRAY_TYPE_DECLARE introduces a structure */


#endif  /*TARRAY_H*/
