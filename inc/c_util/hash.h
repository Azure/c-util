// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
#include <cstdint>
#include <cstddef>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif

MOCKABLE_FUNCTION(, int, hash_compute_hash, const void*, buffer, size_t, length, uint32_t*, hash);

#ifdef __cplusplus
}
#endif

#endif /* HASH_H */
