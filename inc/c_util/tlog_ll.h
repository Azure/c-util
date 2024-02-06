// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TLOG_LL_H
#define TLOG_LL_H

#ifdef __cplusplus
#include <cinttypes>
#else // __cplusplus
#include <stdbool.h>
#include <inttypes.h>
#endif // __cplusplus

#include "c_pal/interlocked.h"
#include "c_pal/srw_lock_ll.h"

#include "c_pal/thandle_ll.h"

#include "umock_c/umock_c_prod.h"

#define TLOG_PUSH_RESULT_VALUES \
    TLOG_PUSH_OK, \
    TLOG_PUSH_INVALID_ARG, \
    TLOG_PUSH_QUEUE_FULL \

MU_DEFINE_ENUM(TLOG_PUSH_RESULT, TLOG_PUSH_RESULT_VALUES);

#define TLOG_POP_RESULT_VALUES \
    TLOG_POP_OK, \
    TLOG_POP_INVALID_ARG, \
    TLOG_POP_QUEUE_EMPTY, \
    TLOG_POP_REJECTED

MU_DEFINE_ENUM(TLOG_POP_RESULT, TLOG_POP_RESULT_VALUES);

#define LOG_ENTRY_STATE_VALUES \
    LOG_ENTRY_STATE_NOT_USED, \
    LOG_ENTRY_STATE_PUSHING, \
    LOG_ENTRY_STATE_USED, \
    LOG_ENTRY_STATE_POPPING \

MU_DEFINE_ENUM(LOG_ENTRY_STATE, LOG_ENTRY_STATE_VALUES);

/*TLOG is backed by a THANDLE build on the structure below*/
#define TLOG_STRUCT_TYPE_NAME_TAG(T) MU_C2(TLOG_TYPEDEF_NAME(T), _TAG)
#define TLOG_TYPEDEF_NAME(T) MU_C2(TLOG_STRUCT_, T)

#define TLOG_ENTRY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TLOG_ENTRY_STRUCT_TYPE_NAME(T), _TAG)
#define TLOG_ENTRY_STRUCT_TYPE_NAME(T) MU_C2(TLOG_ENTRY_STRUCT_, T)

#define TLOG_CHUNK_STRUCT_TYPE_NAME_TAG(T) MU_C2(TLOG_CHUNK_STRUCT_TYPE_NAME(T), _TAG)
#define TLOG_CHUNK_STRUCT_TYPE_NAME(T) MU_C2(TLOG_CHUNK_STRUCT_, T)
#define TLOG_CHUNK_TYPEDEF_NAME(T) MU_C2(TLOG_CHUNK_STRUCT_, T)

/* This introduces the name for the copy item function */
#define TLOG_DEFINE_COPY_ITEM_FUNCTION_TYPE_NAME(T) MU_C2(TLOG_COPY_ITEM_FUNC_TYPE_, T)
#define TLOG_COPY_ITEM_FUNC(T) TLOG_DEFINE_COPY_ITEM_FUNCTION_TYPE_NAME(T)

/* This introduces the name for the dispose item function */
#define TLOG_DEFINE_DISPOSE_ITEM_FUNCTION_TYPE_NAME(T) MU_C2(TLOG_DISPOSE_ITEM_FUNC_TYPE_, T)
#define TLOG_DISPOSE_ITEM_FUNC(T) TLOG_DEFINE_DISPOSE_ITEM_FUNCTION_TYPE_NAME(T)

/* This introduces the name for the pop condition function */
#define TLOG_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T) MU_C2(TLOG_CONDITION_FUNC_TYPE_, T)
#define TLOG_CONDITION_FUNC(T) TLOG_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T)

