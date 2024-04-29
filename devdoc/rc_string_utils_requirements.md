# `rc_string_utils` requirements

## Overview

`rc_string_utils` contains utility functions for acting on `RC_STRING`s.

## Exposed API

```c
MOCKABLE_FUNCTION(, RC_STRING_ARRAY*, rc_string_utils_split_by_char, THANDLE(RC_STRING), str, char, delimiter);
```

### rc_string_utils_split_by_char

```c
MOCKABLE_FUNCTION(, RC_STRING_ARRAY*, rc_string_utils_split_by_char, THANDLE(RC_STRING), str, char, delimiter);
```

Given a `THANDLE(RC_STRING)` `str` and a `delimiter` character, `rc_string_utils_split_by_char` splits the `str` at each `delimiter` character and produces an `RC_STRING_ARRAY` containing all of the substrings that were split. This implementation does not modify the original string `str` and copies each sub-string.

For example, this may be used to convert a comma-separated string into the individual components.

**SRS_RC_STRING_UTILS_42_001: [** If `str` is `NULL` then `rc_string_utils_split_by_char` shall fail and return `NULL`. **]**

**SRS_RC_STRING_UTILS_42_002: [** `rc_string_utils_split_by_char` shall count the delimiter characters in `str` and add 1 to compute the number of resulting sub-strings. **]**

**SRS_RC_STRING_UTILS_42_003: [** `rc_string_utils_split_by_char` shall call `rc_string_array_create` with the count of split strings. **]**

**SRS_RC_STRING_UTILS_42_011: [** If only 1 sub-string is found (`delimiter` is not found in `str`) then `rc_string_utils_split_by_char` shall call `THANDLE_ASSIGN(RC_STRING)` to copy `str` into the allocated array. **]**

**SRS_RC_STRING_UTILS_42_004: [** For each delimiter found in the string, plus the null-terminator: **]**

 - **SRS_RC_STRING_UTILS_42_005: [** `rc_string_utils_split_by_char` shall allocate memory for the sub-string and a null-terminator. **]**

 - **SRS_RC_STRING_UTILS_42_006: [** `rc_string_utils_split_by_char` shall copy the sub-string into the allocated memory. **]**

 - **SRS_RC_STRING_UTILS_42_007: [** `rc_string_utils_split_by_char` shall call `rc_string_create_with_move_memory`. **]**

 - **SRS_RC_STRING_UTILS_42_008: [** `rc_string_utils_split_by_char` shall call `THANDLE_MOVE(RC_STRING)` to move the allocated string into the allocated array. **]**

**SRS_RC_STRING_UTILS_42_009: [** `rc_string_utils_split_by_char` shall return the allocated array. **]**

**SRS_RC_STRING_UTILS_42_010: [** If there are any errors then `rc_string_utils_split_by_char` shall fail and return `NULL`. **]**
