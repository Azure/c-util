// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef THANDLE_STDLIB_H
#define THANDLE_STDLIB_H

#ifdef __cplusplus
#include <cstddef>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    void* THANDLE_MALLOC_FUNCTION(size_t size);
    void THANDLE_FREE_FUNCTION(void* ptr);

#ifdef __cplusplus
}
#endif

#endif  /*THANDLE_STDLIB_H*/