/*TLOG_DEFINE_STRUCT_TYPE(T) introduces the base type that holds the queue typed as T*/
#define TLOG_DEFINE_STRUCT_TYPE(T)                                                                            \
typedef void (*TLOG_DEFINE_COPY_ITEM_FUNCTION_TYPE_NAME(T))(void* context, T* dst, T* src);                   \
typedef void (*TLOG_DEFINE_DISPOSE_ITEM_FUNCTION_TYPE_NAME(T))(void* context, T* item);                       \
typedef bool (*TLOG_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T))(void* context, T* item);                          \
typedef struct TLOG_ENTRY_STRUCT_TYPE_NAME_TAG(T)                                                             \
{                                                                                                               \
    union                                                                                                       \
    {                                                                                                           \
        volatile_atomic int32_t state;                                                                          \
        volatile_atomic LOG_ENTRY_STATE state_as_enum;                                                        \
    };                                                                                                          \
    T value;                                                                                                    \
} TLOG_ENTRY_STRUCT_TYPE_NAME(T);                                                                             \
typedef struct TLOG_CHUNK_STRUCT_TYPE_NAME_TAG(T)                                                                   \
{                                                                                                               \
    TLOG_ENTRY_STRUCT_TYPE_NAME(T) items[];                                                                   \
} TLOG_CHUNK_TYPEDEF_NAME(T);                                                                                       \
typedef struct TLOG_STRUCT_TYPE_NAME_TAG(T)                                                                   \
{                                                                                                               \
    volatile_atomic int64_t head;                                                                               \
    volatile_atomic int64_t tail;                                                                               \
    TLOG_COPY_ITEM_FUNC(T) copy_item_function;                                                                \
    TLOG_DISPOSE_ITEM_FUNC(T) dispose_item_function;                                                          \
    void* dispose_item_function_context;                                                                        \
    uint32_t log_size;                                                                                        \
    uint32_t chunk_size;                                                                                        \
    uint32_t chunk_count;                                                                                        \
    struct TLOG_CHUNK_TYPEDEF_NAME(T)* chunks[];                                                                      \
} TLOG_TYPEDEF_NAME(T);                                                                                       \

/*TLOG is-a THANDLE*/
/*given a type "T" TLOG_LL(T) expands to the name of the type. */
#define TLOG_LL(T) THANDLE(TLOG_TYPEDEF_NAME(T))

/*because TLOG is a THANDLE, all THANDLE's macro APIs are useable with TLOG.*/
/*the below are just shortcuts of THANDLE's public ones*/
#define TLOG_LL_INITIALIZE(T) THANDLE_INITIALIZE(TLOG_TYPEDEF_NAME(T))
#define TLOG_LL_ASSIGN(T) THANDLE_ASSIGN(TLOG_TYPEDEF_NAME(T))
#define TLOG_LL_MOVE(T) THANDLE_MOVE(TLOG_TYPEDEF_NAME(T))
#define TLOG_LL_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(TLOG_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a TLOG_LL(T)*/
#define TLOG_LL_CREATE_NAME(C) MU_C2(TLOG_LL_CREATE_, C)
#define TLOG_LL_CREATE(C) TLOG_LL_CREATE_NAME(C)

/*introduces a new name for the push function */
#define TLOG_LL_PUSH_NAME(C) MU_C2(TLOG_LL_PUSH_, C)
#define TLOG_LL_PUSH(C) TLOG_LL_PUSH_NAME(C)

/*introduces a new name for the pop function */
#define TLOG_LL_POP_NAME(C) MU_C2(TLOG_LL_POP_, C)
#define TLOG_LL_POP(C) TLOG_LL_POP_NAME(C)

/*introduces a function declaration for tlog_create*/
#define TLOG_LL_CREATE_DECLARE(C, T) MOCKABLE_FUNCTION(, TLOG_LL(T), TLOG_LL_CREATE(C), uint32_t, log_size, uint32_t, chunk_size, TLOG_COPY_ITEM_FUNC(T), copy_item_function, TLOG_DISPOSE_ITEM_FUNC(T), dispose_item_function, void*, dispose_item_function_context);

/*introduces a function declaration for tlog_push*/
#define TLOG_LL_PUSH_DECLARE(C, T) MOCKABLE_FUNCTION(, TLOG_PUSH_RESULT, TLOG_LL_PUSH(C), TLOG_LL(T), tlog, int64_t, index, T*, item, void*, copy_item_function_context);

/*introduces a function declaration for tlog_pop*/
#define TLOG_LL_POP_DECLARE(C, T) MOCKABLE_FUNCTION(, TLOG_POP_RESULT, TLOG_LL_POP(C), TLOG_LL(T), tlog, int64_t, index, T*, item, void*, copy_item_function_context, TLOG_CONDITION_FUNC(T), condition_function, void*, condition_function_context);

/*introduces a name for the function that free's a TLOG when it's ref count got to 0*/
#define TLOG_LL_FREE_NAME(C) MU_C2(TLOG_LL_FREE_, C)

