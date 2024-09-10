# `async_type_helpers` requirements

## Overview

`async_type_helper` is a set of macros to be used by [`async_retry_wrapper`](async_retry_wrapper_requirements.md) and [`sync_wrapper`](sync_wrapper_requirements.md).

 - Handling of ref count argument types (or argument types that require copy) is done by allowing the user to define a custom copy handler for out argument values.

 - By default `async_type_helper` supports a given set of types that are always copied by assigning (int, long, uint32_t, etc.).

 - The user can specify whether a type should be copied by assigning by defining `ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type}` to 1. If `ASYNC_TYPE_HELPER_USE_ASSIGN_COPY_{type}` is not defined then the wrapper will call a function with a predefined declaration to copy the argument value.


## Exposed API

```c
#define ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type) \
    ...

#define ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type) \
    ...

#define DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type, destination_arg, source_arg) \
    ...

#define DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type, destination_arg, source_arg) \
    ...

#define ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type) \
    ...

#define DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type, arg) \
    ...

#define DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type, arg) \
    ...
```

## Supported Types

The following types are setup to be copied by assignment by default:
 - `size_t`
 - `int`
 - `long`
 - `float`
 - `double`
 - `byte`
 - `uint8_t`
 - `int8_t`
 - `uint16_t`
 - `int16_t`
 - `uint32_t`
 - `int32_t`
 - `uint64_t`
 - `int64_t`
 - `bool`
 - `void*` (`void_ptr`)

A copy handler is defined for `const char*`:

```c
#define ASYNC_TYPE_HELPER_HAS_CONST_const_charptr_t 1
#define ASYNC_TYPE_HELPER_NON_CONST_TYPE_const_charptr_t charptr_t

typedef const char* const_charptr_t;
typedef char* charptr_t;

DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t, dest, source);
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t, value);
```

A copy handler is defined for `UUID_T`:

```c
DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T, dest, source);
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T, value);
```

A copy handler is defined for `const_UUID_T`:

```c
typedef const UUID_T const_UUID_T;

DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T, dest, source);
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(const_UUID_T, value);
```

A copy handler is defined for `CONSTBUFFER_ARRAY_HANDLE*`:

```c
typedef CONSTBUFFER_ARRAY_HANDLE* constbuffer_array_ptr;

int constbuffer_array_ptr_copy(constbuffer_array_ptr* dest, const constbuffer_array_ptr src, uint32_t item_count);
void constbuffer_array_ptr_free(const constbuffer_array_ptr value, uint32_t item_count);
```

### ASYNC_TYPE_HELPER_STRIP_CONST_TYPE

```c
#define ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(arg_type) \
    ...
```

Helper macro to convert a type to the non-const version, if needed. For example, a function can use `const char*` but a macro may need to copy to a non-const `char*`. In that case, `ASYNC_TYPE_HELPER_HAS_CONST_{type}` must be defined as `1` and `ASYNC_TYPE_HELPER_NON_CONST_TYPE_{type}` must be defined to the non-const type.

**SRS_ASYNC_TYPE_HELPER_42_015: [** If `ASYNC_TYPE_HELPER_HAS_CONST_{type}` is defined as `1` then `ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(type)` shall expand to `ASYNC_TYPE_HELPER_NON_CONST_TYPE_{type}`. **]**

**SRS_ASYNC_TYPE_HELPER_42_017: [** Otherwise, `ASYNC_TYPE_HELPER_STRIP_CONST_TYPE(type)` shall expand to `type`. **]**

### ASYNC_TYPE_HELPER_ADD_CONST_TYPE

```c
#define ASYNC_TYPE_HELPER_ADD_CONST_TYPE(arg_type) \
   ...
```

Helper macro to convert a type to the const version, if needed. For example, a function can use `char*` but a macro may need to generate a function signature with `const char*`. In that case, `ASYNC_TYPE_HELPER_USE_CONST_TYPE_{type}` must be defined as `1` and `ASYNC_TYPE_HELPER_CONST_TYPE_{type}` must be defined to the const type.

**SRS_ASYNC_TYPE_HELPER_42_016: [** If `ASYNC_TYPE_HELPER_USE_CONST_TYPE_{type}` is defined as `1` then `ASYNC_TYPE_HELPER_ADD_CONST_TYPE(type)` shall expand to `ASYNC_TYPE_HELPER_CONST_TYPE_{type}`. **]**

