# rc_string requirements
================

## Overview

`rc_string` is a module that encapsulates a reference counted string.

## Exposed API

```c
    typedef struct RC_STRING_TAG
    {
        const char* string;
    } RC_STRING;

    THANDLE_TYPE_DECLARE(RC_STRING);

    MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create, const char*, string);
```

## rc_string_create

```c
MOCKABLE_FUNCTION(, THANDLE(RC_STRING), rc_string_create, const char*, string);
```

`rc_string_create` created a new ref counted string by copying the ASCII string at `string`.

**SRS_RC_STRING_01_001: [** If `string` is `NULL`, `rc_string_create` shall fail and return `NULL`. **]**

**SRS_RC_STRING_01_002: [** Otherwise, `rc_string_create` shall determine the length of `string`. **]**

**SRS_RC_STRING_01_003: [** `rc_string_create` shall allocate memory for the `THANDLE(RC_STRING)`, ensuring all the bytes in `string` can be copied (including the zero terminator). **]**

**SRS_RC_STRING_01_004: [** `rc_string_create` shall copy the string memory (including the `NULL` terminator). **]**

**SRS_RC_STRING_01_005: [** `rc_string_create` shall succeed and return a non-`NULL` handle. **]**

**SRS_RC_STRING_01_006: [** If any error occurs, `rc_string_create` shall fail and return `NULL`. **]**
