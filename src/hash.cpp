// Copyright (C) Microsoft Corporation. All rights reserved.

#include <cstdint>
#include <climits>
#include <cstddef>
#include "macro_utils/macro_utils.h"
#include "c_util/hash.h"
#include "c_logging/xlogging.h"
#include "MurmurHash2.h"

int hash_compute_hash(const void* buffer, size_t length, uint32_t* hash)
{
    int result;

    /* Codes_SRS_HASH_01_004: [ If buffer is NULL, hash_compute_hash shall fail and return a non-zero value. ]*/
    if ((buffer == NULL) ||
        /* Codes_SRS_HASH_01_005: [ If length is 0, hash_compute_hash shall fail and return a non-zero value. ]*/
        (length == 0) ||
        /* Codes_SRS_HASH_01_006: [ If hash is NULL, hash_compute_hash shall fail and return a non-zero value. ]*/
        (hash == NULL) ||
        /* Codes_SRS_HASH_01_002: [ If length is greater than or equal to INT_MAX, hash_compute_hash shall fail and return a non-zero value. ]*/
        (length >= INT_MAX))
    {
        LogError("Invalid arguments: buffer = %p, length = %zu, hash = %p",
            buffer, length, hash);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_HASH_01_001: [ hash_compute_hash shall call MurmurHash2, while passing as arguments buffer, length and 0 as seed. ]*/
        *hash = MurmurHash2(buffer, (int)length, 0);

        /* Codes_SRS_HASH_01_003: [ On success hash_compute_hash shall return 0 and fill in hash the computed hash value. ]*/
        result = 0;
    }

    return result;
}