**SRS_ASYNC_TYPE_HELPER_42_018: [** Otherwise, `ASYNC_TYPE_HELPER_ADD_CONST_TYPE(type)` shall expand to `type`. **]**

### ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED

```c
#define ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(arg_type) \
   ...
```

Helper macro to add a pointer to a type unless the type has a define to opt-out. For example, a sync wrapper may have a pointer as an output variable. Rather than returning a pointer-to-a-pointer and allocating memory, it may just take the pointer and copy into it. In that case, `ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_{type}` must be defined as `1`.

**SRS_ASYNC_TYPE_HELPER_42_019: [** If `ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_{type}` is defined as `1` then `type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(type)` shall expand to `type`. **]**

**SRS_ASYNC_TYPE_HELPER_42_020: [** Otherwise, `type ASYNC_TYPE_HELPER_ADD_POINTER_IF_NEEDED(type)` shall expand to `type *`. **]**

### ASYNC_TYPE_HELPER_COPY_HANDLER

```c
#define ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type) \
    ...
```

**SRS_ASYNC_TYPE_HELPER_42_001: [** `ASYNC_TYPE_HELPER_COPY_HANDLER` shall expand `arg_type` to the name of the copy handler for the `arg_type`. **]**

### DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER

```c
#define DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type, destination_arg, source_arg) \
    ...
```

`DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER` expands to the declaration of the copy handler for type `arg_type`.

**SRS_ASYNC_TYPE_HELPER_42_002: [** `DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER` shall expand to: **]**

```c
MOCKABLE_FUNCTION(, int, async_type_helper_{arg_type}_copy, arg_type_non_const_ptr, destination_arg, arg_type_const, source_arg);
```

Where `arg_type_non_const_ptr` is either `arg_type` or, if `ASYNC_TYPE_HELPER_HAS_CONST_{arg_type}` is defined as `1`, expands to `ASYNC_TYPE_HELPER_NON_CONST_TYPE_{arg_type}`.

The type `arg_type_non_const_ptr` shall be a pointer (as in `arg_type*`) unless `ASYNC_TYPE_HELPER_NO_POINTER_DECLARATION_{type}` is defined as `1`.

Where `arg_type_const` is either `arg_type` or, if `ASYNC_TYPE_HELPER_USE_CONST_TYPE_{arg_type}` is defined as `1`, expands to `ASYNC_TYPE_HELPER_CONST_TYPE_{type}`.

### DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER

```c
#define DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER(arg_type, destination_arg, source_arg) \
    ...
```

`DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER` expands to the declaration of the copy handler for type `arg_type` to be placed before its definition.

**SRS_ASYNC_TYPE_HELPER_42_003: [** `DEFINE_ASYNC_TYPE_HELPER_COPY_HANDLER` shall expand to: **]**

```c
int async_type_helper_{arg_type}_copy(arg_type_non_const_ptr destination_arg, arg_type_const source_arg)
```

### ASYNC_TYPE_HELPER_FREE_HANDLER

```c
#define ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type) \
    ...
```

**SRS_ASYNC_TYPE_HELPER_42_014: [** `ASYNC_TYPE_HELPER_FREE_HANDLER` shall expand `arg_type` to the name of the free handler for the `arg_type`. **]**

### DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER

```c
#define DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type, arg) \
    ...
```

`DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER` expands to the declaration of the free handler for type `arg_type`.

**SRS_ASYNC_TYPE_HELPER_42_004: [** `DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER` shall expand to: **]**

```c
MOCKABLE_FUNCTION(, int, async_type_helper_{arg_type}_free, arg_type, arg);
```

### DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER

```c
#define DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER(arg_type, arg) \
    ...
```

`DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER` expands to the declaration of the free handler for type `arg_type` to be placed before its definition.

**SRS_ASYNC_TYPE_HELPER_42_005: [** `DEFINE_ASYNC_TYPE_HELPER_FREE_HANDLER` shall expand to: **]**

```c
int async_type_helper_{arg_type}_free(arg_type arg)
```

### ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t)

```c
DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(const_charptr_t, dest, source);
```

Copies the string in `source` to `dest`.

