// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef TQUEUE_LL_H
#define TQUEUE_LL_H

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

#define TQUEUE_PUSH_RESULT_VALUES \
    TQUEUE_PUSH_OK, \
    TQUEUE_PUSH_INVALID_ARG, \
    TQUEUE_PUSH_QUEUE_FULL, \
    TQUEUE_PUSH_ERROR \

MU_DEFINE_ENUM(TQUEUE_PUSH_RESULT, TQUEUE_PUSH_RESULT_VALUES);

#define TQUEUE_POP_RESULT_VALUES \
    TQUEUE_POP_OK, \
    TQUEUE_POP_INVALID_ARG, \
    TQUEUE_POP_QUEUE_EMPTY, \
    TQUEUE_POP_REJECTED

MU_DEFINE_ENUM(TQUEUE_POP_RESULT, TQUEUE_POP_RESULT_VALUES);

#define QUEUE_ENTRY_STATE_VALUES \
    QUEUE_ENTRY_STATE_NOT_USED, \
    QUEUE_ENTRY_STATE_PUSHING, \
    QUEUE_ENTRY_STATE_USED, \
    QUEUE_ENTRY_STATE_POPPING \

MU_DEFINE_ENUM(QUEUE_ENTRY_STATE, QUEUE_ENTRY_STATE_VALUES);

/*TQUEUE is backed by a THANDLE build on the structure below*/
#define TQUEUE_STRUCT_TYPE_NAME_TAG(T) MU_C2(TQUEUE_TYPEDEF_NAME(T), _TAG)
#define TQUEUE_TYPEDEF_NAME(T) MU_C2(TQUEUE_STRUCT_, T)

#define TQUEUE_ENTRY_STRUCT_TYPE_NAME_TAG(T) MU_C2(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T), _TAG)
#define TQUEUE_ENTRY_STRUCT_TYPE_NAME(T) MU_C2(TQUEUE_ENTRY_STRUCT_, T)

/* This introduces the name for the copy item function */
#define TQUEUE_DEFINE_COPY_ITEM_FUNCTION_TYPE_NAME(T) MU_C2(TQUEUE_COPY_ITEM_FUNC_TYPE_, T)
#define TQUEUE_COPY_ITEM_FUNC(T) TQUEUE_DEFINE_COPY_ITEM_FUNCTION_TYPE_NAME(T)

/* This introduces the name for the dispose item function */
#define TQUEUE_DEFINE_DISPOSE_ITEM_FUNCTION_TYPE_NAME(T) MU_C2(TQUEUE_DISPOSE_ITEM_FUNC_TYPE_, T)
#define TQUEUE_DISPOSE_ITEM_FUNC(T) TQUEUE_DEFINE_DISPOSE_ITEM_FUNCTION_TYPE_NAME(T)

/* This introduces the name for the pop condition function */
#define TQUEUE_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T) MU_C2(TQUEUE_CONDITION_FUNC_TYPE_, T)
#define TQUEUE_CONDITION_FUNC(T) TQUEUE_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T)

/*TQUEUE_DEFINE_STRUCT_TYPE(T) introduces the base type that holds the queue typed as T*/
#define TQUEUE_DEFINE_STRUCT_TYPE(T)                                                                            \
typedef void (*TQUEUE_DEFINE_COPY_ITEM_FUNCTION_TYPE_NAME(T))(void* context, T* dst, T* src);                   \
typedef void (*TQUEUE_DEFINE_DISPOSE_ITEM_FUNCTION_TYPE_NAME(T))(void* context, T* item);                       \
typedef bool (*TQUEUE_DEFINE_CONDITION_FUNCTION_TYPE_NAME(T))(void* context, T* item);                          \
typedef struct TQUEUE_ENTRY_STRUCT_TYPE_NAME_TAG(T)                                                             \
{                                                                                                               \
    union                                                                                                       \
    {                                                                                                           \
        volatile_atomic int32_t state;                                                                          \
        volatile_atomic QUEUE_ENTRY_STATE state_as_enum;                                                        \
    };                                                                                                          \
    T value;                                                                                                    \
} TQUEUE_ENTRY_STRUCT_TYPE_NAME(T);                                                                             \
typedef struct TQUEUE_STRUCT_TYPE_NAME_TAG(T)                                                                   \
{                                                                                                               \
    volatile_atomic int64_t head;                                                                               \
    volatile_atomic int64_t tail;                                                                               \
    TQUEUE_COPY_ITEM_FUNC(T) copy_item_function;                                                                \
    TQUEUE_DISPOSE_ITEM_FUNC(T) dispose_item_function;                                                          \
    void* dispose_item_function_context;                                                                        \
    uint32_t queue_size;                                                                                        \
    uint32_t max_size;                                                                                          \
    SRW_LOCK_LL resize_lock;                                                                                    \
    TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)* queue;                                                                    \
} TQUEUE_TYPEDEF_NAME(T);                                                                                       \

/*TQUEUE is-a THANDLE*/
/*given a type "T" TQUEUE_LL(T) expands to the name of the type. */
#define TQUEUE_LL(T) THANDLE(TQUEUE_TYPEDEF_NAME(T))

/*because TQUEUE is a THANDLE, all THANDLE's macro APIs are useable with TQUEUE.*/
/*the below are just shortcuts of THANDLE's public ones*/
#define TQUEUE_LL_INITIALIZE(T) THANDLE_INITIALIZE(TQUEUE_TYPEDEF_NAME(T))
#define TQUEUE_LL_ASSIGN(T) THANDLE_ASSIGN(TQUEUE_TYPEDEF_NAME(T))
#define TQUEUE_LL_MOVE(T) THANDLE_MOVE(TQUEUE_TYPEDEF_NAME(T))
#define TQUEUE_LL_INITIALIZE_MOVE(T) THANDLE_INITIALIZE_MOVE(TQUEUE_TYPEDEF_NAME(T))

