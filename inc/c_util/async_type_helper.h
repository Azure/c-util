// Copyright (c) Microsoft. All rights reserved.

#ifndef ASYNC_TYPE_HELPER_TYPES_H
#define ASYNC_TYPE_HELPER_TYPES_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdlib>
#else
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"

#include "c_pal/uuid.h"

#include "c_util/constbuffer_array.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_size_t 1

#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_int 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_long 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_float 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_double 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_byte 1

#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_uint8_t 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_int8_t 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_uint16_t 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_int16_t 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_uint32_t 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_int32_t 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_uint64_t 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_int64_t 1

#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_bool 1
#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY__Bool 1

#define ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_void_ptr 1


#define ASYNC_TYPE_HELPER_HAS_CONST_const_charptr_t 1
#define ASYNC_TYPE_HELPER_NON_CONST_TYPE_const_charptr_t charptr_t

#define ASYNC_TYPE_HELPER_USE_CONST_TYPE_charptr_t 1
#define ASYNC_TYPE_HELPER_CONST_TYPE_charptr_t const_charptr_t

#define ASYNC_TYPE_HELPER_HAS_CONST_const_UUID_T 1
#define ASYNC_TYPE_HELPER_NON_CONST_TYPE_const_UUID_T UUID_T

typedef const char* const_charptr_t;
typedef char* charptr_t;
typedef void* void_ptr;
typedef CONSTBUFFER_ARRAY_HANDLE* constbuffer_array_ptr;

/*Codes_SRS_ASYNC_TYPE_HELPER_42_001: [ ASYNC_TYPE_HELPER_COPY_HANDLER shall expand arg_type to the name of the copy handler for the arg_type. ]*/
#define ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type) \
    MU_C3(async_type_helper_, arg_type, _copy)

/*Codes_SRS_ASYNC_TYPE_HELPER_42_014: [ ASYNC_TYPE_HELPER_FREE_HANDLER shall expand arg_type to the name of the free handler for the arg_type. ]*/
#define ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type) \
    MU_C3(async_type_helper_, arg_type, _free)

#define ASYNC_TYPE_HELPER_TEST_1 0

#define ASYNC_TYPE_HELPER_DO_NOT_USE_ASSIGN_COPY(arg_type) MU_C2(ASYNC_TYPE_HELPER_TEST_, MU_C2(ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_, arg_type))

// Helpers to modify the const of the type based on defines for the type

#define ASYNC_TYPE_HELPER_DO_NOT_STRIP_CONST(arg_type) MU_C2(ASYNC_TYPE_HELPER_TEST_, MU_C2(ASYNC_TYPE_HELPER_HAS_CONST_, arg_type))

#define ASYNC_TYPE_HELPER_DO_NOT_ADD_CONST(arg_type) MU_C2(ASYNC_TYPE_HELPER_TEST_, MU_C2(ASYNC_TYPE_HELPER_USE_CONST_TYPE_, arg_type))

#define ASYNC_TYPE_HELPER_USE_TYPE(arg_type) arg_type
#define ASYNC_TYPE_HELPER_USE_NON_CONST_TYPE(arg_type) MU_C2(ASYNC_TYPE_HELPER_NON_CONST_TYPE_, arg_type)
#define ASYNC_TYPE_HELPER_USE_CONST_TYPE(arg_type) MU_C2(ASYNC_TYPE_HELPER_CONST_TYPE_, arg_type)

// Helpers to opt out of having a pointer added to destination

#define ASYNC_TYPE_HELPER_ADD_POINTER_IN_DECLARATION(arg_type) MU_C2(ASYNC_TYPE_HELPER_TEST_, MU_C2(ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_, arg_type))

#define ASYNC_TYPE_HELPER_USE_NONPOINTER_TYPE(arg_type)
#define ASYNC_TYPE_HELPER_USE_POINTER_TYPE(arg_type) *

