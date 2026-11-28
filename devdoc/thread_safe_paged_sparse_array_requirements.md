# `thread_safe_paged_sparse_array` requirements

## Overview

`THREAD_SAFE_PAGED_SPARSE_ARRAY` wraps a [`PAGED_SPARSE_ARRAY`](paged_sparse_array_requirements.md) so that `ALLOCATE`, `RELEASE` and `ACQUIRE_ITEM` can be called concurrently from multiple threads. `ALLOCATE` copies an item into the array and `ACQUIRE_ITEM` copies it out, so the caller's copy stays valid after the element is released.

`THREAD_SAFE_PAGED_SPARSE_ARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `THREAD_SAFE_PAGED_SPARSE_ARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_MOVE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T)`

## Design

Each element of the wrapped `PAGED_SPARSE_ARRAY` is a slot that holds an item. The wrapped `PAGED_SPARSE_ARRAY` tracks which elements of a page are allocated (the allocation bitmap and the allocated count of the page), and each page has a lock (an `SRW_LOCK_LL`) that protects them:
- `ALLOCATE` and `RELEASE` hold the page lock in exclusive mode while they call `PAGED_SPARSE_ARRAY_ALLOCATE(T)` and `PAGED_SPARSE_ARRAY_RELEASE(T)`, which allocate the page when its first element is allocated and free it when its last element is released.
- `ACQUIRE_ITEM` holds the page lock in shared mode while it calls `PAGED_SPARSE_ARRAY_GET(T)` and copies the item out, so `ACQUIRE_ITEM` calls on the same page run concurrently.
- `RELEASE` moves the item out of the slot before it releases the element, and disposes it after it releases the page lock.

## Threading Model

`ALLOCATE`, `RELEASE` and `ACQUIRE_ITEM` may be called concurrently from any number of threads. The array guarantees:
- `ACQUIRE_ITEM` copies an item only after `ALLOCATE` finished copying it in and before `RELEASE` moves it out, since they hold the page lock (see [Design](#design)).
- `item_copy_func` is called while holding the page lock: in exclusive mode for `ALLOCATE` and in shared mode for `ACQUIRE_ITEM`, it may be called concurrently for the same element.
- `item_dispose_func` is called without holding any lock, it may be called concurrently for different elements.

Callers shall follow these rules:
- Only the owner of an `index` shall release it, once per allocation. The owner is the caller that allocated the `index` or the caller it was handed over to (e.g., a producer allocates an `index` and a consumer releases it).
- `item_copy_func` shall not call `ALLOCATE`, `RELEASE` or `ACQUIRE_ITEM` of any `THREAD_SAFE_PAGED_SPARSE_ARRAY` or wait for an `ALLOCATE` or `RELEASE` of the same array.
- Callers shall synchronize access to what an item points to (e.g., the content of a `THANDLE`): the array only protects its own bookkeeping.

## Exposed API

```c
/*to be used as the type of handle that wraps T*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY(T)

/*custom item copy function type*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T)

/*custom item dispose function type*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T)

/*to be used in a header file*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)
```

The macros expand to these useful APIs:

```c
THREAD_SAFE_PAGED_SPARSE_ARRAY(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T) item_copy_func, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func);
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T* item);
void THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T* item);
```

### THREAD_SAFE_PAGED_SPARSE_ARRAY(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY(T)` is a `THANDLE`(`THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T`), where `THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T` is a structure that holds the wrapped `PAGED_SPARSE_ARRAY` of slots, the custom item copy and dispose functions and the lock of each page.

### THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T)

```c
typedef void (*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T))(T* dst, T* src);
```

The custom item copy function initializes `dst` from `src`. `dst` is not initialized when it is called.

### THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)` is a macro to be used in a header declaration. It introduces the item copy and dispose function types (`THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T)` and `THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T)`), the slot type, the wrapped `PAGED_SPARSE_ARRAY` of slots (with its `THANDLE` declaration) and `THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T`.

### THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)` is a macro to be used in a header declaration.

It introduces the APIs (as MOCKABLE_FUNCTIONS) that can be called for a `THREAD_SAFE_PAGED_SPARSE_ARRAY` and for the wrapped `PAGED_SPARSE_ARRAY` of slots.

Example usage:

```c
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(int32_t)
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(int32_t));
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(int32_t);
```

### THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)` is a macro to be used in a .c file to define all the needed functions for `THREAD_SAFE_PAGED_SPARSE_ARRAY(T)` and for the wrapped `PAGED_SPARSE_ARRAY` of slots.

Example usage:

```c
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(int32_t));
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(int32_t);
```

### THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)

```c
THREAD_SAFE_PAGED_SPARSE_ARRAY(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_COPY_FUNC(T) item_copy_func, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` creates a thread safe paged sparse array with the given maximum size and page size. `item_copy_func` and `item_dispose_func` are optional, but shall be both `NULL` or both not `NULL`: `item_copy_func` copies items into and out of the array (otherwise they are copied as is) and `item_dispose_func` disposes them.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_109: [** If only one of `item_copy_func` and `item_dispose_func` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `PAGED_SPARSE_ARRAY_CREATE(T)` with `max_size`, `page_size` and `NULL` as `item_dispose_func` to create the paged sparse array of slots. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [** If `PAGED_SPARSE_ARRAY_CREATE(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` to allocate the array with a lock for each page. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `srw_lock_ll_init` to initialize the lock of each page. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store `item_dispose_func`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_110: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store `item_copy_func`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store the paged sparse array of slots. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall succeed and return a non-`NULL` value. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [** If there are any other errors, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* paged_sparse_array);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` is the internal dispose function called by `THANDLE` when all references to the thread safe paged sparse array are released.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [** If `item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `item_dispose_func` with a pointer to the item of each allocated element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `srw_lock_ll_deinit` for the lock of each page. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall release the paged sparse array of slots. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)

```c
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T* item);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` allocates the element at `index` and copies `item` into it, while holding the page lock in exclusive mode.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_111: [** If `item` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall acquire the lock of the page of `index` in exclusive mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_132: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `PAGED_SPARSE_ARRAY_ALLOCATE(T)` to allocate the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_133: [** If `PAGED_SPARSE_ARRAY_ALLOCATE(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return the result of `PAGED_SPARSE_ARRAY_ALLOCATE(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_113: [** If `item_copy_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `item_copy_func` with a pointer to the item of the slot as `dst` and `item` as `src`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_114: [** Otherwise, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall copy `item` to the item of the slot. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_116: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall succeed and return `PAGED_SPARSE_ARRAY_ALLOCATE_OK`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall release the page lock in exclusive mode before returning. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)

```c
void THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` releases the element at `index`, which shall be allocated (see [Threading Model](#threading-model)), while holding the page lock in exclusive mode. It disposes the item after it releases the page lock.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall acquire the lock of the page of `index` in exclusive mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` to get the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_134: [** If `PAGED_SPARSE_ARRAY_GET(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall release the page lock and return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_135: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall copy the item of the slot to a local item. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_136: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `PAGED_SPARSE_ARRAY_RELEASE(T)` to release the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall release the page lock. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [** If `item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `item_dispose_func` with a pointer to the local item. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)

```c
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T* item);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` copies the item of the element at `index` to `item`, while holding the page lock in shared mode.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [** If `item` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall acquire the lock of the page of `index` in shared mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` to get the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_137: [** If `PAGED_SPARSE_ARRAY_GET(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall fail and return the result of `PAGED_SPARSE_ARRAY_GET(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_126: [** If `item_copy_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall call `item_copy_func` with `item` as `dst` and a pointer to the item of the slot as `src`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_127: [** Otherwise, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall copy the item of the slot to `item`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_130: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall succeed and return `PAGED_SPARSE_ARRAY_GET_OK`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_131: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ACQUIRE_ITEM(T)` shall release the page lock in shared mode before returning. **]**
