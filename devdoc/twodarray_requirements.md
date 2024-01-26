# `two_d_array` requirements
============================

## Overview

`TWO_D_ARRAY` is a module provides a 2D array with variable size of rows and columns. Each row has the same size to ensure easier access. The first dimension of array is resizable, it contains the pointer to the second dimension of array. The pointer is initialized to NULL and points to the allocated memory address when there's not enough space. When the memory stored in a row is not needed any more, the entire row shall be freed and set back to `NULL`.

`TWO_D_ARRAY` is a kind of `THANDLE`, all of the `THANDLE`'s API apply to `TWO_D_ARRAY`. The following macros are provided with the same semantics as those of `THANDLE`'s:
`TWO_D_ARRAY_INITIALIZE(T)`
`TWO_D_ARRAY_ASSIGN(T)`
`TWO_D_ARRAY_MOVE(T)`
`TWO_D_ARRAY_INITIALIZE_MOVE(T)`


## Exposed API

```c

/*to be used as the type of handle that wraps T - it has "rows", "cols" and "row_arrays"*/
#define TWO_D_ARRAY(T)

/*to be used in a header file*/
#define TWO_D_ARRAY_TYPE_DECLARE(T)

/*to be used in a .c file*/
#define TWO_D_ARRAY_TYPE_DEFINE(T)
```

The macros expand to these useful somewhat more useful APIs:

```c

TWO_D_ARRAY(T) TWO_D_ARRAY_CREATE(T)(uint64_t row_size, uint64_t col_size);
int TWO_D_ARRAY_FREE_ROW(T)(TWO_D_ARRAY(T) two_d_array, uint64_t row_index);
int TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)(TWO_D_ARRAY(T) two_d_array, uint64_t row_index);
T* TWO_D_ARRAY_GET(T)(TWO_D_ARRAY(T) two_d_array, uint64_t row_index);
void TWO_D_ARRAY_FREE(T)(TWO_D_ARRAY(T) two_d_array);

```

### TWO_D_ARRAY(T)

```c
#define TWO_D_ARRAY(T)
```
`TWO_D_ARRAY(T)` is a `THANDLE`(`TWO_D_ARRAY_STRUCT_T`), where `TWO_D_ARRAY_STRUCT_T` is a structure introduced like below:

```c
typedef struct TWO_D_ARRAY_STRUCT_T_TAG
{
    uint64_t rows;
    uint64_t cols;
    T * row_arrays[];
}TWO_D_ARRAY_STRUCT_T;
```

### TWO_D_ARRAY_TYPE_DECLARE(T)
```c
#define TWO_D_ARRAY_TYPE_DECLARE(T)
```

`TWO_D_ARRAY_TYPE_DECLARE(T)` is a macro to be used in a header declaration.

It introduces the APIs (as MOCKABLE_FUNCTIONS) that can be called for a `TWO_D_ARRAY`.

Example usage:

```c
TWO_D_ARRAY_TYPE_DECLARE(int32_t);
```

### TWO_D_ARRAY_TYPE_DEFINE(T)
```c
#define TWO_D_ARRAY_TYPE_DEFINE(T)
```

`TWO_D_ARRAY_TYPE_DEFINE(T)` is a macro to be used in a .c file to define all the needed functions for `TWO_D_ARRAY(T)`.

Example usage:

```c
TWO_D_ARRAY_TYPE_DEFINE(int32_t);
```


### TWO_D_ARRAY_CREATE(T)

```c
TWO_D_ARRAY(T) TWO_D_ARRAY_CREATE(T)(uint64_t row_size, uint64_t col_size);
```

`TWO_D_ARRAY_CREATE(T)` shall create a new empty two dimensional array.

If `row_size` equals to zero, `TWO_D_ARRAY_CREATE(T)` shall fail and return `NULL`.

If `col_size` equals to zero, `TWO_D_ARRAY_CREATE(T)` shall fail and return `NULL`.

`TWO_D_ARRAY_CREATE(T)` shall call `THANDLE_MALLOC_FLEX` with `TWO_D_ARRAY_FREE(T)` as dispose function, `nmemb` set to `row_size` and `size` set to `sizeof(T*)`.

`TWO_D_ARRAY_CREATE(T)` shall set all rows point to `NULL`.

If there are any errors then `TWO_D_ARRAY_CREATE(T)` shall fail and return `NULL`.

`TWO_D_ARRAY_CREATE(T)` shall succeed and return a non-`NULL` value.

### TWO_D_ARRAY_FREE(T)

```c
void TWO_D_ARRAY_FREE(T)(TWO_D_ARRAY(T) two_d_array);
```

`TWO_D_ARRAY_FREE(T)` is called when there are no more references to the 2d array and the contents of it should be disposed of.

If `two_d_array` is `NULL`, `TWO_D_ARRAY_FREE(T)` shall do nothing.

`TWO_D_ARRAY_FREE(T)` shall free all rows with value (T*).

`TWO_D_ARRAY_FREE(T)` shall free the memory associated with `TWO_D_ARRAY(T)`.

### TWO_D_ARRAY_FREE_ROW(T)

```c
int TWO_D_ARRAY_FREE_ROW(T)(TWO_D_ARRAY(T) two_d_array, uint64_t row_index);
```

`TWO_D_ARRAY_FREE_ROW(T)` shall free the row specified by `row_index` and set it to `NULL`.

If `two_d_array` is `NULL`, `TWO_D_ARRAY_FREE_ROW(T)` shall fail return a non-zero value.

If `row_index` is equal or greater than `row_size`, `TWO_D_ARRAY_FREE_ROW(T)` shall fail and return a non-zero value.

`TWO_D_ARRAY_FREE_ROW(T)` shall free the memory associated with the row specified by `row_index` and set it to `NULL`.

`TWO_D_ARRAY_FREE_ROW(T)` shall return zero on success.

### TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)

```c
int TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)(TWO_D_ARRAY(T) two_d_array, uint64_t row_index);
```

`TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)` shall allocate the memory for a new row.

If `two_d_array` is `NULL`, `TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)` shall fail return a non-zero value.

If `row_index` is equal or greater than `row_size`, `TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)` shall fail return a non-zero value.

If the row specified by `row_index` already been allocated, `TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)` fail and return a non-zero value.

Otherwise, `TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)` shall allocate memory for the new row and return zero on success.

If there are any errors then `TWO_D_ARRAY_ALLOCATE_NEW_ROW(T)` shall fail and return a non-zero value.

### TWO_D_ARRAY_GET(T)

```c
T* TWO_D_ARRAY_GET(T)(TWO_D_ARRAY(T) two_d_array, uint64_t row_index);
```

`TWO_D_ARRAY_GET(T)` shall return the entire column stored in `row_index`.

If `two_d_array` is `NULL`, `TWO_D_ARRAY_GET(T)` shall fail return `NULL`.

If `row_index` is equal or greater than `row_size`, `TWO_D_ARRAY_GET(T)` shall fail return `NULL`.

If the array stored in `row_index` is `NULL`, `TWO_D_ARRAY_GET(T)` shall fail and return `NULL`.

Otherwise, `TWO_D_ARRAY_GET(T)` shall return the entire column stored in the correspding `row_index`.
