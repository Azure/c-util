`for_each_in_sub_folder` requirements
================

## Overview

`for_each_in_sub_folder` is a stateless module that given a Windows path to a folder, will enumerate all constituents in that folder's folders (be those files, folders or other "things").

During the enumeration `for_each_in_sub_folder` calls a user supplied callback passing a user provided callback. The user can decide to stop the enumeration, continue, or stop with an error.

When `for_each_in_sub_folder` encounters an internal error it will stop the enumeration. It will also stop the enumeration when there are no more files to be found.


## Exposed API

```c
    typedef int(*ON_EACH_IN_FOLDER)(const char* folder, const WIN32_FIND_DATAA* findData, void* context, bool* enumerationShouldContinue);

MOCKABLE_FUNCTION(, int, for_each_in_sub_folder, const char*, folder, ON_EACH_IN_FOLDER, on_each_in_folder, void*, context);
```

### for_each_in_sub_folder

```c
MOCKABLE_FUNCTION(, int, for_each_in_sub_folder, const char*, folder, ON_EACH_IN_FOLDER, on_each_in_folder, void*, context);
```

`for_each_in_sub_folder` enumerates all constituents in `folder`'s folders. 

**SRS_FOR_EACH_IN_SUBFOLDER_02_001: [** If `folder` is `NULL` then `for_each_in_sub_folder` shall fail and return a non-zero value.  **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_002: [** If `on_each_in_folder` is `NULL` then `for_each_in_sub_folder` shall fail and return a non-zero value. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_003: [** `for_each_in_sub_folder` shall construct a new context that contains `on_each_in_folder` and `context`. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_004: [** `for_each_in_sub_folder` shall call `for_each_in_folder` with `folder` set to the same `folder`, `on_each_in_folder` set to `on_subfolder_folder` and `context` set the previous constructed context. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_005: [** `for_each_in_sub_folder` shall succeed and return 0. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_012: [** If there are any failure then `for_each_in_sub_folder` shall fail and return a non-zero value. **]**

### on_subfolder_folder
```c
static int on_subfolder_folder(const char* folder, const WIN32_FIND_DATAA* findData, void* context, bool* enumerationShouldContinue)
```

`on_subfolder_folder` is called from `for_each_in_sub_folder` for each constituent of the original `folder` passed to `for_each_in_sub_folder`.

**SRS_FOR_EACH_IN_SUBFOLDER_02_006: [** If `findData` does not have `FILE_ATTRIBUTE_DIRECTORY` flag set then `on_subfolder_folder` shall set `enumerationShouldContinue` to `true`, succeed, and return 0. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_007: [** If `findData->cFileName` is "." then `on_subfolder_folder` shall set `enumerationShouldContinue` to `true`, succeed, and return 0. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_013: [** If `findData->cFileName` is ".." then `on_subfolder_folder` shall set `enumerationShouldContinue` to `true`, succeed, and return 0. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_008: [** `on_subfolder_folder` shall assemble a string `folder`\\`findData->cFileName`. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_009: [** `on_subfolder_folder` shall call `for_each_in_folder` with `folder` set to the previous string, `on_each_in_folder` set to the original `on_each_in_folder` passed to `for_each_in_sub_folder` and `context` set to the original context passed to `for_each_in_sub_folder`. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_010: [** If there are any failures then `on_subfolder_folder` shall fail and return a non-zero value. **]**

**SRS_FOR_EACH_IN_SUBFOLDER_02_011: [** `on_subfolder_folder` shall set `enumerationShouldContinue` to `true`, succeed and return 0. **]**






