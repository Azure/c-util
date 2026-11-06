# `thread_safe_paged_sparse_array` requirements

## Overview

`THREAD_SAFE_PAGED_SPARSE_ARRAY` is a module that provides a templatized paged sparse array whose `ALLOCATE`, `RELEASE`, `GET` and `FREE_EMPTY_PAGES` can be called concurrently from multiple threads.

It is the thread-safe counterpart of [`PAGED_SPARSE_ARRAY`](paged_sparse_array_requirements.md): the array has a maximum size and allocates memory in pages (groups of elements) on demand. It uses the result codes of `PAGED_SPARSE_ARRAY` (`PAGED_SPARSE_ARRAY_ALLOCATE_RESULT` and `PAGED_SPARSE_ARRAY_GET_RESULT`) and adds:
- a page slot for each page, with a use count and a lock (see [Page Slots](#page-slots) and [Threading Model](#threading-model)). `ALLOCATE`, `RELEASE` and `GET` do not acquire any lock when the page is allocated, the lock of a page is acquired only to allocate or to free that page,
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)`, which frees the empty pages that contain the indexes of a range,
- 2 options selected at compile time (see [Options](#options)).

`THREAD_SAFE_PAGED_SPARSE_ARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `THREAD_SAFE_PAGED_SPARSE_ARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_ASSIGN(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_MOVE(T)`
- `THREAD_SAFE_PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T)`

## Design

The array is organized into pages. Each page contains `page_size` elements. The total number of elements that can be stored is `max_size`. Pages are allocated lazily when an element is first allocated within that page.

Each element has an allocation state (allocated or not allocated). The user can:
- Allocate an index for usage (fails if already allocated)
- Release an index (by default, when all indexes in a page are released, the page is freed)
- Get an index (fails if not allocated)
- Free the pages that contain the indexes of a range, if no element in those pages is allocated

### Page Structure

Each page is a single memory allocation containing:
- An array of `page_size` elements of type `T` (the items)
- An allocation bitmap: a `uint8_t` array where each bit tracks whether the corresponding element is allocated (1) or not (0)

The bitmap size is `(page_size + 7) / 8` bytes. The bitmap is stored immediately after the items in the same allocation.

Elements are **not stored contiguously in memory**, pointer arithmetic between elements shall not be used. Always use the provided API functions to access elements by index.

### Page Slots

The array has one page slot for each page. The page slots are allocated with the array and exist as long as the array exists, also when their page is not allocated. A page slot contains:
- `page`: a pointer to the page (see [Page Structure](#page-structure)), `NULL` when the page is not allocated
- `use_count`: the count of allocated elements in the page plus the count of calls that use the page at that moment. While the page is being freed, `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING` (2<sup>62</sup>) is added to it.
- `lock`: an `SRW_LOCK_LL` that is acquired in exclusive mode only to allocate or to free the page

`ALLOCATE`, `RELEASE` and `GET` begin to use a page before they access it (`THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)`) and end the use when they no longer access it (`THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)`). When `ALLOCATE` succeeds, the use of the call is not ended, it becomes the use of the allocated element and it is ended by `RELEASE`. A page is freed (`THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)`) only if its use count is 0.

### Visual Representation

The following diagram shows a `THREAD_SAFE_PAGED_SPARSE_ARRAY` with `page_size=4` and `max_size=12` while a thread is in a call to `GET` for index 9:

```
THREAD_SAFE_PAGED_SPARSE_ARRAY (max_size=12, page_size=4, page_count=3)
│
├── page_slots[0]: use_count=3 (3 allocated elements), lock
│   └── page ──► PAGE 0
│                ├── items[0]: [DATA] ◄─ index 0
│                ├── items[1]: [DATA] ◄─ index 1
│                ├── items[2]: [    ] ◄─ index 2 (unallocated)
│                ├── items[3]: [DATA] ◄─ index 3
│                └── bitmap: 0b1011  (slots 0,1,3 allocated)
│
├── page_slots[1]: use_count=0, lock
│   └── page ──► NULL (page not yet allocated, indices 4-7)
│
└── page_slots[2]: use_count=2 (1 allocated element + the call to GET), lock
    └── page ──► PAGE 2
                 ├── items[0]: [DATA] ◄─ index 8
                 ├── items[1]: [    ] ◄─ index 9 (unallocated)
                 ├── items[2]: [    ] ◄─ index 10 (unallocated)
                 ├── items[3]: [    ] ◄─ index 11 (unallocated)
                 └── bitmap: 0b0001  (only slot 0 allocated)
```

### Options

2 options are selected at compile time, each one by a name:

| Option | Value | Meaning |
|---|---|---|
| `empty_pages` | `AUTO_FREE_EMPTY_PAGES` (default) | A page is freed when its use count becomes 0: no element in it is allocated and no call uses it. |
| `empty_pages` | `KEEP_EMPTY_PAGES` | Empty pages are kept until `FREE_EMPTY_PAGES` is called or the array is disposed. This avoids freeing and re-allocating a page when its elements are released and allocated again in quick succession (e.g. when the array is used as a ring buffer). |
| `new_pages` | `DO_NOT_ZERO_NEW_PAGES` (default) | The items of a newly allocated page are left uninitialized. |
| `new_pages` | `ZERO_NEW_PAGES` | All the items of a newly allocated page are set to zero. Items are not zeroed when an element is released and allocated again in a page that was kept. |

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)` uses the default values. Other values are selected with `THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(T, empty_pages, new_pages)`, e.g. `THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(T, KEEP_EMPTY_PAGES, ZERO_NEW_PAGES)`. At the low level (`thread_safe_paged_sparse_array_ll.h`), each `THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_*_DEFINE(C, T)` macro has a `_WITH_OPTIONS(C, T, empty_pages, new_pages)` variant. `new_pages` changes only the allocation of a page and `empty_pages` changes only the end of the use of a page, the other functions only log the options. All the functions of a type shall be defined with the same options: `THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_DEFINE_WITH_OPTIONS(C, T, empty_pages, new_pages)` (and `THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_TYPE_DEFINE(C, T)`) defines all of them at once.

The value of each option is pasted to a prefix that is specific to that option (`THREAD_SAFE_PAGED_SPARSE_ARRAY_EMPTY_PAGES_OPTION_` or `THREAD_SAFE_PAGED_SPARSE_ARRAY_NEW_PAGES_OPTION_`), and only the names above are defined (as `1` or `0`). Therefore a misspelled name, a number, or the 2 options given in the wrong order expand to an undeclared identifier and do not compile.

Since the options are compile-time constants, only the code for the selected options is compiled.

### Custom Item Dispose Function

The custom item dispose function is passed to `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` and is called:
- By `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` when an element is released
- By `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` for all elements that are still allocated at the time of disposal

If no custom dispose function is needed, `NULL` can be passed to `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)`.

### Threading Model

`ALLOCATE`, `RELEASE`, `GET` and `FREE_EMPTY_PAGES` may be called concurrently from any number of threads:
- `ALLOCATE`, `RELEASE` and `GET` do not acquire any lock when the page is allocated. They change the use count of the page and the allocation bitmap (see [Page Slots](#page-slots)) with interlocked operations.
- The lock of a page is acquired in exclusive mode only to allocate that page (by `ALLOCATE`, when the page is not allocated) and to free that page (when its use count became 0 and `empty_pages` is `AUTO_FREE_EMPTY_PAGES`, or by `FREE_EMPTY_PAGES`). Calls that use other pages, and calls that do not allocate or free the page, do not wait for it.
- A page is freed only after its use count was changed from 0 to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING` with an interlocked compare-exchange: no element in the page is allocated and no call uses it. A call that begins to use the page while it is being freed considers the page not allocated. The memory of the page is freed after the lock of the page is released.
- Therefore a pointer obtained from `ALLOCATE` or `GET` for an `index` remains valid until that `index` is released.
- Since the use count also counts the calls that use a page, `FREE_EMPTY_PAGES` does not free an empty page that another call uses at the same time. When `empty_pages` is `AUTO_FREE_EMPTY_PAGES`, the call that ends the last use of an empty page frees it.

The rules for callers are:
- Only the user that allocated an `index` shall release it, and an `index` shall not be released more than once per allocation.
- The array only protects its own bookkeeping. Synchronizing access to the contents of an element (e.g., one thread writing an element while another thread reads it after `GET`) is the responsibility of the caller.
- `item_dispose_func` is called without holding any lock.

## Exposed API

```c
/*to be used as the type of handle that wraps T*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY(T)

/*custom item dispose function type*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) MU_C2(THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC_, T)
typedef void (*THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T))(T* item);

/*to be used in a header file*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)
/*empty_pages is AUTO_FREE_EMPTY_PAGES or KEEP_EMPTY_PAGES, new_pages is DO_NOT_ZERO_NEW_PAGES or ZERO_NEW_PAGES*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(T, empty_pages, new_pages)

/*result codes for THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES*/
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT_VALUES \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK, \
    THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS

MU_DEFINE_ENUM(THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT, THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT_VALUES)
```

The macros expand to these useful APIs:

```c
THREAD_SAFE_PAGED_SPARSE_ARRAY(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func);
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** allocated_ptr);
void THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** item);
THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t start_index, uint64_t end_index);
```

### THREAD_SAFE_PAGED_SPARSE_ARRAY(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY(T)` is a `THANDLE`(`THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T`), where `THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T` is a structure that holds the paged sparse array data and the page slots.

### THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)` is a macro to be used in a header declaration.

It introduces the APIs (as MOCKABLE_FUNCTIONS) that can be called for a `THREAD_SAFE_PAGED_SPARSE_ARRAY`.

Example usage:

```c
THREAD_SAFE_PAGED_SPARSE_ARRAY_DEFINE_STRUCT_TYPE(int32_t)
THANDLE_TYPE_DECLARE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(int32_t));
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DECLARE(int32_t);
```

### THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)

```c
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)
#define THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(T, empty_pages, new_pages)
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)` is a macro to be used in a .c file to define all the needed functions for `THREAD_SAFE_PAGED_SPARSE_ARRAY(T)`, with `AUTO_FREE_EMPTY_PAGES` and `DO_NOT_ZERO_NEW_PAGES`.

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(T, empty_pages, new_pages)` does the same with the given options (see [Options](#options)).

Example usage:

```c
THANDLE_TYPE_DEFINE(THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPEDEF_NAME(int32_t));
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE(int32_t);

/*or, to keep empty pages until FREE_EMPTY_PAGES is called and to zero new pages*/
THREAD_SAFE_PAGED_SPARSE_ARRAY_TYPE_DEFINE_WITH_OPTIONS(int32_t, KEEP_EMPTY_PAGES, ZERO_NEW_PAGES);
```

### THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)

```c
THREAD_SAFE_PAGED_SPARSE_ARRAY(T) THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)(uint64_t max_size, uint64_t page_size, THREAD_SAFE_PAGED_SPARSE_ARRAY_LL_ITEM_DISPOSE_FUNC(T) item_dispose_func);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` creates a new thread safe paged sparse array with the specified maximum size and page size.

`item_dispose_func` is an optional custom dispose function (see [Custom Item Dispose Function](#custom-item-dispose-function)). It may be `NULL` if no cleanup is needed.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_001: [** If `max_size` is zero, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_002: [** If `page_size` is zero, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_003: [** If `max_size + page_size - 1` would overflow, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_004: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall compute the number of pages as `(max_size + page_size - 1) / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_005: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` to allocate memory for the paged sparse array with one page slot for each page. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_006: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall store `max_size`, `page_size`, and `item_dispose_func` in the structure. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_007: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall set the page pointer of each page slot to `NULL` and its use count to 0. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_008: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `srw_lock_ll_init` to initialize the lock of each page slot. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_009: [** If `srw_lock_ll_init` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall call `srw_lock_ll_deinit` for the locks that were initialized, free the paged sparse array and return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_010: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall succeed and return a non-`NULL` value. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_011: [** If there are any other errors, `THREAD_SAFE_PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` is the internal dispose function called by `THANDLE` when all references to the paged sparse array are released. Since no other thread uses the array at that time, it does not acquire any lock.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_012: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_013: [** If `item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `item_dispose_func` for each element that is still allocated. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_014: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall free all pages that are non-`NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_015: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_DISPOSE(T)` shall call `srw_lock_ll_deinit` for the lock of each page slot. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)

