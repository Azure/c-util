uuid requirements
=================

## Overview
The UUID module provides functions to create and convert UUID values.

## Exposed API
```c
typedef unsigned char UUID_T[16];

extern const UUID_T NIL_UUID;

/* These 2 strings can be conveniently used directly in printf statements
  Notice that PRI_UUID has to be used like any other print format specifier, meaning it
  has to be preceded with % */
#define PRI_UUID        "02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x"
#define UUID_FORMAT_VALUES(uuid) \
    (uuid)[0], (uuid)[1], (uuid)[2], (uuid)[3], (uuid)[4], (uuid)[5], (uuid)[6], (uuid)[7], \
    (uuid)[8], (uuid)[9], (uuid)[10], (uuid)[11], (uuid)[12], (uuid)[13], (uuid)[14], (uuid)[15]

#define UUID_FORMAT_VALUES_OR_NULL(uuid) \
    ((uuid) == NULL) ? 0 : (uuid)[0], ((uuid) == NULL) ? 0 : (uuid)[1], ((uuid) == NULL) ? 0 : (uuid)[2], ((uuid) == NULL) ? 0 : (uuid)[3], \
    ((uuid) == NULL) ? 0 : (uuid)[4], ((uuid) == NULL) ? 0 : (uuid)[5], ((uuid) == NULL) ? 0 : (uuid)[6], ((uuid) == NULL) ? 0 : (uuid)[7], \
    ((uuid) == NULL) ? 0 : (uuid)[8], ((uuid) == NULL) ? 0 : (uuid)[9], ((uuid) == NULL) ? 0 : (uuid)[10], ((uuid) == NULL) ? 0 : (uuid)[11], \
    ((uuid) == NULL) ? 0 : (uuid)[12], ((uuid) == NULL) ? 0 : (uuid)[13], ((uuid) == NULL) ? 0 : (uuid)[14], ((uuid) == NULL) ? 0 : (uuid)[15] \

/* @brief               Generates a true UUID
*  @param uuid          A pre-allocated buffer for the bytes of the generated UUID
*  @returns             Zero if no failures occur, non-zero otherwise.
*/
MOCKABLE_FUNCTION(, int, UUID_generate, UUID_T*, uuid);

/* @brief               Gets the UUID value (byte sequence) of an well-formed UUID string.
*  @param uuid_string   A null-terminated well-formed UUID string (e.g., "7f907d75-5e13-44cf-a1a3-19a01a2b4528").
*  @param uuid          Sequence of bytes representing an UUID.
*  @returns             Zero if no failures occur, non-zero otherwise.
*/
MOCKABLE_FUNCTION(, int, UUID_from_string, const char*, uuid_string, UUID_T*, uuid);

/* @brief               Gets the string representation of the UUID value.
*  @param uuid          Sequence of bytes representing an UUID.
*  @returns             A null-terminated string representation of the UUID value provided (e.g., "7f907d75-5e13-44cf-a1a3-19a01a2b4528").
*/
MOCKABLE_FUNCTION(, char*, UUID_to_string, const UUID_T*, uuid);
```

###  UUID_generate

```c
MOCKABLE_FUNCTION(, int, UUID_generate, UUID_T*, uuid);
```

`UUID_generate` generates a `UUID_T`.

**SRS_UUID_09_001: [** If `uuid` is NULL, UUID_generate shall return a non-zero value **]**

**SRS_UUID_09_002: [** UUID_generate shall obtain an UUID string from UniqueId_Generate **]**

**SRS_UUID_09_003: [** If the UUID string fails to be obtained, UUID_generate shall fail and return a non-zero value **]**

**SRS_UUID_09_004: [** The UUID string shall be parsed into an UUID_T type (16 unsigned char array) and filled in `uuid` **]**  

**SRS_UUID_09_005: [** If `uuid` fails to be set, UUID_generate shall fail and return a non-zero value **]**

**SRS_UUID_09_006: [** If no failures occur, UUID_generate shall return zero **]**


###  UUID_from_string

```c
MOCKABLE_FUNCTION(, int, UUID_from_string, const char*, uuid_string, UUID_T*, uuid);
```

`UUID_from_string` converts a string to a `UUID_T`.

**SRS_UUID_09_007: [** If `uuid_string` or `uuid` are NULL, UUID_from_string shall return a non-zero value **]**

**SRS_UUID_09_008: [** Each pair of digits in `uuid_string`, excluding dashes, shall be read as a single HEX value and saved on the respective position in `uuid` **]**  

**SRS_UUID_09_009: [** If `uuid` fails to be generated, UUID_from_string shall return a non-zero value **]**

**SRS_UUID_09_010: [** If no failures occur, UUID_from_string shall return zero **]**


###  UUID_to_string

```c
MOCKABLE_FUNCTION(, char*, UUID_to_string, const UUID_T*, uuid);
```

`UUID_to_string` converts a `UUID_T` to a string.

**SRS_UUID_09_011: [** If `uuid` is NULL, UUID_to_string shall return a non-zero value **]**  

**SRS_UUID_09_012: [** UUID_to_string shall allocate a valid UUID string (`uuid_string`) as per RFC 4122 **]**  

**SRS_UUID_09_013: [** If `uuid_string` fails to be allocated, UUID_to_string shall return NULL **]**  

**SRS_UUID_09_014: [** Each character in `uuid` shall be written in the respective positions of `uuid_string` as a 2-digit HEX value **]**  

**SRS_UUID_09_015: [** If `uuid_string` fails to be set, UUID_to_string shall return NULL **]**  

**SRS_UUID_09_016: [** If no failures occur, UUID_to_string shall return `uuid_string` **]**  

### NIL_UUID

```c
extern const UUID_T NIL_UUID;
```

**SRS_UUID_01_001: [** `NIL_UUID` shall contain all zeroes. **]**
