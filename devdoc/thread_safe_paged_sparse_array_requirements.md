# `thread_safe_paged_sparse_array` requirements

## Overview

`THREAD_SAFE_PAGED_SPARSE_ARRAY` is a module that wraps a [`PAGED_SPARSE_ARRAY`](paged_sparse_array_requirements.md) so that `ALLOCATE`, `RELEASE` and `GET` can be called concurrently from multiple threads, without acquiring any lock when the page of the element is allocated.

The indexes, the pages, `max_size`, `page_size` and the result codes are those of the wrapped `PAGED_SPARSE_ARRAY` (see its [Design](paged_sparse_array_requirements.md#design) section). The wrapped `PAGED_SPARSE_ARRAY` is not changed. The custom item dispose function has the same meaning as the one of `PAGED_SPARSE_ARRAY`: it is called for an element when the element is released and, for each element that is still allocated, when the array is disposed. It is called by `THREAD_SAFE_PAGED_SPARSE_ARRAY`, the wrapped `PAGED_SPARSE_ARRAY` is created without an item dispose function.

`THREAD_SAFE_PAGED_SPARSE_ARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `THREAD_SAFE_PAGED_SPARSE_ARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_MOVE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T)`

## Design

Terms:
- **slot**: an element of the wrapped `PAGED_SPARSE_ARRAY`. A slot holds an item of type `T` and the state of the item: `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED` or `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING`. The state is changed only with interlocked operations.
- **page guard**: there is one page guard for each page. It has a use count, a page state and a lock.
- **use count**: the number of allocated elements of the page plus the number of calls that use the page, plus `THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING` (2<sup>62</sup>, larger than any count of elements and calls) while the page is being freed.
- **page state**: `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED` when the page is allocated in the wrapped `PAGED_SPARSE_ARRAY` and its slots can be used, `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING` while the page is being freed and `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` otherwise. The page state is 32 bits because `wait_on_address_64` on Linux compares only 32 bits of the value, so a wait on the 64 bits use count could miss its change.
- **indexes of a page**: the indexes from `page_index * page_size` up to `(page_index + 1) * page_size - 1`. The last page has only the indexes lower than `max_size`.

How the operations work:
- `ALLOCATE`, `RELEASE` and `GET` of an element of an allocated page do not acquire any lock. They increment the use count of the page and call `PAGED_SPARSE_ARRAY_GET(T)` (which only reads the wrapped `PAGED_SPARSE_ARRAY`) to get the slot. `ALLOCATE` and `RELEASE` change the state of the slot with interlocked operations, `GET` only reads it. A successful `ALLOCATE` keeps its use of the page for the allocated element, `RELEASE` subtracts its own use and the use of the released element, the other calls subtract their use before returning.
- A page is allocated by `ALLOCATE` while holding the lock of its page guard: `PAGED_SPARSE_ARRAY_ALLOCATE(T)` is called for each index of the page and all the slots are set to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED`.
- A page is freed when its use count becomes 0. While holding the lock of its page guard, the use count is changed from 0 to `THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING` and the page state is set to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING`. After the lock is released, `PAGED_SPARSE_ARRAY_RELEASE(T)` is called for each index of the page (the last one frees the page), `THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING` is subtracted from the use count, the page state is set to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` and the calls waiting for the page state to change are woken. A call that allocates a page while the page is being freed waits for the page state to change.
- `PAGED_SPARSE_ARRAY_ALLOCATE(T)` and `PAGED_SPARSE_ARRAY_RELEASE(T)` are called only for a page that no other call uses, so only `PAGED_SPARSE_ARRAY_GET(T)` is called concurrently for the same page.

## Threading Model

`ALLOCATE`, `RELEASE` and `GET` may be called concurrently from any number of threads.

The array guarantees:
- A pointer obtained from `ALLOCATE` or `GET` for an `index` remains valid until that `index` is released.
- `item_dispose_func` is called without holding any lock: by `RELEASE` for the element being released, and by the dispose of the array for each element that is still allocated. It may be called concurrently from multiple threads, for different elements.

Callers shall follow these rules:
- Only the user that allocated an `index` shall release it, and an `index` shall not be released more than once per allocation.
- Callers shall synchronize access to the contents of an item (e.g., one thread writing an item while another thread reads it after `GET`): the array only protects its own bookkeeping.

## Exposed API

```c
/*to be used as the type of handle that wraps T*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY(T)

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
THREAD_SAFE_PAGED_SPARSE_ARRAY(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func);
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** allocated_ptr);
void THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** item);
```

### THREAD_SAFE_PAGED_SPARSE_ARRAY(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY(T)` is a `THANDLE`(`THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T`), where `THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T` is a structure that holds the wrapped `PAGED_SPARSE_ARRAY` of slots, the custom item dispose function and one page guard for each page.

### THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(T)` is a macro to be used in a header declaration. It introduces the item dispose function type (`THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T)`), the slot type, the wrapped `PAGED_SPARSE_ARRAY` of slots (with its `THANDLE` declaration) and `THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T`.

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
THREAD_SAFE_PAGED_SPARSE_ARRAY(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` creates a new thread safe paged sparse array with the specified maximum size and page size. `PAGED_SPARSE_ARRAY_CREATE(T)` validates the arguments.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `PAGED_SPARSE_ARRAY_CREATE(T)` with `max_size`, `page_size` and `NULL` as `item_dispose_func` to create the paged sparse array of slots. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [** If `PAGED_SPARSE_ARRAY_CREATE(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` to allocate memory for the thread safe paged sparse array with one page guard for each page of the paged sparse array. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall set the use count of each page guard to 0 and the page state of each page guard to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `srw_lock_ll_init` to initialize the lock of each page guard. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_006: [** If `srw_lock_ll_init` fails: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_080: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `srw_lock_ll_deinit` for each lock that was initialized. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_081: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall free the memory of the thread safe paged sparse array. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store `item_dispose_func` in the thread safe paged sparse array. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store the paged sparse array in the thread safe paged sparse array. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall succeed and return a non-`NULL` value. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [** If there are any other errors: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_082: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall release the paged sparse array. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_083: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* paged_sparse_array);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` is the internal dispose function called by `THANDLE` when all references to the thread safe paged sparse array are released.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [** If `item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `item_dispose_func` with a pointer to the item of each slot whose state is `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED` in the pages whose page state is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `srw_lock_ll_deinit` for the lock of each page guard. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall release the paged sparse array. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)

```c
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** allocated_ptr);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` allocates the element at `index`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_042: [** If `allocated_ptr` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_044: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` for the page with index `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [** If `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` returns `false`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_046: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_048: [** If `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` with the paged sparse array and `index` to get the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_050: [** If `PAGED_SPARSE_ARRAY_GET(T)` fails: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_084: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_085: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall change the state of the slot from `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED` to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED` using `interlocked_compare_exchange`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [** If the state of the slot was not `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_086: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_087: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_053: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall store a pointer to the item of the slot in `allocated_ptr` and return `PAGED_SPARSE_ARRAY_ALLOCATE_OK`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)

```c
void THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` releases the element at `index`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_056: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` for the page with index `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_057: [** If `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` returns `true`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` with the paged sparse array and `index` to get the slot of the element. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_059: [** If `PAGED_SPARSE_ARRAY_GET(T)` fails: **]**

   - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_088: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

   - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_089: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_060: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall change the state of the slot from `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED` to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_RELEASING` using `interlocked_compare_exchange`. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_061: [** If the state of the slot was `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED`: **]**

   - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [** If `item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `item_dispose_func` with a pointer to the item of the slot. **]**

   - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall set the state of the slot to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED` using interlocked operations. **]**

   - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_064: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 2. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_065: [** Otherwise, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_066: [** If `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` returns `false`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)

```c
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** item);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` gets the element at `index`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [** If `item` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` for the page with index `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [** If `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` returns `false`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` with the paged sparse array and `index` to get the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_073: [** If `PAGED_SPARSE_ARRAY_GET(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall return the result of `PAGED_SPARSE_ARRAY_GET(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [** If the state of the slot is not `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_075: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall store a pointer to the item of the slot in `item` and return `PAGED_SPARSE_ARRAY_GET_OK`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_076: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1 before returning, also when `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` returns `false`. **]**

### Internal functions

#### THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)

```c
static bool THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` begins a use of the page with index `page_index`. It returns `true` when the page is allocated and can be used. The caller shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` when the use ends, also when `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` returns `false`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_015: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` shall increment the use count of the page guard using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_016: [** If the use count is greater than or equal to `THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` shall return `false`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [** If the page state of the page guard is not `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` shall return `false`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_018: [** Otherwise, `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` shall return `true`. **]**

#### THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index, int64_t ended_use_count);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` ends `ended_use_count` uses of the page with index `page_index`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` shall subtract `ended_use_count` from the use count of the page guard using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [** If the use count became 0 and the page state of the page guard is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)`. **]**

#### THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)

```c
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` allocates the page with index `page_index` if it is not allocated. When it succeeds, it has begun a use of the page for the caller.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall acquire the lock of the page guard in exclusive mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [** While the page state of the page guard is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall call `InterlockedHL_WaitForNotValue` to wait for the page state to change. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [** If `InterlockedHL_WaitForNotValue` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [** If the page state of the page guard is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall call `PAGED_SPARSE_ARRAY_ALLOCATE(T)` for each index of the page. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall set the state of each slot of the page to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED` using interlocked operations. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [** If `PAGED_SPARSE_ARRAY_ALLOCATE(T)` fails: **]**

   - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_078: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall call `PAGED_SPARSE_ARRAY_RELEASE(T)` for each index of the page that was allocated. **]**

   - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_079: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall increment the use count of the page guard using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall set the page state of the page guard to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED` using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [** In all cases, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall release the lock of the page guard before returning. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_040: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall succeed and return `PAGED_SPARSE_ARRAY_ALLOCATE_OK`. **]**

#### THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` frees the page with index `page_index` if it is allocated and no call uses it.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall acquire the lock of the page guard in exclusive mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [** If the page state of the page guard is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall change the use count of the page guard from 0 to `THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING` using `interlocked_compare_exchange_64`. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [** If the use count was 0, `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall set the page state of the page guard to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING` using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall release the lock of the page guard. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [** If the use count was changed to `THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall call `PAGED_SPARSE_ARRAY_RELEASE(T)` for each index of the page. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_028: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall subtract `THREAD_SAFE_PAGED_SPARSE_ARRAY_USE_COUNT_FREEING` from the use count of the page guard using interlocked operations. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_077: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall set the page state of the page guard to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` using interlocked operations. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_029: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall call `wake_by_address_all` for the page state of the page guard. **]**
