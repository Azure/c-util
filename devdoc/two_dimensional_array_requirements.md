# two_dimensional_array_requirements
============================

## Overview

`two_dimensional_array` provides a 2D array with variable size of rows and columns. Each row has the same size to ensure easier access. The first dimention of array is resizable, it contains the pointer to the second dimention of array. The pointer is initiazlied to NULL and points to the allocated memory address when there's not enough space. When the memory stored in a row is not needed any more, the entire row shall be freed and set back to `NULL`.

## Exposed API
```c
typedef struct TWO_D_ARRAY_TAG
{
    uint64_t rows;
    uint64_t cols;
    void * row_arrays[];
}TWO_D_ARRAY;

typedef struct TWO_D_ARRAY_TAG* TWO_D_ARRAY_HANDLE;
MOCKABLE_FUNCTION(, TWO_D_ARRAY_HANDLE, two_d_array_create, uint64_t, row_size);
MOCKABLE_FUNCTION(, int, two_d_array_free_row, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index);
MOCKABLE_FUNCTION(, int, two_d_array_allocate_new_row, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index, size_t, item_size);
MOCKABLE_FUNCTION(, int, two_d_array_add, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index, uint64_t, col_index, void*, item);
MOCKABLE_FUNCTION(, void*, two_d_array_get, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index, uint64_t, col_index);
MOCKABLE_FUNCTION(, void, two_d_array_destroy, TWO_D_ARRAY_HANDLE, handle);
```

### two_d_array_create

```c
MOCKABLE_FUNCTION(, TWO_D_ARRAY_HANDLE, two_d_array_create, uint64_t, row_size);
```

`two_d_array_create` shall create a new empty two_dimensional_array.

If `row_size` equals to zero, `two_d_array_create` shall return `NULL`.

`two_d_array_create` shall allocate memory for the first dimention of array using `row_size`.

`two_d_array_create` shall set the remaining rows except the first row to `NULL`.

If there are any errors then `two_d_array_create` shall fail and return `NULL`.

`two_d_array_create` shall succeed and return the allocated `TWO_D_ARRAY_HANDLE`.

### two_d_array_free_row

```c
MOCKABLE_FUNCTION(, int, two_d_array_free_first_row, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index);
```

`two_d_array_free_first_row` shall free the row specified by `row_index` and set it to `NULL`.

If `handle` is `NULL`, `two_d_array_free_first_row` shall fail return a non-zero value.

`two_d_array_free_first_row` shall free the memory associated with the row specified by `row_index` and set it to NULL.

`two_d_array_free_first_row` shall return zero on success.

### two_d_array_allocate_new_row

```c
MOCKABLE_FUNCTION(, int, two_d_array_allocate_new_row, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index, size_t, item_size);
```

`two_d_array_allocate_new_row` shall allocate the memory for a new row.

If `handle` is `NULL`, `two_d_array_allocate_new_row` shall fail return a non-zero value.

If `row_index` is equal or greater than `row_size`, `two_d_array_allocate_new_row` shall fail return a non-zero value.

`two_d_array_allocate_new_row` shall allocate memory for the new row and return zero on success.

If there are any errors then `two_d_array_allocate_new_row` shall fail and return a non-zero value.

### two_d_array_add

```c
MOCKABLE_FUNCTION(, int, two_d_array_add, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index, uint64_t, col_index, void*, item);
```

`two_d_array_add` shall add a new item in the 2d array.

If `handle` is `NULL`, `two_d_array_add` shall fail return a non-zero value.

If `row_index` is equal or greater than `row_size`, `two_d_array_add` shall fail return a non-zero value.

If `col_index` is equal or greater than `row_size`, `two_d_array_add` shall fail return a non-zero value.

If the array stored in `row_index` is `NULL`, `two_d_array_add` shall fail and return a non-zero value.

Otherwise, `two_d_array_add` shall store the `item` in the correspding `row_index` and `col_index`.

`two_d_array_add` shall return 0 on success.

### two_d_array_get

```c
MOCKABLE_FUNCTION(, void*, two_d_array_get, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index, uint64_t, col_index);
```

`two_d_array_get` shall return the item stored in `row_index` and `col_index`.

If `handle` is `NULL`, `two_d_array_get` shall fail return `NULL`.

If `row_index` is equal or greater than `row_size`, `two_d_array_get` shall fail return `NULL`.

If `col_index` is equal or greater than `row_size`, `two_d_array_get` shall fail return `NULL`.

If the array stored in `row_index` is `NULL`, `two_d_array_get` shall fail and return `NULL`.

Otherwise, `two_d_array_get` shall return the `item` stored in the correspding `row_index` and `col_index`.

### two_d_array_destroy

```c
MOCKABLE_FUNCTION(, void, two_d_array_destroy, TWO_D_ARRAY_HANDLE, handle);
```

`two_d_array_destroy` shall clean up the 2d array.

If `handle` is `NULL`, `two_d_array_destroy` shall fail and return.

`two_d_array_destroy` shall free all `non-NULL` rows and set to NULL.

`two_d_array_destroy` shall free the memory allocated for the first dimension of array.
