# two_dimensional_array_requirements
============================

## Overview

`two_dimensional_array` is a module provide to declare a 2D array with variable size of rows and columns. Each rows has the same size to ensure easier access. The first dimentional of array is resizable, it contains the pointer to the second dimention of array. The pointer is intiazlied to NULL and points to the allocated memory address when there's not enough space. When the memory stored in a row is not needed any more, the entire row shall be freed and set back to `NULL`.

## Exposed API
```c
typedef struct TWO_D_ARRAY_TAG
{
    uint64_t rows;
    uint64_t cols;
} TWO_D_ARRAY_HANDLE;

MOCKABLE_FUNCTION(, TWO_D_ARRAY_HANDLE, two_d_array_create, uint64_t, row_size, uint_64_t, col_size);
MOCKABLE_FUNCTION(, int, two_d_array_free_row, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index);
MOCKABLE_FUNCTION(, int, two_d_array_allocate_new_row, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index);
MOCKABLE_FUNCTION(, void, two_d_array_destroy, TWO_D_ARRAY_HANDLE, handle);
```

### two_d_array_create

```c
MOCKABLE_FUNCTION(, TWO_D_ARRAY_HANDLE, two_d_array_create, uint64_t, row_size, uint_64_t, col_size);
```

`two_d_array_create` shall create a new empty two_dimensional_array.

If `row_size` or `col_size` equals to zero, `two_d_array_create` shall return `NULL`.

`two_d_array_create` shall allocate memory for the first dimentional of array using `row_size`.

`two_d_array_create` shall allocate memory for the first row using `col_size`.

`two_d_array_create` shall set the remaing rows except the first row to `NULL`.

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
MOCKABLE_FUNCTION(, int, two_d_array_allocate_new_row, TWO_D_ARRAY_HANDLE, handle, uint64_t, row_index);
```

`two_d_array_allocate_new_row` shall allocate the memory for a new row.

If `handle` is `NULL`, `two_d_array_allocate_new_row` shall fail return a non-zero value.

If `row_index` is equal or greater than `row_size`, `two_d_array_allocate_new_row` shall fail return a non-zero value.

`two_d_array_allocate_new_row` shall allocate memory for the new row and return zero on success.

If there are any errors then `two_d_array_allocate_new_row` shall fail and return a non-zero value.

### two_d_array_destroy

```c
MOCKABLE_FUNCTION(, void, two_d_array_destroy, TWO_D_ARRAY_HANDLE, handle);
```

`two_d_array_destroy` shall clean up the 2d array.

If `handle` is `NULL`, `two_d_array_destroy` shall fail and return.

`two_d_array_destroy` shall free all `non-NULL` rows and set to NULL.

`two_d_array_destroy` shall free the memory allcated for the first dimension of array.
