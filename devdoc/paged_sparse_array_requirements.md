# `paged_sparse_array` requirements

## Overview

`PAGED_SPARSE_ARRAY` is a module that provides a templatized paged sparse array data structure. The array has a maximum size and allocates memory in pages (groups of elements) on demand. When all elements in a page are released, the page is automatically freed.

`PAGED_SPARSE_ARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `PAGED_SPARSE_ARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
- `PAGED_SPARSE_ARRAY_INITIALIZE(T)`
- `PAGED_SPARSE_ARRAY_ASSIGN(T)`
- `PAGED_SPARSE_ARRAY_MOVE(T)`
- `PAGED_SPARSE_ARRAY_INITIALIZE_MOVE(T)`

## Design

The array is organized into pages. Each page contains `page_size` elements. The total number of elements that can be stored is `max_size`. Pages are allocated lazily when an element is first allocated within that page. When all elements in a page are released, the page is automatically freed.

Each element has an allocation state (allocated or not allocated). The user can:
- Allocate an index for usage (fails if already allocated)
- Release an index (when all indexes in a page are released, the page is freed)
- Allocate or get an index (allocates if not exists, gets if exists)
- Get an index (fails if not allocated)

### Threading Model

`PAGED_SPARSE_ARRAY` is a `THANDLE`, which means that the ownership of the array (reference counting, assignment, move) is thread-safe. However, the operations on the array contents (ALLOCATE, RELEASE, ALLOCATE_OR_GET, GET) are **not thread-safe**.

If multiple threads need to access the same `PAGED_SPARSE_ARRAY` instance, the caller must provide external synchronization (e.g., using a mutex or SRW lock) to protect against concurrent modifications.

### Memory Layout

Elements in the array are **not stored contiguously in memory**. Each page is allocated separately, so elements in different pages are in different memory locations. Even elements within the same page should not be accessed via pointer arithmetic.

**Important:** Pointer arithmetic does not work with this data structure. For example:
```c
T* elem0 = PAGED_SPARSE_ARRAY_GET(T)(array, 0);
T* elem1 = PAGED_SPARSE_ARRAY_GET(T)(array, 1);
// elem0 + 1 is NOT guaranteed to equal elem1
```

Always use the provided API functions (GET, ALLOCATE, etc.) to access elements by index.

## Exposed API

```c
/*to be used as the type of handle that wraps T*/
#define PAGED_SPARSE_ARRAY(T)

/*to be used in a header file*/
#define PAGED_SPARSE_ARRAY_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define PAGED_SPARSE_ARRAY_TYPE_DEFINE(T)
```

The macros expand to these useful APIs:

```c
PAGED_SPARSE_ARRAY(T) PAGED_SPARSE_ARRAY_CREATE(T)(uint32_t max_size, uint32_t page_size);
T* PAGED_SPARSE_ARRAY_ALLOCATE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
void PAGED_SPARSE_ARRAY_RELEASE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
T* PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
T* PAGED_SPARSE_ARRAY_GET(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
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
PAGED_SPARSE_ARRAY(T) PAGED_SPARSE_ARRAY_CREATE(T)(uint32_t max_size, uint32_t page_size);
```

`PAGED_SPARSE_ARRAY_CREATE(T)` creates a new paged sparse array with the specified maximum size and page size.

