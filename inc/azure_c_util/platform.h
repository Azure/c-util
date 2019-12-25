// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PLATFORM_H
#define PLATFORM_H

#include "umock_c/umock_c_prod.h"

#define GUID_LENGTH 64

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    MOCKABLE_FUNCTION(, int, platform_init);
    MOCKABLE_FUNCTION(, void, platform_deinit);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PLATFORM_H */
