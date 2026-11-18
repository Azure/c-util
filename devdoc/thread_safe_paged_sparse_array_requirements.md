# `thread_safe_paged_sparse_array` requirements

## Overview

`THREAD_SAFE_PAGED_SPARSE_ARRAY` is a module that wraps a [`PAGED_SPARSE_ARRAY`](paged_sparse_array_requirements.md) so that `ALLOCATE`, `RELEASE`, `GET` and `END_GET` can be called concurrently from multiple threads. Each page has a lock: `ALLOCATE` and `GET` of an element of an allocated page hold it in shared mode, so many calls use the same page at the same time, and it is held in exclusive mode only to allocate a page and to start freeing a page. `RELEASE` releases the element without acquiring it, because the element it releases keeps the page allocated. `ALLOCATE` of an element of a page that is not allocated allocates the page, and `RELEASE` of the last element of a page frees the page, so only that `RELEASE` acquires the lock (in exclusive mode). A successful `GET` keeps holding the lock of the page until `END_GET`, so the page is not freed while the caller uses the item.

The indexes, the pages, `max_size`, `page_size` and the result codes are those of the wrapped `PAGED_SPARSE_ARRAY` (see its [Design](paged_sparse_array_requirements.md#design) section). The wrapped `PAGED_SPARSE_ARRAY` is not changed.

`THREAD_SAFE_PAGED_SPARSE_ARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `THREAD_SAFE_PAGED_SPARSE_ARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_MOVE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T)`

## Design

Terms:
- **slot**: an element of the wrapped `PAGED_SPARSE_ARRAY`. A slot holds an item of type `T` and the state of the item: `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED` or `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED`. The state is changed only with interlocked operations.
- **page guard**: there is one page guard for each page. It has an element count, a page state and a lock.
- **element count**: the number of allocated elements of the page. It is changed with interlocked operations, because several calls change it at the same time: `ALLOCATE` while holding the lock and `RELEASE` without holding it.
- **page state**: `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED` when the page is allocated in the wrapped `PAGED_SPARSE_ARRAY` and its slots can be used, `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING` while the page is being freed and `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` otherwise.
- **lock**: an `SRW_LOCK_LL`. It is held in shared mode by `ALLOCATE` and `GET` of an element of the page and in exclusive mode by the calls that allocate the page or start freeing it.
- **indexes of a page**: the indexes from `page_index * page_size` up to `(page_index + 1) * page_size - 1`. The last page has only the indexes lower than `max_size`.

How the operations work:
- `ALLOCATE` and `GET` of an element of an allocated page hold the lock of the page in shared mode while they use the page, and call `PAGED_SPARSE_ARRAY_GET(T)` (which only reads the wrapped `PAGED_SPARSE_ARRAY`) to get the slot. `ALLOCATE` changes the state of the slot and the element count with interlocked operations, `GET` only reads the state of the slot. `ALLOCATE` releases the lock before returning, a successful `GET` keeps holding it until `END_GET`.
- `RELEASE` releases the element without acquiring the lock of the page. The released element is counted in the element count until `RELEASE` decrements it, so until then the page is not freed or allocated again. `RELEASE` calls `PAGED_SPARSE_ARRAY_GET(T)` to get the slot, disposes the item, sets the state of the slot to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED` and decrements the element count with interlocked operations, which is the last time it uses the page without holding the lock. If the element count became 0, `RELEASE` then frees the page (see below), which acquires the lock in exclusive mode.
- A page is allocated by `ALLOCATE` while holding the lock of the page in exclusive mode: `PAGED_SPARSE_ARRAY_ALLOCATE(T)` is called for each index of the page, all the slots are set to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED` and all the bytes of their items are set to 0. The element is allocated before the lock is released, so a new page is never empty.
- A page is freed by the `RELEASE` that makes its element count 0. It acquires the lock of the page in exclusive mode (it waits for the calls that hold the lock, including the `GET`s that did not call `END_GET` yet) and, if the page is still allocated and its element count is still 0, sets the page state to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING`. After the lock is released, `PAGED_SPARSE_ARRAY_RELEASE(T)` is called for each index of the page (the last one frees the page), the page state is set to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` and the calls waiting for the page state to change are woken. `GET` of an element of a page that is being freed returns `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`, and `ALLOCATE` of an element of a page that is being freed waits (while holding the lock in exclusive mode) for the page state to change and then allocates the page again.
- `PAGED_SPARSE_ARRAY_ALLOCATE(T)` and `PAGED_SPARSE_ARRAY_RELEASE(T)` are called only for a page that no other call uses, so only `PAGED_SPARSE_ARRAY_GET(T)` is called concurrently for the same page. Calls for different pages run concurrently. The wrapped `PAGED_SPARSE_ARRAY` allows both, because it keeps its state per page (see its [Threading Model](paged_sparse_array_requirements.md#threading-model)).

## Threading Model

`ALLOCATE`, `RELEASE`, `GET` and `END_GET` may be called concurrently from any number of threads.

The array guarantees:
- A pointer obtained from `ALLOCATE` for an `index` remains valid until that `index` is released.
- A pointer obtained from `GET` remains valid until `END_GET` is called for that `index`: the page of the element is not freed before. The owner of the `index` can still release it in the meantime (then the item is disposed, and the element can be allocated again).
- The items of a new page have all their bytes set to 0 before any call can get them. An element that is allocated again keeps its item as it was when it was released.
- `item_dispose_func` is called without holding any lock: by `RELEASE` for the element being released, and by the dispose of the array for each element that is still allocated. It may be called concurrently from multiple threads, for different elements.

Callers shall follow these rules:
- Only the owner of an `index` shall release it, and an `index` shall not be released more than once per allocation. The owner of an `index` is the caller that allocated it, or the caller that the owner handed it over to (e.g., a producer allocates an `index` and a consumer releases it).
- Each `GET` that returns `PAGED_SPARSE_ARRAY_GET_OK` shall be followed by exactly one `END_GET` for the same `index`, from the same thread (on Linux the lock is a `pthread_rwlock_t`, which shall be released by the thread that acquired it). `END_GET` shall not be called otherwise.
- Between `GET` and `END_GET` the thread shall not call `ALLOCATE`, `RELEASE` or `GET` of any `THREAD_SAFE_PAGED_SPARSE_ARRAY` (the same array or another one). Between `GET` and `END_GET` the thread shall also not wait for anything (a lock, an event, a queue, another thread) that depends on a call to `ALLOCATE` or `RELEASE` of the same array, because `RELEASE` of the last element of the page and `ALLOCATE` of an element of a page that is being freed wait for `END_GET`. The time between `GET` and `END_GET` should be short: the `RELEASE` of the last element of the page waits for `END_GET` before it frees the page, and while it waits, the calls that acquire the lock of the page can wait too.
- `GET` can get an element right after `ALLOCATE`, before its owner wrote the item. A caller that gets an element it does not own shall check that the owner wrote it, for example with a state in the item that is 0 until the owner sets it with an interlocked operation after writing the rest of the item. An element that is allocated again keeps its item, so the owner shall set that state back to 0 before releasing the `index` (for example, in `item_dispose_func`). Other concurrent access to the contents of an item shall be synchronized by the callers: the array only protects its own bookkeeping.

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
void THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
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

`THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` creates a new thread safe paged sparse array with the specified maximum size and page size. `PAGED_SPARSE_ARRAY_CREATE(T)` validates the arguments. `item_dispose_func` is optional: when it is not `NULL`, it is called for an element when the element is released and, when the array is disposed, for each element that was not released.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `PAGED_SPARSE_ARRAY_CREATE(T)` with `max_size`, `page_size` and `NULL` as `item_dispose_func` to create the paged sparse array of slots. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [** If `PAGED_SPARSE_ARRAY_CREATE(T)` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` to allocate `result` with one page guard in `result->page_guards` for each page of the paged sparse array. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall set `element_count` to 0 and `page_state` to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` in each page guard of `result->page_guards`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `srw_lock_ll_init` to initialize `lock` in each page guard of `result->page_guards`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store `item_dispose_func` in `result->item_dispose_func`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store the paged sparse array in `result->paged_sparse_array`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall succeed and return a non-`NULL` value. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [** If there are any other errors, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* paged_sparse_array);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` is the internal dispose function called by `THANDLE` when all references to the thread safe paged sparse array are released.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [** If `paged_sparse_array->item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `paged_sparse_array->item_dispose_func` with a pointer to the item of each slot whose state is `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED` in the pages whose `page_state` is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `srw_lock_ll_deinit` for `lock` in each page guard of `paged_sparse_array->page_guards`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall release `paged_sparse_array->paged_sparse_array`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)

```c
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** allocated_ptr);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` allocates the element at `index`. When the page of the element is allocated, the element is allocated while holding the lock of the page in shared mode. Otherwise, the page and then the element are allocated while holding the lock of the page in exclusive mode.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_042: [** If `allocated_ptr` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_093: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall acquire `page_guard->lock` in shared mode, where `page_guard` is the page guard of the page with index `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_094: [** If `page_guard->page_state` is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_095: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` and return its result. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_096: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall release `page_guard->lock` in shared mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [** If `page_guard->page_state` is not `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_097: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall release `page_guard->lock` in shared mode. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` and return its result. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)

```c
void THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` releases the element at `index`. The element shall be allocated (see [Threading Model](#threading-model)), so its page is allocated. The item is disposed before the slot is set to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED`, so the slot cannot be allocated again while its item is disposed. `RELEASE` releases the element without acquiring the lock of the page: the element is counted in the element count of the page until it is decremented, so the page is not freed or allocated again before. If the element count becomes 0, `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` acquires the lock in exclusive mode to free the page.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` with `paged_sparse_array->paged_sparse_array` and `index` to get the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [** If `paged_sparse_array->item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `paged_sparse_array->item_dispose_func` with a pointer to the item of the slot. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall set the state of the slot to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_101: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall decrement `page_guard->element_count`, where `page_guard` is the page guard of the page with index `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_103: [** If the `element_count` became 0, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)

```c
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** item);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` gets the element at `index`. When it returns `PAGED_SPARSE_ARRAY_GET_OK`, it keeps holding the lock of the page in shared mode, so the page is not freed while the caller uses the item: the caller shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)` for the same `index` when it does not use the item anymore (see [Threading Model](#threading-model)).

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [** If `item` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_104: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall acquire `page_guard->lock` in shared mode, where `page_guard` is the page guard of the page with index `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [** If `page_guard->page_state` is not `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_072: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` with `paged_sparse_array->paged_sparse_array` and `index` to get the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_074: [** If the state of the slot is not `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_075: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall store a pointer to the item of the slot in `item` and return `PAGED_SPARSE_ARRAY_GET_OK`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_105: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall release `page_guard->lock` in shared mode before returning, unless it returns `PAGED_SPARSE_ARRAY_GET_OK`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)

```c
void THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)` ends the use of an element that `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` got: it releases the lock of the page that `GET` holds. After it returns, the pointer obtained from `GET` shall not be used.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_106: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_107: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_108: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_GET(T)` shall release `page_guard->lock` in shared mode, where `page_guard` is the page guard of the page with index `index / page_size`. **]**

### Internal functions

In the requirements of the internal functions, `page_guard` is `array->page_guards[page_index]`.

#### THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)

```c
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index, uint64_t index, T** allocated_ptr);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` allocates the element at `index` of the allocated page with index `page_index`. It is called while holding the lock of the page, in shared mode or in exclusive mode. In shared mode, other calls can try to allocate the same element at the same time: the state of the slot is changed with `interlocked_compare_exchange`, so only one of them allocates the element and the others return `PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` shall call `PAGED_SPARSE_ARRAY_GET(T)` with `array->paged_sparse_array` and `index` to get the slot of the element. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` shall change the state of the slot from `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED` to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_ALLOCATED` using `interlocked_compare_exchange`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [** If the state of the slot was not `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_098: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` shall increment `page_guard->element_count`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_053: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` shall store a pointer to the item of the slot in `allocated_ptr` and return `PAGED_SPARSE_ARRAY_ALLOCATE_OK`. **]**

#### THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)

```c
static PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index, uint64_t index, T** allocated_ptr);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` allocates the page with index `page_index` if it is not allocated, and then allocates the element at `index`, while holding the lock of the page in exclusive mode. Another call can allocate the page between the release of the lock in shared mode by `ALLOCATE` and the acquire in exclusive mode. `PAGED_SPARSE_ARRAY_ALLOCATE(T)` for the first index of the page allocates the page of the wrapped `PAGED_SPARSE_ARRAY`, so it cannot fail for the other indexes of the page.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall acquire `page_guard->lock` in exclusive mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [** If `page_guard->page_state` is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall call `InterlockedHL_WaitForNotValue` to wait for `page_guard->page_state` to change. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [** If `InterlockedHL_WaitForNotValue` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [** If `page_guard->page_state` is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall call `PAGED_SPARSE_ARRAY_ALLOCATE(T)` on `array->paged_sparse_array` for each index of the page. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall set the state of each slot of the page to `THREAD_SAFE_PAGED_SPARSE_ARRAY_SLOT_STATE_NOT_ALLOCATED`. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_092: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall set all the bytes of the item of each slot of the page to 0. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [** If `PAGED_SPARSE_ARRAY_ALLOCATE(T)` fails for the first index of the page, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall set `page_guard->page_state` to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_099: [** If `page_guard->page_state` is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_ELEMENT(T)` and return its result. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [** In all cases, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE_PAGE(T)` shall release `page_guard->lock` before returning. **]**

#### THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` frees the page with index `page_index` if it is allocated and has no allocated element. Another call can allocate an element of the page, or free the page, between the decrement of the element count by `RELEASE` and the acquire of the lock in exclusive mode. While the lock is held in exclusive mode, the element count can still decrease (`RELEASE` decrements it without acquiring the lock), but each `RELEASE` that makes it 0 calls `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` after that. While the page is allocated, all its indexes stay allocated in the wrapped `PAGED_SPARSE_ARRAY` (`RELEASE` only changes the state of the slot), so `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` releases each of them and the last release frees the page of the wrapped `PAGED_SPARSE_ARRAY`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall acquire `page_guard->lock` in exclusive mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [** If `page_guard->page_state` is `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_ALLOCATED` and `page_guard->element_count` is 0: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall set `page_guard->page_state` to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall release `page_guard->lock`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [** If `page_guard->page_state` was set to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_FREEING`: **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall call `PAGED_SPARSE_ARRAY_RELEASE(T)` on `array->paged_sparse_array` for each index of the page. **]**

 - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_077: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall call `InterlockedHL_SetAndWakeAll` to set `page_guard->page_state` to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_STATE_NOT_ALLOCATED` and wake the calls waiting for it to change. **]**
