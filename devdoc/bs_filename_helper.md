`bs_filename_helper` requirements
================

## Overview

`bs_filename_helper` is a module for working with filenames.

It currently supports a way to append a suffix to a filename, which can be used to get checkpoint filenames.

## Exposed API

```c
MOCKABLE_FUNCTION(, char*, bs_filename_append_suffix, const char*, filename, const char*, suffix);
```

Given `filename` and `suffix`, `bs_filename_append_suffix` creates a new string that adds `suffix` to the filename before the extension. The user needs to call `free` on the returned value.

**SRS_BS_FILENAME_HELPER_42_001: [** If `filename` is `NULL` then `bs_filename_append_suffix` shall fail and return `NULL`. **]**

**SRS_BS_FILENAME_HELPER_42_002: [** If `suffix` is `NULL` then `bs_filename_append_suffix` shall fail and return `NULL`. **]**

**SRS_BS_FILENAME_HELPER_42_003: [** If `filename` does not contain a `.` then `bs_filename_append_suffix` shall return `filename` + `suffix`. **]**

**SRS_BS_FILENAME_HELPER_42_004: [** If `filename` does not contain a `\` then `bs_filename_append_suffix` shall return the name of the file + `suffix` + `.` + extension. **]**

**SRS_BS_FILENAME_HELPER_42_005: [** If `filename` contains last `.` before last `\` then `bs_filename_append_suffix` shall return `filename` + `suffix`. **]**

**SRS_BS_FILENAME_HELPER_42_006: [** If `filename` contains last `.` after last `\` then `bs_filename_append_suffix` shall return the name of the file + `suffix` + `.` + extension. **]**

**SRS_BS_FILENAME_HELPER_42_007: [** If there are any failures then `bs_filename_append_suffix` shall return `NULL`. **]**
