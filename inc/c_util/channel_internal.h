// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//#ifndef CHANNEL_INTERNAL_H
//#define CHANNEL_INTERNAL_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle.h"
#include "c_pal/threadpool.h"
#include "c_pal/srw_lock.h"

#include "c_util/doublylinkedlist.h"

#include "umock_c/umock_c_prod.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#ifdef COMPILING_CHANNEL_C
typedef struct CHANNEL_INTERNAL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    SRW_LOCK_HANDLE lock;
    DLIST_ENTRY op_list;
}
CHANNEL_INTERNAL;
THANDLE_TYPE_DECLARE(CHANNEL_INTERNAL);
#endif

#ifdef TEST_CHANNEL
typedef struct TEST_CHANNEL_INTERNAL_TAG
{
    THANDLE(THREADPOOL) threadpool;
    SRW_LOCK_HANDLE lock;
    DLIST_ENTRY op_list;
}
TEST_CHANNEL_INTERNAL;
THANDLE_TYPE_DECLARE(TEST_CHANNEL_INTERNAL);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

//#endif // CHANNEL_INTERNAL_H
