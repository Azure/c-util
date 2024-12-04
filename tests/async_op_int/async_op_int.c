// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>


#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"


#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "c_pal/thandle.h"

#include "c_util/async_op.h"

static const uint32_t context_aligns[] =
{
    /*only powers of 2 are allowed */
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 4096, 8192
};

static const uint32_t context_sizes[] =
{
    /*powers of 2... */
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 4096, 8192,

    /*around 64/128/256 byte mutiple */
    62, 63, 65, 66, 126,127, 129, 130, 254, 255, 257, 258,  

    /*non power of 2*/
    3, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 17,

    /*then some random numbers*/
    345, 56, 487, 2829, 930, 881, 9230, 87309, 2173
};

/*this function exists only to have a real address somewhere, which is different from any other address*/
static void ASYNC_OP_CANCEL_IMPL_1(void* context)
{
    (void)context;
    /*doesn't do anything*/
}

static int ASYNC_OP_CANCEL_IMPL_2_ANTI_COMDAT = 0;
/*this function exists only to have a real address somewhere, which is different from any other address*/
static void ASYNC_OP_CANCEL_IMPL_2(void* context)
{
    ASYNC_OP_CANCEL_IMPL_2_ANTI_COMDAT += (context == NULL); /*just use context and ASYNC_OP_CANCEL_IMPL_2_ANTI_COMDAT somehow*/
    /*doesn't do anything*/
}

static ASYNC_OP_CANCEL_IMPL cancels[] =
{
    NULL, 
    ASYNC_OP_CANCEL_IMPL_1,
    ASYNC_OP_CANCEL_IMPL_2
};

/*this function exists only to have a real address somewhere, which is different from NULL*/
static void ASYNC_OP_DISPOSE_1(void* context)
{
    (void)context;
    /*doesn't do anything*/
}

static int ASYNC_OP_DISPOSE_2_ANTI_COMDAT = 0;
/*this function exists only to have a real address somewhere, which is different from any other address*/
static void ASYNC_OP_DISPOSE_2(void* context)
{
    ASYNC_OP_DISPOSE_2_ANTI_COMDAT += (context == NULL); /*just use context and ASYNC_OP_DISPOSE_2_ANTI_COMDAT somehow*/
}

static ASYNC_OP_DISPOSE disposes[] =
{
    NULL,
    ASYNC_OP_DISPOSE_1,
    ASYNC_OP_DISPOSE_2
};

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(setsBufferTempSize)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(function_init)
{
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

TEST_FUNCTION(async_op_from_context_with_alignment_1)
{
    for (uint32_t i_align = 0; i_align < sizeof(context_aligns) / sizeof(context_aligns[0]); i_align++)
    {
        for (uint32_t i_size = 0; i_size < sizeof(context_sizes) / sizeof(context_sizes[0]); i_size++)
        {
            for (uint32_t i_cancel = 0; i_cancel < sizeof(cancels) / sizeof(cancels[0]); i_cancel++)
            {
                for (uint32_t i_dispose = 0; i_dispose < sizeof(disposes) / sizeof(disposes[0]); i_dispose++)
                {
                    ///arrange
                    THANDLE(ASYNC_OP) async_op = async_op_create(cancels[i_cancel], context_sizes[i_size], context_aligns[i_align], disposes[i_dispose]);

                    ASSERT_IS_NOT_NULL(async_op);
                    void* context = async_op->context;

                    ///act(1) - check that context is writable for at least context_size[i_size] bytes
                    (void)memset(context, '3', context_sizes[i_size]); /*asserts that the memory is USABLE by setting all bytes to '3'. for valgrind/fsanitize*/

                    ///act(2) - get the THANDLE(ASYNC_OP) from the context
                    THANDLE(ASYNC_OP) result = async_op_from_context(context); /*note: this THANDLE(ASYNC_OP) does not need to be THANDLE_ASSIGN(ASYNC_OP)(&result, NULL) by convention*/

                    ///assert
                    ASSERT_IS_NOT_NULL(result);
                    ASSERT_ARE_EQUAL(void_ptr, async_op, result);

                    /*did we return some ASYNC_OP which matches even remotely the context?*/
                    /*check that the context stored in the returned ASYNC_OP matches the context which was given to the user*/
                    /*check that inside the ASYNC_OP other fields are... as expected*/
                    ASSERT_ARE_EQUAL(void_ptr, result->context, context);
                    ASSERT_ARE_EQUAL(void_ptr, result->HERE_BE_DRAGONS_DO_NOT_USE.cancel, cancels[i_cancel]);
                    ASSERT_ARE_EQUAL(void_ptr, result->HERE_BE_DRAGONS_DO_NOT_USE.dispose, disposes[i_dispose]);
                    ASSERT_IS_TRUE((void*)result->HERE_BE_DRAGONS_DO_NOT_USE.private_context <= (void*)context);

                    ///clean
                    THANDLE_ASSIGN(ASYNC_OP)(&async_op, NULL);
                }
            }
        }
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
