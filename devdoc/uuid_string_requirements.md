uuid string requirements
=================

## Overview
The uuid string module provides functions to convert between UUID_T and const char*.

## Exposed API
```c

#define UUID_FROM_STRING_RESULT_VALUES \
    UUID_FROM_STRING_RESULT_OK, \
    UUID_FROM_STRING_RESULT_INVALID_DATA, \
    UUID_FROM_STRING_RESULT_INVALID_ARG

MU_DEFINE_ENUM(UUID_FROM_STRING_RESULT, UUID_FROM_STRING_RESULT_VALUES)

#define UUID_T_STRING_LENGTH 36 /*all UUID_T have 36 characters when stringified (not counting a '\0' terminator)*/

extern const UUID_T NIL_UUID;

MOCKABLE_FUNCTION(, UUID_FROM_STRING_RESULT, uuid_from_string, const char*, uuid_string, UUID_T, uuid);

MOCKABLE_FUNCTION(, char*, uuid_to_string, const UUID_T, uuid);
```

### uuid_from_string
```c
MOCKABLE_FUNCTION(, UUID_FROM_STRING_RESULT, uuid_from_string, const char*, uuid_string, UUID_T, uuid);
```

`uuid_from_string` fills `uuid`'s bytes with the values from its representation in `uuid_string`. The string representation is `hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh`, where `h` is a hex digit, either lower case, or upper case.

**SRS_UUD_STRING_02_001: [** If `uuid_string` is `NULL` then `uuid_from_string` shall fail and return `UUID_FROM_STRING_RESULT_INVALID_ARG`. **]**

**SRS_UUD_STRING_02_002: [** If `uuid` is `NULL` then `uuid_from_string` shall fail and return `UUID_FROM_STRING_RESULT_INVALID_ARG`. **]**

**SRS_UUD_STRING_02_003: [** If any character of `uuid_string` doesn't match the string representation `hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh` then `uuid_from_string` shall succeed and return `UUID_FROM_STRING_RESULT_INVALID_DATA`. **]**

**SRS_UUD_STRING_02_004: [** If any character of `uuid_string` is `\0` instead of a hex digit then `uuid_from_string` shall succeed and return `UUID_FROM_STRING_RESULT_INVALID_DATA`. **]**

**SRS_UUD_STRING_02_005: [** If any character of `uuid_string` is `\0` instead of a `-` then `uuid_from_string` shall succeed and return `UUID_FROM_STRING_RESULT_INVALID_DATA`. **]**

**SRS_UUD_STRING_02_006: [** `uuid_from_string` shall convert the hex digits to the bytes of `uuid`, succeed and return `UUID_FROM_STRING_RESULT_OK`. **]**


### uuid_to_string
```c
MOCKABLE_FUNCTION(, char*, uuid_to_string, const UUID_T, uuid);
```

`uuid_to_string` produces the string representation of `uuid`.

**SRS_UUD_STRING_02_007: [** If `uuid` is `NULL` then `uuid_to_string` shall fail and return `NULL`. **]**

**SRS_UUD_STRING_02_008: [** `uuid_to_string` shall output a `\0` terminated string in format `hhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhh` where every `h` is a nibble of one the bytes in `uuid`. **]**

**SRS_UUD_STRING_02_009: [** If there are any failures then `uuid_to_string` shall fail and return `NULL`. **]**


### NIL_UUID

```c
extern const UUID_T NIL_UUID;
```

**SRS_UUID_01_001: [** `NIL_UUID` shall contain all zeroes. **]**
