// Copyright (c) Microsoft. All rights reserved.

#include "windows.h"

#define GetLastError mocked_GetLastError
#define FindFirstFileA mocked_FindFirstFileA
#define FindNextFileA mocked_FindNextFileA
#define FindClose mocked_FindClose

extern DWORD mocked_GetLastError(void);

extern HANDLE mocked_FindFirstFileA(
    LPCSTR             lpFileName,
    LPWIN32_FIND_DATAA lpFindFileData
);

extern BOOL mocked_FindNextFileA(
    HANDLE             hFindFile,
    LPWIN32_FIND_DATAA lpFindFileData
);

extern BOOL mocked_FindClose(
    HANDLE hFindFile
);


#include "../../src/for_each_in_folder.c"