**SRS_ASYNC_TYPE_HELPER_42_006: [** If `dest` is `NULL` then the copy handler will fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_42_007: [** If `source` is `NULL` then the copy handler will set the `dest` to `NULL` and return 0. **]**

**SRS_ASYNC_TYPE_HELPER_42_008: [** The copy handler shall allocate a string large enough to hold `source`, including the terminating `NULL`. **]**

**SRS_ASYNC_TYPE_HELPER_42_009: [** The copy handler shall copy the string from `source` to `dest`. **]**

**SRS_ASYNC_TYPE_HELPER_42_010: [** If there are any failures then the copy handler shall fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_42_011: [** The copy handler shall succeed and return 0. **]**

### ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t)

```c
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(const_charptr_t, value);
```

Frees a string.

**SRS_ASYNC_TYPE_HELPER_42_012: [** If `value` is `NULL` then the free handler shall return. **]**

**SRS_ASYNC_TYPE_HELPER_42_013: [** The free handler shall free `value`. **]**

### ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T)

```c
DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(UUID_T, dest, source);
```

Copies the `UUID_T` from `source` to `dest`.

**SRS_ASYNC_TYPE_HELPER_04_001: [** If `dest` is `NULL` then the copy handler will fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_04_002: [** If `source` is `NULL` then the copy handler will set the `dest` to a zero UUID value. **]**

**SRS_ASYNC_TYPE_HELPER_04_003: [** Otherwise, the copy handler shall copy the `UUID_T` from `source` to `dest`. **]**

**SRS_ASYNC_TYPE_HELPER_04_004: [** The copy handler shall succeed and return 0. **]**

### ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T)

```c
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T, value);
```

Does nothing since we don't `malloc` a `UUID_T` in the copy handler.

**SRS_ASYNC_TYPE_HELPER_04_005: [** This handler shall do nothing. **]**

### ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T)

```c
DECLARE_ASYNC_TYPE_HELPER_COPY_HANDLER(const_UUID_T, dest, source);
```

Copies the const `UUID_T` from `source` to `dest`.

**SRS_ASYNC_TYPE_HELPER_01_001: [** If `dest` is `NULL` then the copy handler will fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_01_002: [** If `source` is `NULL` then the copy handler will set the `dest` to a zero UUID value. **]**

**SRS_ASYNC_TYPE_HELPER_01_003: [** Otherwise, the copy handler shall copy the const `UUID_T` from `source` to `dest`. **]**

**SRS_ASYNC_TYPE_HELPER_01_004: [** The copy handler shall succeed and return 0. **]**

### ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T)

```c
DECLARE_ASYNC_TYPE_HELPER_FREE_HANDLER(UUID_T, value);
```

Does nothing since we don't `malloc` a `UUID_T` in the copy handler.

**SRS_ASYNC_TYPE_HELPER_01_005: [** This handler shall do nothing. **]**

```c
int constbuffer_array_ptr_copy(constbuffer_array_ptr* dest, const constbuffer_array_ptr src, uint32_t item_count);
```

Copies the constbuffer_arrays in `src` to `dest`.

**SRS_ASYNC_TYPE_HELPER_28_001: [** If `dest` is `NULL` then the copy handler will fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_28_011: [** If `item_count` is 0, the copy handler will fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_28_002: [** If `src` is `NULL` then the copy handler will set the dest to `NULL` and return 0. **]**

**SRS_ASYNC_TYPE_HELPER_28_003: [** The copy handler shall allocate an array to store all the constbuffer_array in the `src`. **]**

**SRS_ASYNC_TYPE_HELPER_28_004: [** If there are any failures then the copy handler shall fail and return a non-zero value. **]**

**SRS_ASYNC_TYPE_HELPER_28_005: [** The copy handler shall call `constbuffer_array_inc_ref` on each constbuffer array in `src`. **]**

**SRS_ASYNC_TYPE_HELPER_28_006: [** The copy handler shall copy all the constbuffer_arrays from the `src` to the `dest`. **]**

**SRS_ASYNC_TYPE_HELPER_28_007: [** The copy handler shall succeed and return 0. **]**

```c
void constbuffer_array_ptr_free(const constbuffer_array_ptr value, uint32_t item_count);
```

Frees the constbuffer_arrays in `value`.

**SRS_ASYNC_TYPE_HELPER_28_008: [** If `value` is `NULL` then the free handler shall return. **]**

**SRS_ASYNC_TYPE_HELPER_28_012: [** If `item_count` is 0, the free handler shall return. **]**

**SRS_ASYNC_TYPE_HELPER_28_009: [** The free handler shall call `constbuffer_array_dec_ref` on each constbuffer array in `value`. **]**

**SRS_ASYNC_TYPE_HELPER_28_010: [** The free handler shall free `value`. **]**