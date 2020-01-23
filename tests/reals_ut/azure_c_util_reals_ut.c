// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "../tests/reals/real_constbuffer.h"
#include "../tests/reals/real_constbuffer_array.h"
#include "../tests/reals/real_constbuffer_array_batcher.h"
#include "../tests/reals/real_crt_abstractions.h"
#include "../tests/reals/real_doublylinkedlist.h"
#include "../tests/reals/real_memory_data.h"
#include "../tests/reals/real_singlylinkedlist.h"
#include "../tests/reals/real_srw_lock.h"
#include "../tests/reals/real_string_utils.h"
#include "../tests/reals/real_timer.h"
#include "../tests/reals/real_uuid.h"

#include "azure_c_util/constbuffer.h"
#include "azure_c_util/constbuffer_array.h"
#include "azure_c_util/constbuffer_array_batcher.h"
#include "azure_c_util/crt_abstractions.h"
#include "azure_c_util/doublylinkedlist.h"
#include "azure_c_util/memory_data.h"
#include "azure_c_util/singlylinkedlist.h"
#include "azure_c_util/srw_lock.h"
#include "azure_c_util/string_utils.h"
#include "azure_c_util/timer.h"
#include "azure_c_util/uuid.h"

BEGIN_TEST_SUITE(azure_c_util_reals_ut)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_util_reals)
{
    // arrange

    // act
    REGISTER_CONSTBUFFER_GLOBAL_MOCK_HOOK();
    REGISTER_CONSTBUFFER_ARRAY_GLOBAL_MOCK_HOOK();
    REGISTER_CONSTBUFFER_ARRAY_BATCHER_GLOBAL_MOCK_HOOK();
    REGISTER_CRT_ABSTRACTIONS_GLOBAL_MOCK_HOOKS();
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_MEMORY_DATA_GLOBAL_MOCK_HOOK();
    REGISTER_SINGLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();
    REGISTER_TIMER_GLOBAL_MOCK_HOOK();
    REGISTER_UUID_GLOBAL_MOCK_HOOK();

    // assert
    // no explicit assert, if it builds it works
}

END_TEST_SUITE(azure_c_util_reals_ut)
