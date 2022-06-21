// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_CONSTBUFFER_ARRAY_BATCHER_H
#define REAL_CONSTBUFFER_ARRAY_BATCHER_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_CONSTBUFFER_ARRAY_BATCHER_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        constbuffer_array_batcher_nv_batch, \
        constbuffer_array_batcher_nv_unbatch \
)


#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


#include "c_util/constbuffer_array.h"


CONSTBUFFER_ARRAY_HANDLE real_constbuffer_array_batcher_nv_batch(CONSTBUFFER_ARRAY_HANDLE* payloads, uint32_t count);
CONSTBUFFER_ARRAY_HANDLE* real_constbuffer_array_batcher_nv_unbatch(CONSTBUFFER_ARRAY_HANDLE batch, uint32_t* payload_count);




#endif // REAL_CONSTBUFFER_ARRAY_BATCHER_H
