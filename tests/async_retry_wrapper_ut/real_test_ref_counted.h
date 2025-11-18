// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_TEST_REF_COUNTED_H
#define REAL_TEST_REF_COUNTED_H

#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_TEST_REF_COUNTED_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        test_refcounted_create, \
        test_refcounted_inc_ref, \
        test_refcounted_dec_ref \
    )

#ifdef __cplusplus
extern "C" {
#endif

#include "test_ref_counted.h"

    TEST_REFCOUNTED_HANDLE real_test_refcounted_create(void);
    void real_test_refcounted_inc_ref(TEST_REFCOUNTED_HANDLE test_refcounted);
    void real_test_refcounted_dec_ref(TEST_REFCOUNTED_HANDLE test_refcounted);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // REAL_TEST_REF_COUNTED_H