```c
static T* THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` is an internal function that `ALLOCATE`, `RELEASE` and `GET` call before they access the page with index `page_index`. It returns the page, or `NULL` if the page is not allocated or is being freed. In both cases the use is ended by calling `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_016: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` shall increment the use count of the page slot using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_017: [** If the use count is greater than or equal to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` shall return `NULL`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_018: [** Otherwise, `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)` shall return the page pointer of the page slot, read using interlocked operations. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index, int64_t ended_use_count);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` is an internal function that ends `ended_use_count` uses of the page with index `page_index`: the use of a call, plus the use of an element when `RELEASE` releases it.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_019: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` shall subtract `ended_use_count` from the use count of the page slot using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_020: [** If `empty_pages` is `AUTO_FREE_EMPTY_PAGES`, the use count became 0 and the page is allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)

```c
static void THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY_STRUCT_T* array, uint64_t page_index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` is an internal function that frees the page with index `page_index` if it is allocated, no element in it is allocated and no call uses it.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_021: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall acquire the lock of the page slot in exclusive mode. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_022: [** If the page is not allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall release the lock of the page slot and return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_023: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall change the use count of the page slot from 0 to `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING` using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_024: [** If the use count was not 0, `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall release the lock of the page slot and return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_025: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall set the page pointer of the page slot to `NULL` using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_026: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall subtract `THREAD_SAFE_PAGED_SPARSE_ARRAY_PAGE_FREEING` from the use count of the page slot using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_027: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall release the lock of the page slot. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_028: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)` shall free the page. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)

```c
PAGED_SPARSE_ARRAY_ALLOCATE_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** allocated_ptr);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` allocates the element at `index`. When the page is allocated, no lock is acquired. The lock of the page slot is acquired only when the page has to be allocated. When `ALLOCATE` succeeds, its use of the page is not ended: it becomes the use of the allocated element, which `RELEASE` ends.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_029: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_030: [** If `allocated_ptr` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_031: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `PAGED_SPARSE_ARRAY_ALLOCATE_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_032: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall compute the page index as `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_033: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_034: [** If the page is not allocated: **]**

  - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_035: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

  - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_036: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall acquire the lock of the page slot in exclusive mode. **]**

  - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_037: [** If the page is still not allocated: **]**

    - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_038: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `malloc_flex` to allocate memory for `page_size` elements followed by the allocation bitmap. **]**

    - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_039: [** If `malloc_flex` fails, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall release the lock of the page slot and return `PAGED_SPARSE_ARRAY_ALLOCATE_ERROR`. **]**

    - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_040: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall set all the bits of the allocation bitmap to 0. **]**

    - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_041: [** If `new_pages` is `ZERO_NEW_PAGES`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall set all the items of the page to zero. **]**

    - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_042: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall store the page pointer in the page slot using interlocked operations. **]**

  - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_043: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall increment the use count of the page slot using interlocked operations. **]**

  - **SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_044: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall release the lock of the page slot. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_045: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall mark the element at `index` as allocated using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_046: [** If the element at `index` was already marked as allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1 and return `PAGED_SPARSE_ARRAY_ALLOCATE_ALREADY_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_047: [** Otherwise, `THREAD_SAFE_PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall store a pointer to the element at `index` in `allocated_ptr` and return `PAGED_SPARSE_ARRAY_ALLOCATE_OK`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)

```c
void THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` releases the element at `index`. It does not acquire any lock, except when it ends the last use of an empty page and `empty_pages` is `AUTO_FREE_EMPTY_PAGES`: then the lock of the page slot is acquired to free the page.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_048: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_049: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_050: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall compute the page index as `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_051: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_052: [** If the page is not allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1 and return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_053: [** If the element at `index` is not allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1 and return. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_054: [** If `item_dispose_func` is not `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `item_dispose_func` for the element being released before marking the element as not allocated. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_055: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall mark the element at `index` as not allocated using interlocked operations. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_056: [** If the element was still marked as allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 2. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_057: [** Otherwise, `THREAD_SAFE_PAGED_SPARSE_ARRAY_RELEASE(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)

```c
PAGED_SPARSE_ARRAY_GET_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t index, T** item);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` retrieves a pointer to the element at `index`. The pointer returned in `item` remains valid until the element at `index` is released.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_058: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_059: [** If `item` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_060: [** If `index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `PAGED_SPARSE_ARRAY_GET_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_061: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall compute the page index as `index / page_size`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_062: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_BEGIN_PAGE_USE(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_063: [** If the page is not allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1 and return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_064: [** If the element at `index` is not allocated, `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1 and return `PAGED_SPARSE_ARRAY_GET_NOT_ALLOCATED`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_065: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall store in `item` a pointer to the element at `index`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_066: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_GET(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_END_PAGE_USE(T)` with `ended_use_count` 1 and return `PAGED_SPARSE_ARRAY_GET_OK`. **]**

### THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)

```c
THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_RESULT THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)(THREAD_SAFE_PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint64_t start_index, uint64_t end_index);
```

`THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)` frees each page that contains at least one index from `start_index` to `end_index` (both included), if no element in that page is allocated and no call uses that page. Pages that are not allocated or not empty are skipped. A single page is freed by passing any index of the page as both `start_index` and `end_index`.

It is used with `KEEP_EMPTY_PAGES` to decide when empty pages are freed (e.g. when the oldest elements of a ring buffer were released), and it can also be used with `AUTO_FREE_EMPTY_PAGES`.

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_067: [** If `paged_sparse_array` is `NULL`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)` shall fail and return `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_068: [** If `start_index` is greater than `end_index`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)` shall fail and return `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_069: [** If `end_index` is greater than or equal to `max_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)` shall fail and return `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_INVALID_ARGS`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_070: [** For each page from `start_index / page_size` to `end_index / page_size`, `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)` shall call `THREAD_SAFE_PAGED_SPARSE_ARRAY_TRY_FREE_PAGE(T)`. **]**

**SRS_THREAD_SAFE_PAGED_SPARSE_ARRAY_09_071: [** `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES(T)` shall succeed and return `THREAD_SAFE_PAGED_SPARSE_ARRAY_FREE_EMPTY_PAGES_OK`. **]**
