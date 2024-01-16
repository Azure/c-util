`for_each_in_folder` requirements
================

## Overview

`for_each_in_folder` is a stateless module that given a Windows path to a folder, will enumerate all constituents in that folder (be those files, folders or other "things").

During the enumeration `for_each_in_folder` calls a user supplied callback passing a user provided callback. The user can decide to stop the enumeration, continue, or stop with an error.

When `for_each_in_folder` encounters an internal error it will stop the enumeration. It will also stop the enumeration when there are no more files to be found.


## Exposed API

```c
    typedef int(*ON_EACH_IN_FOLDER)(const char* folder, const WIN32_FIND_DATAA* findData, void* context, bool* enumerationShouldContinue);

MOCKABLE_FUNCTION(, int, for_each_in_folder, const char*, folder, ON_EACH_IN_FOLDER, on_each_in_folder, void*, context);
```

### for_each_in_folder

```c
MOCKABLE_FUNCTION(, int, for_each_in_folder, const char*, folder, ON_EACH_IN_FOLDER, on_each_in_folder, void*, context);
```

`for_each_in_folder` enumerates using FindFirst/FindNext the files/subfolders in `folder`, calls the supplied `on_each_in_folder` and stop enumerating either when `on_each_in_folder` indicates so, when there are no more files to enumerate, or when there is a failure.

**SRS_FOR_EACH_IN_FOLDER_02_001: [** If `folder` is `NULL` then `for_each_in_folder` shall fail and return a non-zero value. **]**

**SRS_FOR_EACH_IN_FOLDER_02_002: [** If `on_each_in_folder` is `NULL` then `for_each_in_folder` shall fail and return a non-zero value. **]**

**SRS_FOR_EACH_IN_FOLDER_02_003: [** `for_each_in_folder` shall assemble the string `folder\\*` to enumerate all constituents in `folder`. **]**

**SRS_FOR_EACH_IN_FOLDER_02_004: [** `for_each_in_folder` shall call `FindFirstFileA` with `lpFileName` set to the previously build string. **]**

**SRS_FOR_EACH_IN_FOLDER_02_005: [** If `FindFirstFileA` fails and `GetLastError` indicates either `ERROR_FILE_NOT_FOUND` or `ERROR_PATH_NOT_FOUND` then `for_each_in_folder` shall succeed and return 0. **]**

**SRS_FOR_EACH_IN_FOLDER_02_006: [** If `GetLastError` indicates any other error then shall fail and return a non-zero value. **]**

**SRS_FOR_EACH_IN_FOLDER_02_007: [** `for_each_in_folder` shall call `on_each_in_folder` passing `folder`, the discovered `WIN32_FIND_DATAA` and `context`. **]**

**SRS_FOR_EACH_IN_FOLDER_02_008: [** If `on_each_in_folder` fails then `for_each_in_folder` shall fail and return a non-zero value. **]**

**SRS_FOR_EACH_IN_FOLDER_02_009: [** If `on_each_in_folder` indicates that the enumeration should stop then `for_each_in_folder` shall stop further call to `FindNextFileA`. **]**

**SRS_FOR_EACH_IN_FOLDER_02_010: [** `for_each_in_folder` shall discover the next item in folder by a call to `FindNextFileA`.  **]**

**SRS_FOR_EACH_IN_FOLDER_02_011: [** If `FindNextFileA` fails and `GetLastError` returns `ERROR_NO_MORE_FILES` then `for_each_in_folder` shall stop further call to `FindNextFileA`. **]**

**SRS_FOR_EACH_IN_FOLDER_02_012: [** If `FindNextFileA` fails and `GetLastError` returns any other value then `for_each_in_folder` shall fail and return a non-zero value. **]**

**SRS_FOR_EACH_IN_FOLDER_02_013: [** `for_each_in_folder` shall call `on_each_in_folder` passing `folder`, `WIN32_FIND_DATAA` discovered by `FindNextFileA` and `context`. **]**

**SRS_FOR_EACH_IN_FOLDER_02_014: [** `for_each_in_folder` shall call `FindClose`. **]**

**SRS_FOR_EACH_IN_FOLDER_02_015: [** `for_each_in_folder` shall succeed and return 0. **]**

**SRS_FOR_EACH_IN_FOLDER_02_016: [** If there are any failures then `for_each_in_folder` shall fail and return a non-zero value. **]**


