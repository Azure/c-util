// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef GUID_AS_STRING_H
#define GUID_AS_STRING_H

#include "windows.h"

#ifdef __cplusplus
extern "C" {
#endif

    const char* getGuidAsString(void);
    void freeGuidAsString(const char* guid);
    int getGuidFromString(const char* guidAsString, GUID* guid);

#ifdef __cplusplus
}
#endif

#endif /* GUID_AS_STRING_H */
