// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_TUPLE_ARRAY_H
#define THANDLE_TUPLE_ARRAY_H

#ifdef __cplusplus
#include <cinttypes>
#else
#include <inttypes.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_util/thandle.h"

#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#define THANDLE_TUPLE_ARRAY_STRUCT_MEMBER(type, name) THANDLE(type) name;

#define DECLARE_THANDLE_TUPLE_TYPE(name, ...) \
    typedef struct MU_C2(name, _TAG) \
    { \
        MU_FOR_EACH_2(THANDLE_TUPLE_ARRAY_STRUCT_MEMBER, __VA_ARGS__) \
    } name;

#define THANDLE_TUPLE_ARRAY_TYPE(name) MU_C2A(name, _ARRAY)

#define DECLARE_THANDLE_TUPLE_ARRAY_TYPE(name) \
    typedef struct MU_C2B(THANDLE_TUPLE_ARRAY_TYPE(name), _TAG) \
    { \
        const uint32_t count; \
        name tuple_array[]; \
    } THANDLE_TUPLE_ARRAY_TYPE(name);

#define THANDLE_TUPLE_ARRAY_CREATE(name) MU_C2A(name, _array_create)
#define THANDLE_TUPLE_ARRAY_DESTROY(name) MU_C2A(name, _array_destroy)

#define DECLARE_THANDLE_TUPLE_ARRAY(name, ...) \
    DECLARE_THANDLE_TUPLE_TYPE(name, __VA_ARGS__) \
    DECLARE_THANDLE_TUPLE_ARRAY_TYPE(name) \
    MOCKABLE_FUNCTION(, THANDLE_TUPLE_ARRAY_TYPE(name)*, THANDLE_TUPLE_ARRAY_CREATE(name), uint32_t, count); \
    MOCKABLE_FUNCTION(, void, THANDLE_TUPLE_ARRAY_DESTROY(name), THANDLE_TUPLE_ARRAY_TYPE(name)*, tuple_array);

#define THANDLE_TUPLE_ARRAY_INIT_MEMBER(type, name) THANDLE_INITIALIZE(type)(&result->tuple_array[i].name, NULL);

#define IMPLEMENT_THANDLE_TUPLE_ARRAY_CREATE(name, ...) \
    IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE_TUPLE_ARRAY_TYPE(name)*, THANDLE_TUPLE_ARRAY_CREATE(name), uint32_t, count) \
    { \
        THANDLE_TUPLE_ARRAY_TYPE(name)* result; \
        /*Codes_SRS_THANDLE_TUPLE_ARRAY_42_001: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall allocate memory for the array. ]*/ \
        result = malloc_flex(sizeof(THANDLE_TUPLE_ARRAY_TYPE(name)), count, sizeof(name)); \
        \
        if (result == NULL) \
        { \
            /*Codes_SRS_THANDLE_TUPLE_ARRAY_42_004: [ If there are any errors then THANDLE_TUPLE_ARRAY_CREATE(name) shall fail and return NULL. ]*/ \
            LogError("failure in malloc_flex((sizeof(THANDLE_TUPLE_ARRAY_TYPE(name))=%zu, count=%" PRIu32 ", sizeof(name)=%zu)) failed", sizeof(THANDLE_TUPLE_ARRAY_TYPE(name)), count, sizeof(name)); \
        } \
        else \
        { \
            /* Need to cast to assign to the const member */ \
            *(uint32_t*)&result->count = count; \
            \
            /*Codes_SRS_THANDLE_TUPLE_ARRAY_42_003: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall initialize the members of the tuples in the array to NULL. ]*/ \
            for (uint32_t i = 0; i < count; ++i) \
            { \
                MU_FOR_EACH_2(THANDLE_TUPLE_ARRAY_INIT_MEMBER, __VA_ARGS__) \
            } \
            \
            /*Codes_SRS_THANDLE_TUPLE_ARRAY_42_005: [ THANDLE_TUPLE_ARRAY_CREATE(name) shall succeed and return the allocated array. ]*/ \
            goto all_ok; \
        } \
        free(result); \
        result = NULL; \
    all_ok: \
        return result; \
    }

#define THANDLE_TUPLE_ARRAY_FREE_MEMBER(type, name) \
    if (tuple_array->tuple_array[i].name != NULL) \
    { \
        THANDLE_ASSIGN(type)(&tuple_array->tuple_array[i].name, NULL); \
    }

#define IMPLEMENT_THANDLE_TUPLE_ARRAY_DESTROY(name, ...) \
    IMPLEMENT_MOCKABLE_FUNCTION(, void, THANDLE_TUPLE_ARRAY_DESTROY(name), THANDLE_TUPLE_ARRAY_TYPE(name)*, tuple_array) \
    { \
        if (tuple_array == NULL) \
        { \
            /*Codes_SRS_THANDLE_TUPLE_ARRAY_42_006: [ If tuple_array is NULL then THANDLE_TUPLE_ARRAY_DESTROY(name) shall fail and return. ]*/ \
            LogError("Invalid args: " MU_TOSTRING(THANDLE_TUPLE_ARRAY_TYPE(name)) "* tuple_array = %p", \
                tuple_array); \
        } \
        else \
        { \
            /*Codes_SRS_THANDLE_TUPLE_ARRAY_42_007: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall iterate over all of the elements in tuple_array and call THANDLE_ASSIGN(type) with NULL for each field. ]*/ \
            for (uint32_t i = 0; i < tuple_array->count; ++i) \
            { \
                MU_FOR_EACH_2(THANDLE_TUPLE_ARRAY_FREE_MEMBER, __VA_ARGS__) \
            } \
            /*Codes_SRS_THANDLE_TUPLE_ARRAY_42_008: [ THANDLE_TUPLE_ARRAY_DESTROY(name) shall free the memory allocated in tuple_array. ]*/ \
            free(tuple_array); \
        } \
    }

#define DEFINE_THANDLE_TUPLE_ARRAY(name, ...) \
    IMPLEMENT_THANDLE_TUPLE_ARRAY_CREATE(name, __VA_ARGS__) \
    IMPLEMENT_THANDLE_TUPLE_ARRAY_DESTROY(name, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // THANDLE_TUPLE_ARRAY_H