/*introduces a function definition for freeing the allocated resources for a TLOG*/
#define TLOG_LL_FREE_DEFINE(C, T) \
static void TLOG_LL_FREE_NAME(C)(TLOG_TYPEDEF_NAME(T)* tlog)                                                                                                  \
{                                                                                                                                                                   \
    if (tlog == NULL)                                                                                                                                             \
    {                                                                                                                                                               \
        LogError("invalid arguments " MU_TOSTRING(TLOG_TYPEDEF_NAME(T)) "* tlog=%p",                                                                            \
            tlog);                                                                                                                                                \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        if (tlog->dispose_item_function == NULL)                                                                                                                  \
        {                                                                                                                                                           \
            /* Codes_SRS_TLOG_01_008: [ If dispose_item_function passed to TLOG_CREATE(T) is NULL, TLOG_DISPOSE_FUNC(T) shall return. ]*/                     \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*introduces a function definition for tlog_create*/
#define TLOG_LL_CREATE_DEFINE(C, T)                                                                                                                               \
TLOG_LL(T) TLOG_LL_CREATE(C)(uint32_t log_size, uint32_t chunk_size, TLOG_COPY_ITEM_FUNC(T) copy_item_function, TLOG_DISPOSE_ITEM_FUNC(T) dispose_item_function, void* dispose_item_function_context) \
{                                                                                                                                                                   \
    TLOG_TYPEDEF_NAME(T)* result;                                                                                                                                 \
    bool is_copy_item_function_NULL = (copy_item_function == NULL);                                                                                                 \
    bool is_dispose_item_function_NULL = (dispose_item_function == NULL);                                                                                           \
    if (                                                                                                                                                            \
        /* Codes_SRS_TLOG_01_001: [ If queue_size is 0, TLOG_CREATE(T) shall fail and return NULL. ]*/                                                          \
        (log_size == 0) ||                                                                                                                                        \
        ((log_size % chunk_size) != 0) || \
        /* Codes_SRS_TLOG_01_002: [ If any of copy_item_function and dispose_item_function is NULL and at least one of them is not NULL, TLOG_CREATE(T) shall fail and return NULL. ]*/ \
        ((is_copy_item_function_NULL || is_dispose_item_function_NULL) &&                                                                                           \
         !(is_copy_item_function_NULL && is_dispose_item_function_NULL))                                                                                            \
       )                                                                                                                                                            \
    {                                                                                                                                                               \
        LogError("Invalid arguments: uint32_t log_size=%" PRIu32 ", " MU_TOSTRING(TLOG_COPY_ITEM_FUNC(T)) " copy_item_function=%p, " MU_TOSTRING(TLOG_DISPOSE_ITEM_FUNC(T)) " dispose_item_function=%p, void* dispose_item_function_context=%p", \
            log_size, copy_item_function, dispose_item_function, dispose_item_function_context);                                                        \
        result = NULL;                                                                                                                                              \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        uint32_t chunk_count = log_size / chunk_size; \
        /* Codes_SRS_TLOG_01_003: [ TLOG_CREATE(T) shall call THANDLE_MALLOC_FLEX with TLOG_DISPOSE_FUNC(T) as dispose function, nmemb set to queue_size and size set to sizeof(T). ]*/ \
        result = THANDLE_MALLOC_FLEX(TLOG_TYPEDEF_NAME(C))(TLOG_LL_FREE_NAME(C), log_size, sizeof(TLOG_ENTRY_STRUCT_TYPE_NAME(T)));                         \
        if(result == NULL)                                                                                                                                          \
        {                                                                                                                                                           \
            /* Codes_SRS_TLOG_01_007: [ If there are any failures then TLOG_CREATE(T) shall fail and return NULL. ]*/                                           \
            LogError("failure in " MU_TOSTRING(THANDLE_MALLOC_FLEX) "(sizeof(" MU_TOSTRING(TLOG_TYPEDEF_NAME(C)) ")=%zu, log_size=%" PRIu32 ", sizeof(" MU_TOSTRING(TLOG_ENTRY_STRUCT_TYPE_NAME(T)) ")=%zu)", \
                sizeof(TLOG_TYPEDEF_NAME(C)), log_size, sizeof(TLOG_ENTRY_STRUCT_TYPE_NAME(T)));                                                              \
            /*return as is*/                                                                                                                                        \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            result->chunk_count = chunk_count; \
            result->chunk_size = chunk_size; \
            result->log_size = log_size;                                                                                                                        \
            result->copy_item_function = copy_item_function;                                                                                                        \
            result->dispose_item_function = dispose_item_function;                                                                                                  \
            result->dispose_item_function_context = dispose_item_function_context;                                                                                  \
            /* Codes_SRS_TLOG_01_004: [ TLOG_CREATE(T) shall initialize the head and tail of the list with 0 by using interlocked_exchange_64. ]*/              \
            (void)interlocked_exchange_64(&result->head, 0);                                                                                                        \
            (void)interlocked_exchange_64(&result->tail, 0);                                                                                                        \
            for (uint32_t i = 0; i < chunk_count; i++)                                                                                                               \
            {                                                                                                                                                       \
                /* Codes_SRS_TLOG_01_005: [ TLOG_CREATE(T) shall initialize the state for each entry in the array used for the queue with NOT_USED by using interlocked_exchange. ]*/ \
                result->chunks[i] = NULL;                                                                    \
            }                                                                                                                                                       \
            /* Codes_SRS_TLOG_01_006: [ TLOG_CREATE(T) shall succeed and return a non-NULL value. ]*/                                                           \
            /*return as is*/                                                                                                                                        \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}

/*introduces a function definition for tlog_push*/
#define TLOG_LL_PUSH_DEFINE(C, T)                                                                                                                                 \
TLOG_PUSH_RESULT TLOG_LL_PUSH(C)(TLOG_LL(T) tlog, int64_t index, T* item, void* copy_item_function_context)                                                                \
{                                                                                                                                                                   \
    TLOG_PUSH_RESULT result;                                                                                                                                      \
    if (                                                                                                                                                            \
        /* Codes_SRS_TLOG_01_012: [ If tlog is NULL then TLOG_PUSH(T) shall fail and return TLOG_PUSH_INVALID_ARG. ]*/                                      \
        (tlog == NULL) ||                                                                                                                                         \
        /* Codes_SRS_TLOG_01_013: [ If item is NULL then TLOG_PUSH(T) shall fail and return TLOG_PUSH_INVALID_ARG. ]*/                                        \
        (item == NULL)                                                                                                                                              \
       )                                                                                                                                                            \
    {                                                                                                                                                               \
        LogError("Invalid arguments: TLOG_LL(" MU_TOSTRING(T) ") tlog=%p, const " MU_TOSTRING(T) "* item=%p, void* copy_item_function_context=%p",              \
            tlog, item, copy_item_function_context);                                                                                                              \
        result = TLOG_PUSH_INVALID_ARG;                                                                                                                           \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        uint32_t chunk_index = index / tlog->chunk_size; \
        uint32_t chunk_item_index = index % tlog->chunk_size; \
        srw_lock_ll_acquire_shared(tlog->chunks_lock); \
        do \
        { \
            if (tlog->chunks[chunk_index] == NULL) \
            { \
            } \
            else \
            { \
                break; \
            } \
        } \
        TLOG_CHUNK_STRUCT_TYPENAME(T)* chunk = &tlog->chunks[chunk_index]; \
        if (interlocked_compare_exchange((volatile_atomic int32_t*)&chunk->items[chunk_item_index].state, LOG_ENTRY_STATE_PUSHING, LOG_ENTRY_STATE_NOT_USED) != LOG_ENTRY_STATE_NOT_USED) \
        { \
            LogError("Already used"); \
            result = TLOG_PUSH_ERROR; \
        } \
        else \
        { \
            if (tlog->copy_item_function == NULL)                                                                                                     \
            {                                                                                                                                           \
                (void)memcpy((void*)&chunk->items[chunk_item_index].value, (void*)item, sizeof(T));                                                               \
            }                                                                                                                                           \
            else                                                                                                                                        \
            {                                                                                                                                           \
                tlog->copy_item_function(copy_item_function_context, (T*)&chunk->items[chunk_item_index].value, item);                                          \
            }                                                                                                                                           \
            (void)interlocked_exchange((volatile_atomic int32_t*)&tlog->queue[index].state, LOG_ENTRY_STATE_USED);                                  \
            result = TLOG_PUSH_OK;                                                                                                                    \
        } \
        srw_lock_ll_release_shared(tlog->chunks_lock); \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

/*introduces a function definition for tlog_pop*/
#define TLOG_LL_POP_DEFINE(C, T)                                                                                                                                  \
TLOG_POP_RESULT TLOG_LL_POP(C)(TLOG_LL(T) tlog, int64_t index, T* item, void* copy_item_function_context, TLOG_CONDITION_FUNC(T) condition_function, void* condition_function_context) \
{                                                                                                                                                                   \
    TLOG_POP_RESULT result;                                                                                                                                       \
    if (                                                                                                                                                            \
        /* Codes_SRS_TLOG_01_025: [ If tlog is NULL then TLOG_POP(T) shall fail and return TLOG_POP_INVALID_ARG. ]*/                                        \
        (tlog == NULL) ||                                                                                                                                         \
        /* Codes_SRS_TLOG_01_027: [ If item is NULL then TLOG_POP(T) shall fail and return TLOG_POP_INVALID_ARG. ]*/                                          \
        (item == NULL)                                                                                                                                              \
       )                                                                                                                                                            \
    {                                                                                                                                                               \
        LogError("Invalid arguments: TLOG_LL(" MU_TOSTRING(T) ") tlog=%p, " MU_TOSTRING(T) "*=%p, void* copy_item_function_context, TLOG_CONDITION_FUNC(T) condition_function, void* condition_function_context", \
            tlog, item);                                                                                                                                          \
        result = TLOG_POP_INVALID_ARG;                                                                                                                            \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        uint32_t chunk_index = index / tlog->chunk_size; \
        uint32_t chunk_item_index = index % tlog->chunk_size; \
        srw_lock_ll_acquire_shared(tlog->chunks_lock); \
        if (interlocked_compare_exchange((volatile_atomic int32_t*)&tlog->queue[index].state, LOG_ENTRY_STATE_POPPING, LOG_ENTRY_STATE_USED) != LOG_ENTRY_STATE_USED) \
        { \
            LogError("Not used"); \
            result = TLOG_POP_ERROR; \
        } \
        else \
        { \
            if (tlog->copy_item_function == NULL)                                                                                                 \
            {                                                                                                                                       \
                (void)memcpy((void*)item, (void*)&tlog->queue[index].value, sizeof(T));                                                       \
            }                                                                                                                                       \
            else                                                                                                                                    \
            {                                                                                                                                       \
                tlog->copy_item_function(copy_item_function_context, item, (T*)&tlog->queue[index].value);                                      \
            }                                                                                                                                       \
            if (tlog->dispose_item_function != NULL)                                                                                              \
            {                                                                                                                                       \
                /* Codes_SRS_TLOG_01_045: [ TLOG_POP(T) shall call dispose_item_function with dispose_item_function_context as context and the array entry value whose state was changed to POPPING as item. ]*/ \
                tlog->dispose_item_function(tlog->dispose_item_function_context, (T*)&tlog->queue[index].value);                              \
            }                                                                                                                                       \
            /* Codes_SRS_TLOG_01_034: [ TLOG_POP(T) shall set the state to NOT_USED by using interlocked_exchange, succeed and return TLOG_POP_OK. ]*/ \
            (void)interlocked_exchange((volatile_atomic int32_t*)&tlog->queue[index].state, LOG_ENTRY_STATE_NOT_USED);                          \
            result = TLOG_POP_OK;                                                                                                                 \
        } \
        srw_lock_ll_release_shared(tlog->chunks_lock); \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

/*macro to be used in headers*/                                                                                                     \
#define TLOG_LL_TYPE_DECLARE(C, T, ...)                                                                                           \
    /*hint: have TLOG_DEFINE_STRUCT_TYPE(T) before TLOG_LL_TYPE_DECLARE*/                                                       \
    THANDLE_LL_TYPE_DECLARE(TLOG_TYPEDEF_NAME(C), TLOG_TYPEDEF_NAME(T))                                                         \
    TLOG_LL_CREATE_DECLARE(C, T)                                                                                                  \
    TLOG_LL_PUSH_DECLARE(C, T)                                                                                                    \
    TLOG_LL_POP_DECLARE(C, T)                                                                                                     \

/*macro to be used in .c*/                                                                                                          \
#define TLOG_LL_TYPE_DEFINE(C, T, ...)                                                                                            \
    /*hint: have THANDLE_TYPE_DEFINE(TLOG_TYPEDEF_NAME(T)) before TLOG_LL_TYPE_DEFINE*/                                         \
    TLOG_LL_FREE_DEFINE(C, T)                                                                                                     \
    TLOG_LL_CREATE_DEFINE(C, T)                                                                                                   \
    TLOG_LL_PUSH_DEFINE(C, T)                                                                                                     \
    TLOG_LL_POP_DEFINE(C, T)                                                                                                      \

#endif  /*TLOG_LL_H*/
