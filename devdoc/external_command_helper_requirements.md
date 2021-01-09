`external_command_helper` requirements
================

## Overview

`external_command_helper` is a module that can execute another command and parse its output.

It will execute the command and read the output over a pipe to an array of strings. It also returns the exit code of the command.

The callers can parse and convert the actual output lines as needed.

Assumptions:
 - Each line of output is limited to 2048 characters (for now as there are no places where we expect it to be longer).

## Exposed API

```c
#define EXTERNAL_COMMAND_RESULT_VALUES \
        EXTERNAL_COMMAND_OK, \
        EXTERNAL_COMMAND_INVALID_ARGS, \
        EXTERNAL_COMMAND_ERROR

MU_DEFINE_ENUM(EXTERNAL_COMMAND_RESULT, EXTERNAL_COMMAND_RESULT_VALUES)

MOCKABLE_FUNCTION(, EXTERNAL_COMMAND_RESULT, external_command_helper_execute, const char*, command, RC_STRING_ARRAY**, lines, int*, return_code);
```

### external_command_helper_execute

```c
MOCKABLE_FUNCTION(, EXTERNAL_COMMAND_RESULT, external_command_helper_execute, const char*, command, RC_STRING_ARRAY**, lines, int*, return_code);
```

`external_command_helper_execute` executes the specified `command` and returns an array of the output lines to `lines` and the exit code of the command to `return_code`. It does not validate the output in any way.

**SRS_EXTERNAL_COMMAND_HELPER_42_001: [** If `command` is `NULL` then `external_command_helper_execute` shall fail and return `EXTERNAL_COMMAND_INVALID_ARGS`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_002: [** If `command` is empty string then `external_command_helper_execute` shall fail and return `EXTERNAL_COMMAND_INVALID_ARGS`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_003: [** If `lines` is `NULL` then `external_command_helper_execute` shall fail and return `EXTERNAL_COMMAND_INVALID_ARGS`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_004: [** If `return_code` is `NULL` then `external_command_helper_execute` shall fail and return `EXTERNAL_COMMAND_INVALID_ARGS`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_005: [** `external_command_helper_execute` shall call `popen` to execute the `command` and open a read pipe. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_006: [** `external_command_helper_execute` shall read each line of output into a 2048 byte buffer. **]**

 - **SRS_EXTERNAL_COMMAND_HELPER_42_007: [** If a line of output exceeds 2048 bytes then `external_command_helper_execute` shall fail and return `EXTERNAL_COMMAND_ERROR`. **]**

 - **SRS_EXTERNAL_COMMAND_HELPER_42_008: [** `external_command_helper_execute` shall remove the trailing new line. **]**

 - **SRS_EXTERNAL_COMMAND_HELPER_42_009: [** `external_command_helper_execute` shall allocate an array of `THANDLE(RC_STRING)` or grow the existing array to fit the new line. **]**

 - **SRS_EXTERNAL_COMMAND_HELPER_42_010: [** `external_command_helper_execute` shall allocate a `THANDLE(RC_STRING)` of the line by calling `rc_string_create` and store it in the array. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_011: [** `external_command_helper_execute` shall call `rc_string_array_create` with the count of output lines returned. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_012: [** `external_command_helper_execute` shall move all of the strings into the allocated `RC_STRING_ARRAY`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_013: [** `external_command_helper_execute` shall call `pclose` to close the pipe and get the exit code of the command. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_014: [** `external_command_helper_execute` shall store the exit code of the command in `return_code`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_015: [** `external_command_helper_execute` shall store the allocated `RC_STRING_ARRAY` in `lines`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_016: [** `external_command_helper_execute` shall succeed and return `EXTERNAL_COMMAND_OK`. **]**

**SRS_EXTERNAL_COMMAND_HELPER_42_017: [** If there are any other errors then `external_command_helper_execute` shall fail and return `EXTERNAL_COMMAND_ERROR`. **]**