/*introduces a new name for a function that returns a TQUEUE_LL(T)*/
#define TQUEUE_LL_CREATE_NAME(C) MU_C2(TQUEUE_LL_CREATE_, C)
#define TQUEUE_LL_CREATE(C) TQUEUE_LL_CREATE_NAME(C)

/*introduces a new name for the push function */
#define TQUEUE_LL_PUSH_NAME(C) MU_C2(TQUEUE_LL_PUSH_, C)
#define TQUEUE_LL_PUSH(C) TQUEUE_LL_PUSH_NAME(C)

/*introduces a new name for the pop function */
#define TQUEUE_LL_POP_NAME(C) MU_C2(TQUEUE_LL_POP_, C)
#define TQUEUE_LL_POP(C) TQUEUE_LL_POP_NAME(C)

/*introduces a new name for the get_volatile_count function */
#define TQUEUE_LL_GET_VOLATILE_COUNT_NAME(C) MU_C2(TQUEUE_LL_GET_VOLATILE_COUNT_, C)
#define TQUEUE_LL_GET_VOLATILE_COUNT(C) TQUEUE_LL_GET_VOLATILE_COUNT_NAME(C)

/*introduces a function declaration for tqueue_create*/
#define TQUEUE_LL_CREATE_DECLARE(C, T) MOCKABLE_FUNCTION(, TQUEUE_LL(T), TQUEUE_LL_CREATE(C), uint32_t, initial_queue_size, uint32_t, max_queue_size, TQUEUE_COPY_ITEM_FUNC(T), copy_item_function, TQUEUE_DISPOSE_ITEM_FUNC(T), dispose_item_function, void*, dispose_item_function_context);

/*introduces a function declaration for tqueue_push*/
#define TQUEUE_LL_PUSH_DECLARE(C, T) MOCKABLE_FUNCTION(, TQUEUE_PUSH_RESULT, TQUEUE_LL_PUSH(C), TQUEUE_LL(T), tqueue, T*, item, void*, copy_item_function_context);

/*introduces a function declaration for tqueue_pop*/
#define TQUEUE_LL_POP_DECLARE(C, T) MOCKABLE_FUNCTION(, TQUEUE_POP_RESULT, TQUEUE_LL_POP(C), TQUEUE_LL(T), tqueue, T*, item, void*, copy_item_function_context, TQUEUE_CONDITION_FUNC(T), condition_function, void*, condition_function_context);

/*introduces a function declaration for tqueue_get_volatile_count*/
#define TQUEUE_LL_GET_VOLATILE_COUNT_DECLARE(C, T) MOCKABLE_FUNCTION(, int64_t, TQUEUE_LL_GET_VOLATILE_COUNT(C), TQUEUE_LL(T), tqueue);

/*introduces a name for the function that free's a TQUEUE when it's ref count got to 0*/
#define TQUEUE_LL_FREE_NAME(C) MU_C2(TQUEUE_LL_FREE_, C)

/*introduces a function definition for freeing the allocated resources for a TQUEUE*/
#define TQUEUE_LL_FREE_DEFINE(C, T) \
static void TQUEUE_LL_FREE_NAME(C)(TQUEUE_TYPEDEF_NAME(T)* tqueue)                                                                                                  \
{                                                                                                                                                                   \
    if (tqueue == NULL)                                                                                                                                             \
    {                                                                                                                                                               \
        LogError("invalid arguments " MU_TOSTRING(TQUEUE_TYPEDEF_NAME(T)) "* tqueue=%p",                                                                            \
            tqueue);                                                                                                                                                \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        if (tqueue->dispose_item_function == NULL)                                                                                                                  \
        {                                                                                                                                                           \
            /* Codes_SRS_TQUEUE_01_008: [ If dispose_item_function passed to TQUEUE_CREATE(T) is NULL, TQUEUE_DISPOSE_FUNC(T) shall return. ]*/                     \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /* Codes_SRS_TQUEUE_01_009: [ Otherwise, TQUEUE_DISPOSE_FUNC(T) shall obtain the current queue head by calling interlocked_add_64. ]*/                  \
            int64_t current_head = interlocked_add_64((volatile_atomic int64_t*)&tqueue->head, 0);                                                                  \
            /* Codes_SRS_TQUEUE_01_010: [ TQUEUE_DISPOSE_FUNC(T) shall obtain the current queue tail by calling interlocked_add_64. ]*/                             \
            int64_t current_tail = interlocked_add_64((volatile_atomic int64_t*)&tqueue->tail, 0);                                                                  \
            for (int64_t pos = current_tail; pos < current_head; pos++)                                                                                             \
            {                                                                                                                                                       \
                uint32_t index = (uint32_t)(pos % tqueue->queue_size);                                                                                              \
                /* Codes_SRS_TQUEUE_01_011: [ For each item in the queue, dispose_item_function shall be called with dispose_function_context and a pointer to the array entry value (T*). ]*/ \
                tqueue->dispose_item_function(tqueue->dispose_item_function_context, &tqueue->queue[index].value);                                                  \
            }                                                                                                                                                       \
        }                                                                                                                                                           \
        /* Codes_SRS_TQUEUE_01_056: [ The lock initialized in TQUEUE_CREATE(T) shall be de-initialized. ] */                                                        \
        srw_lock_ll_deinit(&tqueue->resize_lock);                                                                                                                   \
        /* Codes_SRS_TQUEUE_01_057: [ The array backing the queue shall be freed. ] */                                                                              \
        free(tqueue->queue);                                                                                                                                        \
    }                                                                                                                                                               \
}                                                                                                                                                                   \