/*Codes_SRS_ASYNC_TYPE_HELPER_42_019: [ If ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_{type} is defined as 1 then type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(type) shall expand to type. ]*/
/*Codes_SRS_ASYNC_TYPE_HELPER_42_020: [ Otherwise, type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(type) shall expand to type *. ]*/
#define ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(arg_type) \
    MU_IF(ASYNC_TYPE_HELPER_ADD_POINTER_IN_DECLARATION(arg_type), ASYNC_TYPE_HELPER_USE_POINTER_TYPE(arg_type), ASYNC_TYPE_HELPER_USE_NONPOINTER_TYPE(arg_type))

/*Codes_SRS_ASYNC_TYPE_HELPER_42_015: [ If ASYNC_TYPE_HELPER_HAS_CONST_{type} is defined as 1 then ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(type) shall expand to ASYNC_TYPE_HELPER_NON_CONST_TYPE_{type}. ]*/
/*Codes_SRS_ASYNC_TYPE_HELPER_42_017: [ Otherwise, ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(type) shall expand to type. ]*/
#define ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type) \
    MU_EXPAND(MU_IF(ASYNC_TYPE_HELPER_DO_NOT_STRIP_CONST(arg_type), MU_NOEXPAND(ASYNC_TYPE_HELPER_USE_TYPE(arg_type)), MU_NOEXPAND(ASYNC_TYPE_HELPER_USE_NON_CONST_TYPE(arg_type))))

/*Codes_SRS_ASYNC_TYPE_HELPER_42_016: [ If ASYNC_TYPE_HELPER_USE_CONST_TYPE_{type} is defined as 1 then ASYNC_TYPE_HELPER_ADD_CONST_TYPE(type) shall expand to ASYNC_TYPE_HELPER_CONST_TYPE_{type}. ]*/
/*Codes_SRS_ASYNC_TYPE_HELPER_42_018: [ Otherwise, ASYNC_TYPE_HELPER_ADD_CONST_TYPE(type) shall expand to type. ]*/
#define ASYNC_TYPE_HELPER_ADD_CONST_TYPE(arg_type) \
    MU_EXPAND(MU_IF(ASYNC_TYPE_HELPER_DO_NOT_ADD_CONST(arg_type), MU_NOEXPAND(ASYNC_TYPE_HELPER_USE_TYPE(arg_type)), MU_NOEXPAND(ASYNC_TYPE_HELPER_USE_CONST_TYPE(arg_type))))

/*Codes_SRS_ASYNC_TYPE_HELPER_42_002: [ DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER shall expand to: ]*/
#define DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type, destination_arg, source_arg) \
    MOCKABLE_FUNCTION(, int, ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type), ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type)ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(arg_type), destination_arg, ASYNC_TYPE_HELPER_ADD_CONST_TYPE(arg_type), source_arg);

/*Codes_SRS_ASYNC_TYPE_HELPER_42_003: [ DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER shall expand to: ]*/
#define DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type, destination_arg, source_arg) \
    int ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type)(ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type)ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(arg_type) destination_arg, ASYNC_TYPE_HELPER_ADD_CONST_TYPE(arg_type) source_arg)

/*Codes_SRS_ASYNC_TYPE_HELPER_42_004: [ DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER shall expand to: ]*/
#define DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type, arg) \
    MOCKABLE_FUNCTION(, void, ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type), ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type), arg);

/*Codes_SRS_ASYNC_TYPE_HELPER_42_005: [ DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER shall expand to: ]*/
#define DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type, arg) \
    void ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type)(ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type) arg)

DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t, dest, source);
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t, value);

DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T, dest, source);
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T, value);

int ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)(const_UUID_T* destination_arg, const_UUID_T source_arg);
void ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T)(const_UUID_T value);

int constbuffer_array_ptr_copy(constbuffer_array_ptr* dest, const constbuffer_array_ptr src, uint32_t item_count);
void constbuffer_array_ptr_free(const constbuffer_array_ptr value, uint32_t item_count);

#ifdef __cplusplus
}
#endif

#endif // ASYNC_TYPE_HELPER_TYPES_H
