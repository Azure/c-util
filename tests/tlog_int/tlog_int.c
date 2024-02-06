// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license.See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/interlocked.h"
#include "c_pal/thandle.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_util/tlog.h"
#include "tlog_foo.h"

// TLOG(FOO) is used for most int tests
// It is declared and defined in its own .h/.c files in order to emulate usage in the wilderness

// A queue with THANDLEs!
typedef struct TEST_THANDLE_TAG
{
    int64_t a_value;
} TEST_THANDLE;

THANDLE_TYPE_DECLARE(TEST_THANDLE);
THANDLE_TYPE_DEFINE(TEST_THANDLE);

TLOG_DEFINE_STRUCT_TYPE(THANDLE(TEST_THANDLE));
THANDLE_TYPE_DECLARE(TLOG_TYPEDEF_NAME(THANDLE(TEST_THANDLE)))
TLOG_TYPE_DECLARE(THANDLE(TEST_THANDLE));

THANDLE_TYPE_DEFINE(TLOG_TYPEDEF_NAME(THANDLE(TEST_THANDLE)))
TLOG_TYPE_DEFINE(THANDLE(TEST_THANDLE));

#define XTEST_FUNCTION(A) void A(void)

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(TLOG_PUSH_RESULT, TLOG_PUSH_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(TLOG_POP_RESULT, TLOG_POP_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(method_init)
{
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

TEST_FUNCTION(TLOG_CREATE_succeds)
{
    // arrange

    // act
    TLOG(FOO) queue = TLOG_CREATE(FOO)(16, NULL, NULL, NULL);

    // assert
    ASSERT_IS_NOT_NULL(queue);

    // clean
    TLOG_ASSIGN(FOO)(&queue, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
