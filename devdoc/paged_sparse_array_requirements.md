# `paged_sparse_array` requirements

## Overview

`PAGED_SPARSE_ARRAY` is a module that provides a templatized paged sparse array data structure. The array has a maximum size and allocates memory in pages (groups of elements) on demand. By default, when all elements in a page are released, the page is automatically freed.

The behavior can be customized at creation time with `PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` (see [Options](#options)), which allows making the array thread-safe, keeping empty pages until the user frees them, and zeroing newly allocated pages.

`PAGED_SPARSE_ARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `PAGED_SPARSE_ARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
- `PAGED_SPARSE_ARRAY_INITIALIZE(T)`
- `PAGED_SPARSE_ARRAY_ASSIGN(T)`
- `PAGED_SPARSE_ARRAY_MOVE(T)`
- `PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T)`

## Design

The array is organized into pages. Each page contains `page_size` elements. The total number of elements that can be stored is `max_size`. Pages are allocated lazily when an element is first allocated within that page. By default, when all elements in a page are released, the page is automatically freed.

Each element has an allocation state (allocated or not allocated). The user can:
- Allocate an index for usage (fails if already allocated)
- Release an index (by default, when all indexes in a page are released, the page is freed)
- Get an index (fails if not allocated)
- Free the page that contains an index, if no element in that page is allocated

### Options

`PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` takes a `PAGED_SPARSE_ARRAY_OPTIONS` structure with the following fields:

| Option | Meaning | Value used by `PAGED_SPARSE_ARRAY_CREATE(T)` |
|---|---|---|
| `thread_safe` | `ALLOCATE`, `RELEASE`, `GET` and `FREE_PAGE_IF_EMPTY` may be called concurrently from multiple threads (see [Threading Model](#threading-model)). | `false` |
| `auto_free_empty_pages` | `RELEASE` frees a page when the last allocated element in it is released. When `false`, empty pages are kept until `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` is called or the array is disposed. This avoids freeing and re-allocating a page when its elements are released and allocated again in quick succession. | `true` |
| `zero_new_pages` | All the items of a newly allocated page are set to zero. Items are not zeroed when an element is released and allocated again in a page that was kept. | `false` |

`PAGED_SPARSE_ARRAY_CREATE(T)` keeps the behavior that existed before options were introduced: it does not create a lock and does not call any lock or interlocked APIs.

### Custom Item Dispose Function

`PAGED_SPARSE_ARRAY` supports an optional custom item dispose function that is called for each allocated element when the paged sparse array is disposed. This allows users to provide cleanup logic for element types that require it (e.g., releasing resources, freeing memory, decrementing reference counts).

The custom item dispose function is passed to `PAGED_SPARSE_ARRAY_CREATE(T)` and is called:
- By `PAGED_SPARSE_ARRAY_RELEASE(T)` when an element is released
- By `PAGED_SPARSE_ARRAY_DISPOSE(T)` for all elements that are still allocated at the time of disposal

If no custom dispose function is needed, `NULL` can be passed to `PAGED_SPARSE_ARRAY_CREATE(T)`.

### Threading Model

`PAGED_SPARSE_ARRAY` is a `THANDLE`, which means that the ownership of the array (reference counting, assignment, move) is thread-safe.

When the array is created with `PAGED_SPARSE_ARRAY_CREATE(T)` (or with `thread_safe` set to `false`), the operations on the array contents (ALLOCATE, RELEASE, GET, FREE_PAGE_IF_EMPTY) are **not thread-safe**. If multiple threads need to access the same `PAGED_SPARSE_ARRAY` instance, the caller must provide external synchronization (e.g., using a mutex or SRW lock) to protect against concurrent modifications.

When the array is created with `thread_safe` set to `true`, ALLOCATE, RELEASE, GET and FREE_PAGE_IF_EMPTY may be called concurrently from any number of threads:
- The array owns an `SRW_LOCK_LL`. ALLOCATE, RELEASE and GET hold it in shared mode, so they run concurrently with each other.
- The lock is held in exclusive mode only to allocate a page or to free a page (once per `page_size` elements in the common case).
- The allocation bitmap and the per-page allocated count are changed with interlocked operations while the lock is held in shared mode.
- A page is freed only while the lock is held in exclusive mode and only if no element in it is allocated. Therefore a pointer obtained from ALLOCATE or GET for an `index` remains valid until that `index` is released.

The rules for callers of a thread-safe array are:
- Only the user that allocated an `index` shall release it, and an `index` shall not be released more than once per allocation.
- The array only protects its own bookkeeping. Synchronizing access to the contents of an element (e.g., one thread writing an element while another thread reads it after GET) is the responsibility of the caller.
- `item_dispose_func` is called while the lock is held. It shall not call back into the same array, otherwise it can deadlock (SRW locks are not re-entrant).

### Page Structure

Each page is a single memory allocation containing:
- An array of `page_size` elements of type `T` (the `items` array)
- An allocation bitmap: a `uint8_t` array where each bit tracks whether the corresponding element is allocated (1) or not (0)

The bitmap size is `(page_size + 7) / 8` bytes (one bit per element, rounded up to the nearest byte). The bitmap is stored immediately after the items array in the same allocation, accessed via pointer arithmetic from the end of the items array.

### Memory Layout

Elements in the array are **not stored contiguously in memory**. Each page is allocated separately, so elements in different pages are in different memory locations. Even elements within the same page should not be accessed via pointer arithmetic.

**Important:** Pointer arithmetic does not work with this data structure. For example:
```c
T* elem0;
T* elem1;
PAGED_SPARSE_ARRAY_GET(T)(array, 0, &elem0);
PAGED_SPARSE_ARRAY_GET(T)(array, 1, &elem1);
// elem0 + 1 is NOT guaranteed to equal elem1
```

Always use the provided API functions (GET, ALLOCATE, etc.) to access elements by index.

### Visual Representation

The following diagram shows a `PAGED_SPARSE_ARRAY` with `page_size=4` and `max_size=12` in a partially allocated state:

```
PAGED_SPARSE_ARRAY (max_size=12, page_size=4, page_count=3)
│
├── pages[0] ──► PAGE 0
│                ├── bitmap: 0b1011  (slots 0,1,3 allocated)
│                ├── items[0]: [DATA] ◄─ index 0
│                ├── items[1]: [DATA] ◄─ index 1
│                ├── items[2]: [    ] ◄─ index 2 (unallocated)
│                └── items[3]: [DATA] ◄─ index 3
│
├── pages[1] ──► NULL (page not yet allocated, indices 4-7)
│
└── pages[2] ──► PAGE 2
                 ├── bitmap: 0b0001  (only slot 0 allocated)
                 ├── items[0]: [DATA] ◄─ index 8
                 ├── items[1]: [    ] ◄─ index 9 (unallocated)
                 ├── items[2]: [    ] ◄─ index 10 (unallocated)
                 └── items[3]: [    ] ◄─ index 11 (unallocated)
```

Key observations:
- Each page is allocated independently when first needed
- The bitmap tracks which slots within a page are in use
- `pages[1]` is NULL because no index in range 4-7 has been allocated yet
- When all items in a page are released, the page is freed and its pointer becomes NULL

## Exposed API

```c
/*to be used as the type of handle that wraps T*/
#define PAGED_SPARSE_ARRAY(T)

/*custom item dispose function type*/
#define PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(T) MU_C2(PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC_, T)
typedef void (*PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(T))(T* item);

/*to be used in a header file*/
#define PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)

/*result codes for PAGED_SPARSE_ARRAY_ALLOCATE*/
#define PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES \
    PAGED_SPARSE_ARRAY_ALLOCATE_OK, \
    PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS, \
    PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED, \
    PAGED_SPARSE_ARRAY_ALLOCATE_ERROR

MU_DEFINE_ENUM(PAGED_SPARSE_ARRAY_ALLOCATE_RESULT, PAGED_SPARSE_ARRAY_ALLOCATE_RESULT_VALUES)

/*result codes for PAGED_SPARSE_ARRAY_GET*/
#define PAGED_SPARSE_ARRAY_GET_RESULT_VALUES \
    PAGED_SPARSE_ARRAY_GET_OK, \
    PAGED_SPARSE_ARRAY_GET_INVALID_ARGS, \
    PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED

MU_DEFINE_ENUM(PAGED_SPARSE_ARRAY_GET_RESULT, PAGED_SPARSE_ARRAY_GET_RESULT_VALUES)

/*result codes for PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY*/
#define PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT_VALUES \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED, \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS, \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_ALLOCATED, \
    PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY

MU_DEFINE_ENUM(PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT, PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT_VALUES)

/*options for PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS*/
typedef struct PAGED_SPARSE_ARRAY_OPTIONS_TAG
{
    bool thread_safe;
    bool auto_free_empty_pages;
    bool zero_new_pages;
} PAGED_SPARSE_ARRAY_OPTIONS;

/*format and values to log a PAGED_SPARSE_ARRAY_OPTIONS, e.g. LogInfo("options=%" PRI_PAGED_SPARSE_ARRAY_OPTIONS "", PAGED_SPARSE_ARRAY_OPTIONS_VALUES(options))*/
#define PRI_PAGED_SPARSE_ARRAY_OPTIONS "s{ thread_safe=%" PRI_BOOL ", auto_free_empty_pages=%" PRI_BOOL ", zero_new_pages=%" PRI_BOOL " }"
#define PAGED_SPARSE_ARRAY_OPTIONS_VALUES(options) "", MU_BOOL_VALUE((options).thread_safe), MU_BOOL_VALUE((options).auto_free_empty_pages), MU_BOOL_VALUE((options).zero_new_pages)
```

The macros expand to these useful APIs:

```c
PAGED_SPARSE_ARRAY(T) PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(T) item_dispose_func);
PAGED_SPARSE_ARRAY(T) PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)(uint64_t max_size, uint64_t page_size, PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(T) item_dispose_func, const PAGED_SPARSE_ARRAY_OPTIONS* options);
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT PAGED_SPARSE_ARRAY_ALLOCATE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** allocated_ptr);
void PAGED_SPARSE_ARRAY_RELEASE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
PAGED_SPARSE_ARRAY_GET_RESULT PAGED_SPARSE_ARRAY_GET(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** item);
PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```

### PAGED_SPARSE_ARRAY(T)

```c
#define PAGED_SPARSE_ARRAY(T)
```

`PAGED_SPARSE_ARRAY(T)` is a `THANDLE`(`PAGED_SPARSE_ARRAY_STRUCT_T`), where `PAGED_SPARSE_ARRAY_STRUCT_T` is a structure that holds the paged sparse array data.

### PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)

```c
#define PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)
```

`PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)` is a macro to be used in a header declaration.

It introduces the APIs (as MOCKABLE_FUNCTIONS) that can be called for a `PAGED_SPARSE_ARRAY`.

Example usage:

```c
PAGED_SPARSE_ARRAY_TYPE_DECLARE(int32_t);
```

### PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)

```c
#define PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)
```

`PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)` is a macro to be used in a .c file to define all the needed functions for `PAGED_SPARSE_ARRAY(T)`.

Example usage:

```c
PAGED_SPARSE_ARRAY_TYPE_DEFINE(int32_t);
```

### PAGED_SPARSE_ARRAY_CREATE(T)

```c
PAGED_SPARSE_ARRAY(T) PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(T) item_dispose_func);
```

`PAGED_SPARSE_ARRAY_CREATE(T)` creates a new paged sparse array with the specified maximum size and page size.

`item_dispose_func` is an optional custom dispose function that will be called for each allocated element when the paged sparse array is disposed. It may be `NULL` if no cleanup is needed.

**SRS_PAGED_SPARSE_ARRAY_88_001: [** If `max_size` is zero, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_002: [** If `page_size` is zero, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_040: [** If `max_size + page_size - 1` would overflow, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_003: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall compute the number of pages as `(max_size + page_size - 1) / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_004: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` to allocate memory for the paged sparse array with the number of pages. **]**

**SRS_PAGED_SPARSE_ARRAY_88_005: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall set all page pointers to `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_006: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall store `max_size`, `page_size`, and `item_dispose_func` in the structure. **]**

**SRS_PAGED_SPARSE_ARRAY_88_007: [** If there are any errors, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_008: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall succeed and return a non-`NULL` value. **]**

**SRS_PAGED_SPARSE_ARRAY_88_046: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall create the paged sparse array with `thread_safe` set to `false`, `auto_free_empty_pages` set to `true` and `zero_new_pages` set to `false`. **]**

### PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)

```c
PAGED_SPARSE_ARRAY(T) PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)(uint64_t max_size, uint64_t page_size, PAGED_SPARSE_ARRAY_ITEM_DISPOSE_FUNC(T) item_dispose_func, const PAGED_SPARSE_ARRAY_OPTIONS* options);
```

`PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` creates a new paged sparse array with the specified maximum size, page size and options (see [Options](#options)).

**SRS_PAGED_SPARSE_ARRAY_88_047: [** If `options` is `NULL`, `PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_048: [** `PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` shall validate `max_size` and `page_size`, and allocate and initialize the paged sparse array, in the same way as `PAGED_SPARSE_ARRAY_CREATE(T)`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_049: [** `PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` shall store a copy of `*options` in the structure. **]**

**SRS_PAGED_SPARSE_ARRAY_88_050: [** If `options->thread_safe` is `true`, `PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` shall call `srw_lock_ll_init` to initialize the lock. **]**

**SRS_PAGED_SPARSE_ARRAY_88_051: [** If `srw_lock_ll_init` fails, `PAGED_SPARSE_ARRAY_CREATE_WITH_OPTIONS(T)` shall free the paged sparse array and return `NULL`. **]**

### PAGED_SPARSE_ARRAY_DISPOSE(T)

```c
static void PAGED_SPARSE_ARRAY_DISPOSE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array);
```

`PAGED_SPARSE_ARRAY_DISPOSE(T)` is the internal dispose function called by `THANDLE` when all references to the paged sparse array are released.

**SRS_PAGED_SPARSE_ARRAY_88_009: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_DISPOSE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_043: [** If `item_dispose_func` is not `NULL`, `PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `item_dispose_func` for each element that is still allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_010: [** `PAGED_SPARSE_ARRAY_DISPOSE(T)` shall free all pages that are non-`NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_052: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `srw_lock_ll_deinit`. **]**

### PAGED_SPARSE_ARRAY_ALLOCATE(T)

```c
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT PAGED_SPARSE_ARRAY_ALLOCATE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** allocated_ptr);
```

`PAGED_SPARSE_ARRAY_ALLOCATE(T)` allocates an element at the specified index for usage.

**SRS_PAGED_SPARSE_ARRAY_88_011: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_041: [** If `allocated_ptr` is `NULL`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_012: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_013: [** `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_014: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `malloc_flex` to allocate memory for the page structure containing `page_size` elements of type `T` plus a `uint8_t` allocation bitmap of `(page_size + 7) / 8` bytes, and initialize all bitmap bits to 0 (not allocated). **]**

**SRS_PAGED_SPARSE_ARRAY_88_015: [** If the element at `index` is already allocated, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_016: [** `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall mark the element at `index` as allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_017: [** `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall store a pointer to the element at `index` in `*allocated_ptr` and return `PAGED_SPARSE_ARRAY_ALLOCATE_OK`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_018: [** If `malloc_flex` fails, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_053: [** If `zero_new_pages` is `true`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall set all the items of a newly allocated page to zero. **]**

The following requirements apply when `thread_safe` is `true`. The common case (the page is already allocated) only holds the lock in shared mode. The lock is held in exclusive mode only when the page has to be allocated.

**SRS_PAGED_SPARSE_ARRAY_88_054: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall acquire the lock in shared mode. **]**

**SRS_PAGED_SPARSE_ARRAY_88_055: [** If `thread_safe` is `true` and the page is not allocated, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall release the shared lock and acquire the lock in exclusive mode. **]**

**SRS_PAGED_SPARSE_ARRAY_88_056: [** After acquiring the lock in exclusive mode, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall check again whether the page is allocated, since another thread may have allocated it in the meantime. **]**

**SRS_PAGED_SPARSE_ARRAY_88_057: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall mark the element at `index` as allocated and increment the allocated count of the page using interlocked operations. **]**

**SRS_PAGED_SPARSE_ARRAY_88_058: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall release the lock (shared or exclusive, whichever is held) before returning. **]**

### PAGED_SPARSE_ARRAY_RELEASE(T)

```c
void PAGED_SPARSE_ARRAY_RELEASE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```


`PAGED_SPARSE_ARRAY_RELEASE(T)` releases the element at the specified index.

> **Note:** If a custom `item_dispose_func` was provided at creation, it will be called for the element being released before marking it as unallocated. This allows automatic cleanup of elements during individual releases as well as during final disposal of the array. When `thread_safe` is `true`, `item_dispose_func` is called while the lock is held in shared mode, so it shall not call back into the same array.

**SRS_PAGED_SPARSE_ARRAY_88_019: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_020: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_021: [** `PAGED_SPARSE_ARRAY_RELEASE(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_022: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_023: [** If the element at `index` is not allocated, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_044: [** If `item_dispose_func` is not `NULL`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `item_dispose_func` for the element being released. **]**

**SRS_PAGED_SPARSE_ARRAY_88_024: [** `PAGED_SPARSE_ARRAY_RELEASE(T)` shall mark the element at `index` as not allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_025: [** If `auto_free_empty_pages` is `true` and all elements in the page are now not allocated, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall free the page and set the page pointer to `NULL`. **]**

The following requirements apply when `thread_safe` is `true`.

**SRS_PAGED_SPARSE_ARRAY_88_059: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall acquire the lock in shared mode. **]**

**SRS_PAGED_SPARSE_ARRAY_88_060: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall mark the element at `index` as not allocated using interlocked operations and, only if the element was still marked as allocated, decrement the allocated count of the page using interlocked operations. **]**

**SRS_PAGED_SPARSE_ARRAY_88_061: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall release the shared lock. **]**

**SRS_PAGED_SPARSE_ARRAY_88_062: [** If `thread_safe` is `true`, `auto_free_empty_pages` is `true` and the allocated count of the page became 0, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall acquire the lock in exclusive mode, free the page only if it is still allocated and its allocated count is still 0, and release the exclusive lock. **]**

### PAGED_SPARSE_ARRAY_GET(T)

```c
PAGED_SPARSE_ARRAY_GET_RESULT PAGED_SPARSE_ARRAY_GET(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** item);
```

`PAGED_SPARSE_ARRAY_GET(T)` gets the element at the specified index.

**SRS_PAGED_SPARSE_ARRAY_88_034: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_045: [** If `item` is `NULL`, `PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_035: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_036: [** `PAGED_SPARSE_ARRAY_GET(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_037: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_GET(T)` shall return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_038: [** If the element at `index` is not allocated, `PAGED_SPARSE_ARRAY_GET(T)` shall return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_039: [** `PAGED_SPARSE_ARRAY_GET(T)` shall store in `item` a pointer to the element at `index` and return `PAGED_SPARSE_ARRAY_GET_OK`. **]**

The following requirements apply when `thread_safe` is `true`. The pointer returned in `item` remains valid until the element at `index` is released.

**SRS_PAGED_SPARSE_ARRAY_88_063: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_GET(T)` shall acquire the lock in shared mode. **]**

**SRS_PAGED_SPARSE_ARRAY_88_064: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_GET(T)` shall check whether the element at `index` is allocated using interlocked operations. **]**

**SRS_PAGED_SPARSE_ARRAY_88_065: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_GET(T)` shall release the shared lock before returning. **]**

### PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)

```c
PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_RESULT PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```

`PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` frees the page that contains the element at `index`, if no element in that page is allocated. `index` can be the index of any element in the page.

This is intended for arrays created with `auto_free_empty_pages` set to `false`, where the user decides when an empty page is freed (for example, when a consumer is done with all the indexes in a page). It can also be used with `auto_free_empty_pages` set to `true`, although in that case empty pages are already freed by `PAGED_SPARSE_ARRAY_RELEASE(T)`.

**SRS_PAGED_SPARSE_ARRAY_88_066: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall fail and return `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_067: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall fail and return `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_INVALID_ARGS`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_068: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall acquire the lock in exclusive mode. **]**

**SRS_PAGED_SPARSE_ARRAY_88_069: [** `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_070: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall return `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_ALLOCATED`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_071: [** If the allocated count of the page is not 0, `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall return `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_NOT_EMPTY`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_072: [** Otherwise, `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall free the page, set the page pointer to `NULL` and return `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY_FREED`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_073: [** If `thread_safe` is `true`, `PAGED_SPARSE_ARRAY_FREE_PAGE_IF_EMPTY(T)` shall release the exclusive lock before returning. **]**