**SRS_PAGED_SPARSE_ARRAY_88_001: [** If `max_size` is zero, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_002: [** If `page_size` is zero, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_040: [** If `max_size + page_size - 1` would overflow, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_003: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall compute the number of pages as `(max_size + page_size - 1) / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_004: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` to allocate memory for the paged sparse array with the number of pages. **]**

**SRS_PAGED_SPARSE_ARRAY_88_005: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall set all page pointers to `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_006: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall store `max_size` and `page_size` in the structure. **]**

**SRS_PAGED_SPARSE_ARRAY_88_007: [** If there are any errors, `PAGED_SPARSE_ARRAY_CREATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_008: [** `PAGED_SPARSE_ARRAY_CREATE(T)` shall succeed and return a non-`NULL` value. **]**

### PAGED_SPARSE_ARRAY_DISPOSE(T)

```c
static void PAGED_SPARSE_ARRAY_DISPOSE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array);
```

`PAGED_SPARSE_ARRAY_DISPOSE(T)` is the internal dispose function called by `THANDLE` when all references to the paged sparse array are released.

**SRS_PAGED_SPARSE_ARRAY_88_009: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_DISPOSE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_010: [** `PAGED_SPARSE_ARRAY_DISPOSE(T)` shall free all pages that are non-`NULL`. **]**

### PAGED_SPARSE_ARRAY_ALLOCATE(T)

```c
T* PAGED_SPARSE_ARRAY_ALLOCATE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
```

`PAGED_SPARSE_ARRAY_ALLOCATE(T)` allocates an element at the specified index for usage.

**SRS_PAGED_SPARSE_ARRAY_88_011: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_012: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_013: [** `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_014: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall allocate memory for the page containing `page_size` elements and an allocation bitmap, and initialize all elements as not allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_015: [** If the element at `index` is already allocated, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_016: [** `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall mark the element at `index` as allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_017: [** `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall return a pointer to the element at `index`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_018: [** If there are any errors, `PAGED_SPARSE_ARRAY_ALLOCATE(T)` shall fail and return `NULL`. **]**

### PAGED_SPARSE_ARRAY_RELEASE(T)

```c
void PAGED_SPARSE_ARRAY_RELEASE(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
```

`PAGED_SPARSE_ARRAY_RELEASE(T)` releases the element at the specified index.

**SRS_PAGED_SPARSE_ARRAY_88_019: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_020: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_021: [** `PAGED_SPARSE_ARRAY_RELEASE(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_022: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_023: [** If the element at `index` is not allocated, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall return. **]**

**SRS_PAGED_SPARSE_ARRAY_88_024: [** `PAGED_SPARSE_ARRAY_RELEASE(T)` shall mark the element at `index` as not allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_025: [** If all elements in the page are now not allocated, `PAGED_SPARSE_ARRAY_RELEASE(T)` shall free the page and set the page pointer to `NULL`. **]**

### PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)

```c
T* PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
```

`PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` allocates the element at the specified index if not already allocated, or gets it if it exists.

**SRS_PAGED_SPARSE_ARRAY_88_027: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_028: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_029: [** `PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_030: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` shall allocate memory for the page containing `page_size` elements and an allocation bitmap, and initialize all elements as not allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_031: [** If the element at `index` is not allocated, `PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` shall mark it as allocated. **]**

**SRS_PAGED_SPARSE_ARRAY_88_032: [** `PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` shall return a pointer to the element at `index`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_033: [** If there are any errors, `PAGED_SPARSE_ARRAY_ALLOCATE_OR_GET(T)` shall fail and return `NULL`. **]**

### PAGED_SPARSE_ARRAY_GET(T)

```c
T* PAGED_SPARSE_ARRAY_GET(T)(PAGED_SPARSE_ARRAY(T) paged_sparse_array, uint32_t index);
```

`PAGED_SPARSE_ARRAY_GET(T)` gets the element at the specified index.

**SRS_PAGED_SPARSE_ARRAY_88_034: [** If `paged_sparse_array` is `NULL`, `PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_035: [** If `index` is greater than or equal to `max_size`, `PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_036: [** `PAGED_SPARSE_ARRAY_GET(T)` shall compute the page index as `index / page_size`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_037: [** If the page is not allocated, `PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_038: [** If the element at `index` is not allocated, `PAGED_SPARSE_ARRAY_GET(T)` shall fail and return `NULL`. **]**

**SRS_PAGED_SPARSE_ARRAY_88_039: [** `PAGED_SPARSE_ARRAY_GET(T)` shall return a pointer to the element at `index`. **]**