/*introduces a function definition for tqueue_create*/
#define TQUEUE_LL_CREATE_DEFINE(C, T)                                                                                                                      \
TQUEUE_LL(T) TQUEUE_LL_CREATE(C)(uint32_t initial_queue_size, uint32_t max_queue_size, TQUEUE_COPY_ITEM_FUNC(T) copy_item_function, TQUEUE_DISPOSE_ITEM_FUNC(T) dispose_item_function, void* dispose_item_function_context) \
{                                                                                                                                                                   \
    TQUEUE_TYPEDEF_NAME(T)* result;                                                                                                                                 \
    bool is_copy_item_function_NULL = (copy_item_function == NULL);                                                                                                 \
    bool is_dispose_item_function_NULL = (dispose_item_function == NULL);                                                                                           \
    if (                                                                                                                                                            \
        /* Codes_SRS_TQUEUE_01_046: [ If initial_queue_size is 0, TQUEUE_CREATE(T) shall fail and return NULL. ]*/                                                  \
        (initial_queue_size == 0) ||                                                                                                                                \
        /* Codes_SRS_TQUEUE_01_047: [ If initial_queue_size is greater than max_queue_size, TQUEUE_CREATE(T) shall fail and return NULL. ]*/                        \
        (initial_queue_size > max_queue_size) ||                                                                                                                    \
        /* Codes_SRS_TQUEUE_01_048: [ If any of copy_item_function and dispose_item_function are NULL and at least one of them is not NULL, TQUEUE_CREATE(T) shall fail and return NULL. ]*/ \
        ((is_copy_item_function_NULL || is_dispose_item_function_NULL) &&                                                                                           \
         !(is_copy_item_function_NULL && is_dispose_item_function_NULL))                                                                                            \
       )                                                                                                                                                            \
    {                                                                                                                                                               \
        LogError("Invalid arguments: uint32_t initial_queue_size=%" PRIu32 ", uint32_t max_queue_size=%" PRIu32 ", " MU_TOSTRING(TQUEUE_COPY_ITEM_FUNC(T)) " copy_item_function=%p, " MU_TOSTRING(TQUEUE_DISPOSE_ITEM_FUNC(T)) " dispose_item_function=%p, void* dispose_item_function_context=%p", \
            initial_queue_size, max_queue_size, copy_item_function, dispose_item_function, dispose_item_function_context);                                          \
        result = NULL;                                                                                                                                              \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /* Codes_SRS_TQUEUE_01_049: [ TQUEUE_CREATE(T) shall call THANDLE_MALLOC with TQUEUE_DISPOSE_FUNC(T) as dispose function. ] */                              \
        result = THANDLE_MALLOC(TQUEUE_TYPEDEF_NAME(C))(TQUEUE_LL_FREE_NAME(C));                                                                                    \
        if (result == NULL)                                                                                                                                         \
        {                                                                                                                                                           \
            /* Codes_SRS_TQUEUE_01_071: [ If there are any failures then TQUEUE_CREATE(T) shall fail and return NULL. ]*/                                           \
            LogError("failure in " MU_TOSTRING(THANDLE_MALLOC(TQUEUE_TYPEDEF_NAME(C))) "");                                                                         \
            /*return as is*/                                                                                                                                        \
        }                                                                                                                                                           \
        else                                                                                                                                                        \
        {                                                                                                                                                           \
            /* Codes_SRS_TQUEUE_01_050: [ TQUEUE_CREATE(T) shall allocate memory for an array of size size containing elements of type T. ] */                      \
            result->queue = malloc_2(initial_queue_size, sizeof(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)));                                                                 \
            if (result->queue == NULL)                                                                                                                              \
            {                                                                                                                                                       \
                /* Codes_SRS_TQUEUE_01_071: [ If there are any failures then TQUEUE_CREATE(T) shall fail and return NULL. ]*/                                       \
                LogError("failure in malloc_2(%" PRIu32 ", sizeof(" MU_TOSTRING(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)) ")=%zu)",                                         \
                    initial_queue_size, sizeof(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)));                                                                                  \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                result->queue_size = initial_queue_size;                                                                                                            \
                result->max_size = max_queue_size;                                                                                                                  \
                result->copy_item_function = copy_item_function;                                                                                                    \
                result->dispose_item_function = dispose_item_function;                                                                                              \
                result->dispose_item_function_context = dispose_item_function_context;                                                                              \
                /* Codes_SRS_TQUEUE_01_051: [ TQUEUE_CREATE(T) shall initialize the head and tail of the list with 0 by using interlocked_exchange_64. ] */         \
                (void)interlocked_exchange_64(&result->head, 0);                                                                                                    \
                (void)interlocked_exchange_64(&result->tail, 0);                                                                                                    \
                for (uint32_t i = 0; i < initial_queue_size; i++)                                                                                                   \
                {                                                                                                                                                   \
                    /* Codes_SRS_TQUEUE_01_052: [ TQUEUE_CREATE(T) shall initialize the state for each entry in the array used for the queue with NOT_USED by using interlocked_exchange. ] */ \
                    (void)interlocked_exchange(&result->queue[i].state, QUEUE_ENTRY_STATE_NOT_USED);                                                                \
                }                                                                                                                                                   \
                /* Codes_SRS_TQUEUE_01_053: [ TQUEUE_CREATE(T) shall initialize a SRW_LOCK_LL to be used for locking the queue when it needs to grow in size. ] */  \
                (void)srw_lock_ll_init(&result->resize_lock);                                                                                                       \
                /* Codes_SRS_TQUEUE_01_054: [ TQUEUE_CREATE(T) shall succeed and return a non-NULL value. ] */                                                      \
                /*return as is*/                                                                                                                                    \
                goto all_ok;                                                                                                                                        \
            }                                                                                                                                                       \
            THANDLE_FREE(TQUEUE_TYPEDEF_NAME(C))(result);                                                                                                           \
            result = NULL;                                                                                                                                          \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
