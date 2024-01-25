# `two_d_array` requirements
============================

## Overview

`TWODARRAY` is a module provides a 2D array with variable size of rows and columns. Each row has the same size to ensure easier access. The first dimension of array is resizable, it contains the pointer to the second dimension of array. The pointer is initialized to NULL and points to the allocated memory address when there's not enough space. When the memory stored in a row is not needed any more, the entire row shall be freed and set back to `NULL`.

`TWODARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `TWODARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
`TWODARRAY_INITIALIZE(T)`
`TWODARRAY_ASSIGN(T)`
`TWODARRAY_MOVE(T)`
`TWODARRAY_INITIALIZE_MOVE(T)`


## Exposed API

```c

/*to be used as the type of handle that wraps T - it has "rows", "cols" and "row_arrays"*/
#define TWODARRAY(T)

/*to be used in a header file*/
#define TWODARRAY_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define TWODARRAY_TYPE_DEFINE(T)
```

The macros expand to these useful somewhat more useful APIs:

```c

TWODARRAY(T) TWODARRAY_CREATE(T)(uint64_t row_size, uint64_t col_size);
int TWODARRAY_FREE_ROW(T)(TWODARRAY(T) two_d_array, uint64_t row_index);
int TWODARRAY_ALLOCATE_NEW_ROW(T)(TWODARRAY(T) two_d_array, uint64_t row_index);
T* TWODARRAY_GET(T)(TWODARRAY(T) two_d_array, uint64_t row_index);
void TWODARRAY_FREE(T)(TWODARRAY(T) two_d_array);

```

### TWODARRAY(T)

```c
#define TWODARRAY(T)
```
`TWODARRAY(T)` is a `THANDLE`(`TWODARRAY_STRUCT_T`), where `TWODARRAY_STRUCT_T` is a structure introduced like below:
```c
typedef struct TWODARRAY_STRUCT_T_TAG
{
    uint64_t rows;
    uint64_t cols;
    T * row_arrays[];
}TWODARRAY_STRUCT_T;
```

### TWODARRAY_TYPE_DECLARE(T)
```c
#define TWODARRAY_TYPE_DECLARE(T)
```

`TWODARRAY_TYPE_DECLARE(T)` is a macro to be used in a header declaration.

It introduces the APIs (as MOCKABLE_FUNCTIONS) that can be called for a `TWODARRAY`.

Example usage:

```c
TWODARRAY_TYPE_DECLARE(int32_t);
```

### TWODARRAY_TYPE_DEFINE(T)
```c
#define TWODARRAY_TYPE_DEFINE(T)
```

`TWODARRAY_TYPE_DEFINE(T)` is a macro to be used in a .c file to define all the needed functions for `TWODARRAY(T)`.

Example usage:

```c
TWODARRAY_TYPE_DEFINE(int32_t);
```


### TWODARRAY_CREATE(T)

```c
TWODARRAY(T) TWODARRAY_CREATE(T)(uint64_t row_size, uint64_t col_size);
```

`TWODARRAY_CREATE(T)` shall create a new empty two dimensional array.

If `row_size` smaller or equals to zero, `TWODARRAY_CREATE(T)` shall fail and return `NULL`.

If `col_size` smaller or equals to zero, `TWODARRAY_CREATE(T)` shall fail and return `NULL`.

`TWODARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` with `TWODARRAY_FREE(T)` as dispose function, `nmemb` set to `row_size` and `size` set to `sizeof(T*)`.

`TWODARRAY_CREATE(T)` shall set all rows point to `NULL`.

If there are any errors then `TWODARRAY_CREATE(T)` shall fail and return `NULL`.

`TWODARRAY_CREATE(T)` shall succeed and return a non-`NULL` value.

### TWODARRAY_FREE(T)

```c
void TWODARRAY_FREE(T)(TWODARRAY(T) two_d_array);
```

`TWODARRAY_FREE(T)` is called when there are no more references to the 2d array and the contents of it should be disposed of.

If `two_d_array` is `NULL`, `TWODARRAY_FREE(T)` shall fail and return.

`TWODARRAY_FREE(T)` shall free all rows with value (T*).

`TWODARRAY_FREE(T)` shall free the memory associated with `TWODARRAY(T)`.

### TWODARRAY_FREE_ROW(T)

```c
int TWODARRAY_FREE_ROW(T)(TWODARRAY(T) two_d_array, uint64_t row_index);
```

`TWODARRAY_FREE_ROW(T)` shall free the row specified by `row_index` and set it to `NULL`.

If `two_d_array` is `NULL`, `TWODARRAY_FREE_ROW(T)` shall fail return a non-zero value.

If `row_index` is equal or greater than `row_size`, `TWODARRAY_FREE_ROW(T)` shall fail and return a non-zero value.

`TWODARRAY_FREE_ROW(T)` shall free the memory associated with the row specified by `row_index` and set it to NULL.

`TWODARRAY_FREE_ROW(T)` shall return zero on success.

### TWODARRAY_ALLOCATE_NEW_ROW(T)

```c
int TWODARRAY_ALLOCATE_NEW_ROW(T)(TWODARRAY(T) two_d_array, uint64_t row_index);
```

`TWODARRAY_ALLOCATE_NEW_ROW(T)` shall allocate the memory for a new row.

If `two_d_array` is `NULL`, `TWODARRAY_ALLOCATE_NEW_ROW(T)` shall fail return a non-zero value.

If `row_index` is equal or greater than `row_size`, `TWODARRAY_ALLOCATE_NEW_ROW(T)` shall fail return a non-zero value.

If the row specified by `row_index` already been allocated, `TWODARRAY_ALLOCATE_NEW_ROW(T)` fail and return a non-zero value.

Otherwise, `TWODARRAY_ALLOCATE_NEW_ROW(T)` shall allocate memory for the new row and return zero on success.

If there are any errors then `TWODARRAY_ALLOCATE_NEW_ROW(T)` shall fail and return a non-zero value.

### TWODARRAY_GET(T)

```c
T* TWODARRAY_GET(T)(TWODARRAY(T) two_d_array, uint64_t row_index);
```

`TWODARRAY_GET(T)` shall return the entire column stored in `row_index`.

If `two_d_array` is `NULL`, `TWODARRAY_GET(T)` shall fail return `NULL`.

If `row_index` is equal or greater than `row_size`, `TWODARRAY_GET(T)` shall fail return `NULL`.

If the array stored in `row_index` is `NULL`, `TWODARRAY_GET(T)` shall fail and return `NULL`.

Otherwise, `TWODARRAY_GET(T)` shall return the entire column stored in the correspding `row_index`.
