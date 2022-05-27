// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#define REGISTER_GLOBAL_MOCK_HOOK(original, real) \
    (original == real) ? (void)0 : (void)1;

#include "../reals/real_constbuffer.h"
#include "../reals/real_constbuffer_array.h"
#include "../reals/real_constbuffer_array_batcher_nv.h"
#include "../reals/real_doublylinkedlist.h"
#include "../reals/real_external_command_helper.h"
#include "../reals/real_interlocked_hl.h"
#include "../reals/real_memory_data.h"
#include "../reals/real_rc_string.h"
#include "../reals/real_rc_string_array.h"
#include "../reals/real_singlylinkedlist.h"
#include "../reals/real_uuid_string.h"
#include "../reals/real_worker_thread.h"

#include "c_util/constbuffer.h"
#include "c_util/constbuffer_array.h"
#include "c_util/constbuffer_array_batcher_nv.h"
#include "c_util/doublylinkedlist.h"
#include "c_util/external_command_helper.h"
#include "c_util/interlocked_hl.h"
#include "c_util/memory_data.h"
#include "c_util/rc_string.h"
#include "c_util/rc_string_array.h"
#include "c_util/singlylinkedlist.h"
#include "c_util/uuid_string.h"

#if defined _MSC_VER
#include "../reals/real_sm.h"
#include "../reals/real_worker_thread.h"
#include "c_util/sm.h"
#include "c_util/worker_thread.h"
#endif

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

// this test makes sure that the mappings work
// (there is a real_ function corresponding to the original)
TEST_FUNCTION(check_all_c_util_reals)
{
    // arrange

    // act
    REGISTER_CONSTBUFFER_GLOBAL_MOCK_HOOK();
    REGISTER_CONSTBUFFER_ARRAY_GLOBAL_MOCK_HOOK();
    REGISTER_CONSTBUFFER_ARRAY_BATCHER_GLOBAL_MOCK_HOOK();
    REGISTER_DOUBLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_EXTERNAL_COMMAND_HELPER_GLOBAL_MOCK_HOOKS();
    REGISTER_INTERLOCKED_HL_GLOBAL_MOCK_HOOK();
    REGISTER_MEMORY_DATA_GLOBAL_MOCK_HOOK();
    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();
    REGISTER_SINGLYLINKEDLIST_GLOBAL_MOCK_HOOKS();
    REGISTER_UUID_STRING_GLOBAL_MOCK_HOOK();

#if defined _MSC_VER
    REGISTER_SM_GLOBAL_MOCK_HOOK();
    REGISTER_WORKER_THREAD_GLOBAL_MOCK_HOOK();
#endif

    // assert
    // no explicit assert. if it builds, it works
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