all_ok:                                                                                                                                                             \
    return result;                                                                                                                                                  \
}

/*introduces a function definition for tqueue_push*/
#define TQUEUE_LL_PUSH_DEFINE(C, T)                                                                                                                                 \
TQUEUE_PUSH_RESULT TQUEUE_LL_PUSH(C)(TQUEUE_LL(T) tqueue, T* item, void* copy_item_function_context)                                                                \
{                                                                                                                                                                   \
    TQUEUE_PUSH_RESULT result;                                                                                                                                      \
    if (                                                                                                                                                            \
        /* Codes_SRS_TQUEUE_01_012: [ If tqueue is NULL then TQUEUE_PUSH(T) shall fail and return TQUEUE_PUSH_INVALID_ARG. ]*/                                      \
        (tqueue == NULL) ||                                                                                                                                         \
        /* Codes_SRS_TQUEUE_01_013: [ If item is NULL then TQUEUE_PUSH(T) shall fail and return TQUEUE_PUSH_INVALID_ARG. ]*/                                        \
        (item == NULL)                                                                                                                                              \
       )                                                                                                                                                            \
    {                                                                                                                                                               \
        LogError("Invalid arguments: TQUEUE_LL(" MU_TOSTRING(T) ") tqueue=%p, const " MU_TOSTRING(T) "* item=%p, void* copy_item_function_context=%p",              \
            tqueue, item, copy_item_function_context);                                                                                                              \
        result = TQUEUE_PUSH_INVALID_ARG;                                                                                                                           \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        TQUEUE_TYPEDEF_NAME(T)* tqueue_ptr = THANDLE_GET_T(TQUEUE_TYPEDEF_NAME(T))(tqueue);                                                                         \
        bool unlock_needed = true;                                                                                                                                  \
        /* Codes_SRS_TQUEUE_01_058: [ TQUEUE_PUSH(T) shall acquire in shared mode the lock used to guard the growing of the queue. ] */                             \
        srw_lock_ll_acquire_shared(&tqueue_ptr->resize_lock);                                                                                                       \
        /* Codes_SRS_TQUEUE_01_014: [ TQUEUE_PUSH(T) shall execute the following actions until it is either able to push the item in the queue or the queue is full: ]*/ \
        do                                                                                                                                                          \
        {                                                                                                                                                           \
            /* Codes_SRS_TQUEUE_01_015: [ TQUEUE_PUSH(T) shall obtain the current head queue by calling interlocked_add_64. ]*/                                     \
            int64_t current_head = interlocked_add_64(&tqueue_ptr->head, 0);                                                                                        \
            /* Codes_SRS_TQUEUE_01_016: [ TQUEUE_PUSH(T) shall obtain the current tail queue by calling interlocked_add_64. ]*/                                     \
            int64_t current_tail = interlocked_add_64(&tqueue_ptr->tail, 0);                                                                                        \
            /* Codes_SRS_TQUEUE_01_060: [ If the queue is full (current head >= current tail + queue size): ]*/                                                     \
            if (current_head >= current_tail + tqueue_ptr->queue_size)                                                                                              \
            {                                                                                                                                                       \
                /* greater cannot really happen */                                                                                                                  \
                if (tqueue_ptr->queue_size >= tqueue_ptr->max_size)                                                                                                 \
                {                                                                                                                                                   \
                    /* Codes_SRS_TQUEUE_01_061: [ If the current queue size is equal to the max queue size, TQUEUE_PUSH(T) shall return TQUEUE_PUSH_QUEUE_FULL. ]*/ \
                    result = TQUEUE_PUSH_QUEUE_FULL;                                                                                                                \
                    break;                                                                                                                                          \
                }                                                                                                                                                   \
                else                                                                                                                                                \
                {                                                                                                                                                   \
                    /* Codes_SRS_TQUEUE_01_062: [ If the current queue size is less than the max queue size: ] */                                                   \
                    /* Codes_SRS_TQUEUE_01_063: [ TQUEUE_PUSH(T) shall release in shared mode the lock used to guard the growing of the queue. ] */                 \
                    srw_lock_ll_release_shared(&tqueue_ptr->resize_lock);                                                                                           \
                    /* Codes_SRS_TQUEUE_01_064: [ TQUEUE_PUSH(T) shall acquire in exclusive mode the lock used to guard the growing of the queue. ] */              \
                    srw_lock_ll_acquire_exclusive(&tqueue_ptr->resize_lock);                                                                                        \
                    /* Codes_SRS_TQUEUE_01_075: [ TQUEUE_PUSH(T) shall obtain again the current head or the queue. ]*/                                              \
                    current_head = interlocked_add_64(&tqueue_ptr->head, 0);                                                                                        \
                    /* Codes_SRS_TQUEUE_01_076: [ TQUEUE_PUSH(T) shall obtain again the current tail or the queue. ]*/                                              \
                    current_tail = interlocked_add_64(&tqueue_ptr->tail, 0);                                                                                        \
                    /* Codes_SRS_TQUEUE_01_074: [ If the size of the queue did not change after acquiring the lock in shared mode: ]*/                              \
                    if (current_head < current_tail + tqueue_ptr->queue_size)                                                                                       \
                    {                                                                                                                                               \
                        /* queue was resized by another thread or now there's space, do nothing */                                                                  \
                    }                                                                                                                                               \
                    else                                                                                                                                            \
                    {                                                                                                                                               \
                        /* Codes_SRS_TQUEUE_01_067: [ TQUEUE_PUSH(T) shall double the size of the queue. ]*/                                                        \
                        uint32_t new_queue_size = tqueue_ptr->queue_size * 2;                                                                                       \
                        if (new_queue_size > tqueue_ptr->max_size)                                                                                                  \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_070: [ If the newly computed queue size is higher than the max_queue_size value passed to TQUEUE_CREATE(T), TQUEUE_PUSH(T) shall use max_queue_size as the new queue size. ]*/ \
                            new_queue_size = tqueue_ptr->max_size;                                                                                                  \
                        }                                                                                                                                           \
                        /* Codes_SRS_TQUEUE_01_068: [ TQUEUE_PUSH(T) shall reallocate the array used to store the queue items based on the newly computed size. ]*/ \
                        TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)* temp_queue = realloc_2(tqueue_ptr->queue, new_queue_size, sizeof(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)));      \
                        if (temp_queue == NULL)                                                                                                                     \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_069: [ If reallocation fails, TQUEUE_PUSH(T) shall return TQUEUE_PUSH_ERROR. ]*/                                 \
                            LogError("realloc_2(tqueue_ptr->queue=%p, new_queue_size=%" PRIu32 ", sizeof(" MU_TOSTRING(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)) ")=%zu) failed", \
                                tqueue_ptr->queue, new_queue_size, sizeof(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)));                                                       \
                            result = TQUEUE_PUSH_ERROR;                                                                                                             \
                            /* Codes_SRS_TQUEUE_01_065: [ TQUEUE_PUSH(T) shall release in exclusive mode the lock used to guard the growing of the queue. ] */      \
                            srw_lock_ll_release_exclusive(&tqueue_ptr->resize_lock);                                                                                \
                            /* No need to take the lock again, we are going to return anyway */                                                                     \
                            unlock_needed = false;                                                                                                                  \
                            break;                                                                                                                                  \
                        }                                                                                                                                           \
                        else                                                                                                                                        \
                        {                                                                                                                                           \
                            tqueue_ptr->queue = temp_queue;                                                                                                         \
                            /* Codes_SRS_TQUEUE_01_077: [ TQUEUE_PUSH(T) shall move the entries between the tail index and the array end like below: ]*/            \
                            uint32_t elements_in_queue = (uint32_t)(current_head - current_tail);                                                                   \
                            uint32_t tail_index = current_tail % tqueue_ptr->queue_size;                                                                            \
                            uint32_t copy_item_count = tqueue_ptr->queue_size - tail_index;                                                                         \
                            uint32_t new_tail_index = new_queue_size - copy_item_count;                                                                             \
                            /* Codes_SRS_TQUEUE_01_078: [ Entries at the tail shall be moved to the end of the resized array ]*/                                    \
                            /* Please see diagram in the spec */                                                                                                    \
                            if (copy_item_count > 0)                                                                                                                \
                            {                                                                                                                                       \
                                (void)memmove(&tqueue_ptr->queue[new_tail_index], &tqueue_ptr->queue[tail_index], sizeof(TQUEUE_ENTRY_STRUCT_TYPE_NAME(T)) * copy_item_count); \
                            }                                                                                                                                       \
                            for (uint32_t i = tail_index; i < new_tail_index; i++)                                                                                  \
                            {                                                                                                                                       \
                                (void)interlocked_exchange(&tqueue_ptr->queue[i].state, QUEUE_ENTRY_STATE_NOT_USED);                                                \
                            }                                                                                                                                       \
                            (void)interlocked_exchange_64(&tqueue_ptr->tail, new_tail_index);                                                                       \
                            (void)interlocked_exchange_64(&tqueue_ptr->head, new_tail_index + elements_in_queue);                                                   \
                            tqueue_ptr->queue_size = new_queue_size;                                                                                                \
                        }                                                                                                                                           \
                    }                                                                                                                                               \
                                                                                                                                                                    \
                    /* Codes_SRS_TQUEUE_01_065: [ TQUEUE_PUSH(T) shall release in exclusive mode the lock used to guard the growing of the queue. ] */              \
                    srw_lock_ll_release_exclusive(&tqueue_ptr->resize_lock);                                                                                        \
                    /* Codes_SRS_TQUEUE_01_066: [ TQUEUE_PUSH(T) shall acquire in shared mode the lock used to guard the growing of the queue. ] */                 \
                    srw_lock_ll_acquire_shared(&tqueue_ptr->resize_lock);                                                                                           \
                    continue;                                                                                                                                       \
                }                                                                                                                                                   \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                uint32_t index = (uint32_t)(current_head % tqueue_ptr->queue_size);                                                                                 \
                /* Codes_SRS_TQUEUE_01_017: [ Using interlocked_compare_exchange, TQUEUE_PUSH(T) shall change the head array entry state to PUSHING (from NOT_USED). ]*/ \
                if (interlocked_compare_exchange(&tqueue_ptr->queue[index].state, QUEUE_ENTRY_STATE_PUSHING, QUEUE_ENTRY_STATE_NOT_USED) != QUEUE_ENTRY_STATE_NOT_USED) \
                {                                                                                                                                                   \
                    /* Codes_SRS_TQUEUE_01_023: [ If the state of the array entry corresponding to the head is not NOT_USED, TQUEUE_PUSH(T) shall retry the whole push. ]*/ \
                    /* likely queue full */                                                                                                                         \
                    continue;                                                                                                                                       \
                }                                                                                                                                                   \
                else                                                                                                                                                \
                {                                                                                                                                                   \
                    /* Codes_SRS_TQUEUE_01_018: [ Using interlocked_compare_exchange_64, TQUEUE_PUSH(T) shall replace the head value with the head value obtained earlier + 1. ]*/ \
                    if (interlocked_compare_exchange_64(&tqueue_ptr->head, current_head + 1, current_head) != current_head)                                         \
                    {                                                                                                                                               \
                        /* Codes_SRS_TQUEUE_01_043: [ If the queue head has changed, TQUEUE_PUSH(T) shall set the state back to NOT_USED and retry the push. ]*/    \
                        (void)interlocked_exchange(&tqueue_ptr->queue[index].state, QUEUE_ENTRY_STATE_NOT_USED);                                                    \
                        continue;                                                                                                                                   \
                    }                                                                                                                                               \
                    else                                                                                                                                            \
                    {                                                                                                                                               \
                        if (tqueue_ptr->copy_item_function == NULL)                                                                                                 \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_019: [ If no copy_item_function was specified in TQUEUE_CREATE(T), TQUEUE_PUSH(T) shall copy the value of item into the array entry value whose state was changed to PUSHING. ]*/ \
                            (void)memcpy((void*)&tqueue_ptr->queue[index].value, (void*)item, sizeof(T));                                                           \
                        }                                                                                                                                           \
                        else                                                                                                                                        \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_024: [ If a copy_item_function was specified in TQUEUE_CREATE(T), TQUEUE_PUSH(T) shall call the copy_item_function with copy_item_function_context as context, a pointer to the array entry value whose state was changed to PUSHING as push_dst and item as push_src. ] */ \
                            tqueue_ptr->copy_item_function(copy_item_function_context, &tqueue_ptr->queue[index].value, item);                                      \
                        }                                                                                                                                           \
                        /* Codes_SRS_TQUEUE_01_020: [ TQUEUE_PUSH(T) shall set the state to USED by using interlocked_exchange. ]*/                                 \
                        (void)interlocked_exchange(&tqueue_ptr->queue[index].state, QUEUE_ENTRY_STATE_USED);                                                        \
                        /* Codes_SRS_TQUEUE_01_021: [ TQUEUE_PUSH(T) shall succeed and return TQUEUE_PUSH_OK. ]*/                                                   \
                        result = TQUEUE_PUSH_OK;                                                                                                                    \
                        break;                                                                                                                                      \
                    }                                                                                                                                               \
                }                                                                                                                                                   \
            }                                                                                                                                                       \
        } while (1);                                                                                                                                                \
                                                                                                                                                                    \
        /* Codes_SRS_TQUEUE_01_059: [ TQUEUE_PUSH(T) shall release in shared mode the lock used to guard the growing of the queue. ] */                             \
        if (unlock_needed)                                                                                                                                          \
        {                                                                                                                                                           \
            srw_lock_ll_release_shared(&tqueue_ptr->resize_lock);                                                                                                   \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

/*introduces a function definition for tqueue_pop*/
#define TQUEUE_LL_POP_DEFINE(C, T)                                                                                                                                  \
TQUEUE_POP_RESULT TQUEUE_LL_POP(C)(TQUEUE_LL(T) tqueue, T* item, void* copy_item_function_context, TQUEUE_CONDITION_FUNC(T) condition_function, void* condition_function_context) \
{                                                                                                                                                                   \
    TQUEUE_POP_RESULT result;                                                                                                                                       \
    if (                                                                                                                                                            \
        /* Codes_SRS_TQUEUE_01_025: [ If tqueue is NULL then TQUEUE_POP(T) shall fail and return TQUEUE_POP_INVALID_ARG. ]*/                                        \
        (tqueue == NULL) ||                                                                                                                                         \
        /* Codes_SRS_TQUEUE_01_027: [ If item is NULL then TQUEUE_POP(T) shall fail and return TQUEUE_POP_INVALID_ARG. ]*/                                          \
        (item == NULL)                                                                                                                                              \
       )                                                                                                                                                            \
    {                                                                                                                                                               \
        LogError("Invalid arguments: TQUEUE_LL(" MU_TOSTRING(T) ") tqueue=%p, " MU_TOSTRING(T) "*=%p, void* copy_item_function_context, TQUEUE_CONDITION_FUNC(T) condition_function, void* condition_function_context", \
            tqueue, item);                                                                                                                                          \
        result = TQUEUE_POP_INVALID_ARG;                                                                                                                            \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        /* Codes_SRS_TQUEUE_01_072: [ TQUEUE_POP(T) shall acquire in shared mode the lock used to guard the growing of the queue. ]*/                               \
        TQUEUE_TYPEDEF_NAME(T)* tqueue_ptr = THANDLE_GET_T(TQUEUE_TYPEDEF_NAME(T))(tqueue);                                                                         \
        srw_lock_ll_acquire_shared(&tqueue_ptr->resize_lock);                                                                                                       \
        {                                                                                                                                                           \
            /* Codes_SRS_TQUEUE_01_026: [ TQUEUE_POP(T) shall execute the following actions until it is either able to pop the item from the queue or the queue is empty: ] */ \
            do                                                                                                                                                      \
            {                                                                                                                                                       \
                /* Codes_SRS_TQUEUE_01_028: [ TQUEUE_POP(T) shall obtain the current head queue by calling interlocked_add_64. ]*/                                  \
                int64_t current_head = interlocked_add_64(&tqueue_ptr->head, 0);                                                                                    \
                /* Codes_SRS_TQUEUE_01_029: [ TQUEUE_POP(T) shall obtain the current tail queue by calling interlocked_add_64. ]*/                                  \
                int64_t current_tail = interlocked_add_64(&tqueue_ptr->tail, 0);                                                                                    \
                if (current_tail >= current_head)                                                                                                                   \
                {                                                                                                                                                   \
                    /* Codes_SRS_TQUEUE_01_035: [ If the queue is empty (current tail >= current head), TQUEUE_POP(T) shall return TQUEUE_POP_QUEUE_EMPTY. ]*/      \
                    result = TQUEUE_POP_QUEUE_EMPTY;                                                                                                                \
                    break;                                                                                                                                          \
                }                                                                                                                                                   \
                else                                                                                                                                                \
                {                                                                                                                                                   \
                    /* Codes_SRS_TQUEUE_01_030: [ Using interlocked_compare_exchange, TQUEUE_PUSH(T) shall set the tail array entry state to POPPING (from USED). ]*/ \
                    uint32_t index = (uint32_t)(current_tail % tqueue_ptr->queue_size);                                                                             \
                    if (interlocked_compare_exchange(&tqueue_ptr->queue[index].state, QUEUE_ENTRY_STATE_POPPING, QUEUE_ENTRY_STATE_USED) != QUEUE_ENTRY_STATE_USED) \
                    {                                                                                                                                               \
                        /* Codes_SRS_TQUEUE_01_036: [ If the state of the array entry corresponding to the tail is not USED, TQUEUE_POP(T) shall try again. ]*/     \
                        continue;                                                                                                                                   \
                    }                                                                                                                                               \
                    else                                                                                                                                            \
                    {                                                                                                                                               \
                        bool should_pop;                                                                                                                            \
                        /* Codes_SRS_TQUEUE_01_039: [ If condition_function is not NULL: ]*/                                                                        \
                        if (condition_function != NULL)                                                                                                             \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_040: [ TQUEUE_POP(T) shall call condition_function with condition_function_context and a pointer to the array entry value whose state was changed to POPPING. ] */ \
                            should_pop = condition_function(condition_function_context, (T*)&tqueue_ptr->queue[index].value);                                       \
                        }                                                                                                                                           \
                        else                                                                                                                                        \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_042: [ Otherwise, shall proceed with the pop. ]*/                                                                \
                            should_pop = true;                                                                                                                      \
                        }                                                                                                                                           \
                        if (!should_pop)                                                                                                                            \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_041: [ If condition_function returns false, TQUEUE_POP(T) shall set the state to USED by using interlocked_exchange and return TQUEUE_POP_REJECTED. ]*/ \
                            (void)interlocked_exchange(&tqueue_ptr->queue[index].state, QUEUE_ENTRY_STATE_USED);                                                    \
                            result = TQUEUE_POP_REJECTED;                                                                                                           \
                        }                                                                                                                                           \
                        else                                                                                                                                        \
                        {                                                                                                                                           \
                            /* Codes_SRS_TQUEUE_01_031: [ TQUEUE_POP(T) shall replace the tail value with the tail value obtained earlier + 1 by using interlocked_exchange_64. ]*/ \
                            if (interlocked_compare_exchange_64(&tqueue_ptr->tail, current_tail + 1, current_tail) != current_tail)                                 \
                            {                                                                                                                                       \
                                /* Codes_SRS_TQUEUE_01_044: [ If incrementing the tail by using interlocked_compare_exchange_64 does not succeed, TQUEUE_POP(T) shall revert the state of the array entry to USED and retry. ]*/ \
                                (void)interlocked_exchange(&tqueue_ptr->queue[index].state, QUEUE_ENTRY_STATE_USED);                                                \
                                continue;                                                                                                                           \
                            }                                                                                                                                       \
                            else                                                                                                                                    \
                            {                                                                                                                                       \
                                if (tqueue_ptr->copy_item_function == NULL)                                                                                         \
                                {                                                                                                                                   \
                                    /* Codes_SRS_TQUEUE_01_032: [ If a copy_item_function was not specified in TQUEUE_CREATE(T): ]*/                                \
                                    /* Codes_SRS_TQUEUE_01_033: [ TQUEUE_POP(T) shall copy array entry value whose state was changed to POPPING to item. ]*/        \
                                    (void)memcpy((void*)item, (void*)&tqueue_ptr->queue[index].value, sizeof(T));                                                   \
                                }                                                                                                                                   \
                                else                                                                                                                                \
                                {                                                                                                                                   \
                                    /* Codes_SRS_TQUEUE_01_037: [ If copy_item_function and sispose_item_function were specified in TQUEUE_CREATE(T): ]*/           \
                                    /* Codes_SRS_TQUEUE_01_038: [ TQUEUE_POP(T) shall call copy_item_function with copy_item_function_context as context, the array entry value whose state was changed to POPPING to item as pop_src and item as pop_dst. ]*/ \
                                    tqueue_ptr->copy_item_function(copy_item_function_context, item, (T*)&tqueue_ptr->queue[index].value);                          \
                                }                                                                                                                                   \
                                if (tqueue_ptr->dispose_item_function != NULL)                                                                                      \
                                {                                                                                                                                   \
                                    /* Codes_SRS_TQUEUE_01_045: [ TQUEUE_POP(T) shall call dispose_item_function with dispose_item_function_context as context and the array entry value whose state was changed to POPPING as item. ]*/ \
                                    tqueue_ptr->dispose_item_function(tqueue_ptr->dispose_item_function_context, (T*)&tqueue_ptr->queue[index].value);              \
                                }                                                                                                                                   \
                                /* Codes_SRS_TQUEUE_01_034: [ TQUEUE_POP(T) shall set the state to NOT_USED by using interlocked_exchange, succeed and return TQUEUE_POP_OK. ]*/ \
                                (void)interlocked_exchange(&tqueue_ptr->queue[index].state, QUEUE_ENTRY_STATE_NOT_USED);                      \
                                result = TQUEUE_POP_OK;                                                                                                             \
                            }                                                                                                                                       \
                        }                                                                                                                                           \
                        break;                                                                                                                                      \
                    }                                                                                                                                               \
                }                                                                                                                                                   \
            } while (1);                                                                                                                                            \
            /* Codes_SRS_TQUEUE_01_073: [ TQUEUE_POP(T) shall release in shared mode the lock used to guard the growing of the queue. ] */                          \
            srw_lock_ll_release_shared(&tqueue_ptr->resize_lock);                                                                                                   \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

/*introduces a function definition for tqueue_pop*/
#define TQUEUE_LL_GET_VOLATILE_COUNT_DEFINE(C, T)                                                                                                                   \
int64_t TQUEUE_LL_GET_VOLATILE_COUNT(C)(TQUEUE_LL(T) tqueue)                                                                                                        \
{                                                                                                                                                                   \
    int64_t result;                                                                                                                                                 \
    /* Codes_SRS_TQUEUE_22_001: [ If tqueue is NULL then TQUEUE_GET_VOLATILE_COUNT(T) shall return zero. ]*/                                                        \
    if (tqueue == NULL)                                                                                                                                             \
    {                                                                                                                                                               \
        LogError("Invalid arguments: TQUEUE_LL(" MU_TOSTRING(T) ") tqueue=%p.", tqueue);                                                                            \
        result = 0;                                                                                                                                                 \
    }                                                                                                                                                               \
    else                                                                                                                                                            \
    {                                                                                                                                                               \
        TQUEUE_TYPEDEF_NAME(T)* tqueue_ptr = THANDLE_GET_T(TQUEUE_TYPEDEF_NAME(T))(tqueue);                                                                         \
        /* Codes_SRS_TQUEUE_01_080: [ TQUEUE_GET_VOLATILE_COUNT(T) shall acquire in shared mode the lock used to guard the growing of the queue. ] */               \
        srw_lock_ll_acquire_shared(&tqueue_ptr->resize_lock);                                                                                                       \
        {                                                                                                                                                           \
            int64_t current_tail = 0;                                                                                                                               \
            int64_t current_head = 0;                                                                                                                               \
            do                                                                                                                                                      \
            {                                                                                                                                                       \
                /* Codes_SRS_TQUEUE_22_003: [ TQUEUE_GET_VOLATILE_COUNT(T) shall obtain the current tail queue by calling interlocked_add_64. ]*/                   \
                current_tail = interlocked_add_64(&tqueue_ptr->tail, 0);                                                                                            \
                /* Codes_SRS_TQUEUE_22_002: [ TQUEUE_GET_VOLATILE_COUNT(T) shall obtain the current head queue by calling interlocked_add_64. ]*/                   \
                current_head = interlocked_add_64(&tqueue_ptr->head, 0);                                                                                            \
                /* Codes_SRS_TQUEUE_22_006: [ TQUEUE_GET_VOLATILE_COUNT(T) shall obtain the current tail queue again by calling interlocked_add_64 and compare with the previosuly obtained tail value.  The tail value is valid only if it has not changed. ]*/   \
            } while (current_tail != interlocked_add_64(&tqueue_ptr->tail, 0));                                                                                     \
                                                                                                                                                                    \
            if (current_tail >= current_head)                                                                                                                       \
            {                                                                                                                                                       \
                /* Codes_SRS_TQUEUE_22_004: [ If the queue is empty (current tail >= current head), TQUEUE_GET_VOLATILE_COUNT(T) shall return zero. ]*/             \
                result = 0;                                                                                                                                         \
            }                                                                                                                                                       \
            else                                                                                                                                                    \
            {                                                                                                                                                       \
                /* Codes_SRS_TQUEUE_22_005: [ TQUEUE_GET_VOLATILE_COUNT(T) shall return the item count of the queue. ]*/                                            \
                result = current_head - current_tail;                                                                                                               \
            }                                                                                                                                                       \
            /* Codes_SRS_TQUEUE_01_081: [ TQUEUE_GET_VOLATILE_COUNT(T) shall release in shared mode the lock used to guard the growing of the queue. ] */           \
            srw_lock_ll_release_shared(&tqueue_ptr->resize_lock);                                                                                                   \
        }                                                                                                                                                           \
    }                                                                                                                                                               \
    return result;                                                                                                                                                  \
}                                                                                                                                                                   \

/*macro to be used in headers*/                                                                                                     \
#define TQUEUE_LL_TYPE_DECLARE(C, T, ...)                                                                                           \
    /*hint: have TQUEUE_DEFINE_STRUCT_TYPE(T) before TQUEUE_LL_TYPE_DECLARE*/                                                       \
    THANDLE_LL_TYPE_DECLARE(TQUEUE_TYPEDEF_NAME(C), TQUEUE_TYPEDEF_NAME(T))                                                         \
    TQUEUE_LL_CREATE_DECLARE(C, T)                                                                                                  \
    TQUEUE_LL_PUSH_DECLARE(C, T)                                                                                                    \
    TQUEUE_LL_POP_DECLARE(C, T)                                                                                                     \
    TQUEUE_LL_GET_VOLATILE_COUNT_DECLARE(C, T)                                                                                      \

/*macro to be used in .c*/                                                                                                          \
#define TQUEUE_LL_TYPE_DEFINE(C, T, ...)                                                                                            \
    /*hint: have THANDLE_TYPE_DEFINE(TQUEUE_TYPEDEF_NAME(T)) before TQUEUE_LL_TYPE_DEFINE*/                                         \
    TQUEUE_LL_FREE_DEFINE(C, T)                                                                                                     \
    TQUEUE_LL_CREATE_DEFINE(C, T)                                                                                                   \
    TQUEUE_LL_PUSH_DEFINE(C, T)                                                                                                     \
    TQUEUE_LL_POP_DEFINE(C, T)                                                                                                      \
    TQUEUE_LL_GET_VOLATILE_COUNT_DEFINE(C, T)                                                                                       \

#endif  /*TQUEUE_LL_H*/
