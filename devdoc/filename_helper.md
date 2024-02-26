`filename_helper` requirements
================

## Overview

`filename_helper` is a module for working with filenames.

It currently supports a way to append a suffix to a filename, which can be used to get checkpoint filenames.

## Exposed API

```c
MOCKABLE_FUNCTION(, char*, filename_append_suffix, const char*, filename, const char*, suffix);
```

Given `filename` and `suffix`, `filename_append_suffix` creates a new string that adds `suffix` to the filename before the extension. The user needs to call `free` on the returned value.

**SRS_FILENAME_HELPER_42_001: [** If `filename` is `NULL` then `filename_append_suffix` shall fail and return `NULL`. **]**

**SRS_FILENAME_HELPER_42_002: [** If `suffix` is `NULL` then `filename_append_suffix` shall fail and return `NULL`. **]**

**SRS_FILENAME_HELPER_42_003: [** If `filename` does not contain a `.` then `filename_append_suffix` shall return `filename` + `suffix`. **]**

**SRS_FILENAME_HELPER_42_004: [** If `filename` does not contain a `\` then `filename_append_suffix` shall return the name of the file + `suffix` + `.` + extension. **]**

**SRS_FILENAME_HELPER_42_005: [** If `filename` contains last `.` before last `\` then `filename_append_suffix` shall return `filename` + `suffix`. **]**

**SRS_FILENAME_HELPER_42_006: [** If `filename` contains last `.` after last `\` then `filename_append_suffix` shall return the name of the file + `suffix` + `.` + extension. **]**

**SRS_FILENAME_HELPER_42_007: [** If there are any failures then `filename_append_suffix` shall return `NULL`. **]**
