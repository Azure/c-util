// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef RC_STRING_TEST_DECL_H
#define RC_STRING_TEST_DECL_H

#include "c_util/rc_string.h"
#include "c_util/thandle.h"
#include "real_rc_string.h"

#include "macro_utils/macro_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// The following is used to simplify the transition to THANDLE(RC_STRING) in tests

/*
With normal strings, test code would have:
    static const char foo = "bar";
Then just use foo

With THANDLE(RC_STRING), we want:
    static THANDLE(RC_STRING) foo;
And later it needs to be initialized and destroyed
And we can't modify "static THANDLE(RC_STRING)"
So really we want:
    static struct G_TAG
    {
        THANDLE(RC_STRING) foo;
    } g;

But foo needs to be initialized in c++ (C just lets that slide, and we can't do default brace initialization)

So the solution is a macro:

RC_STRING_TEST_DECL(
    foo, "bar",
    ...
)

With one setup function to call
    rc_string_test_init_statics();

And one cleanup function to call:
    rc_string_test_cleanup_statics();
*/

#ifdef __cplusplus
#define RC_STRING_TEST_DECL_MEMBER(name, value) THANDLE(RC_STRING) name = {};
#else
#define RC_STRING_TEST_DECL_MEMBER(name, value) THANDLE(RC_STRING) name;
#endif

#define RC_STRING_TEST_INIT_RC_STRING(name, value) \
    THANDLE(RC_STRING) MU_C2A(name, _temp) = real_rc_string_create(value); \
    ASSERT_IS_NOT_NULL(MU_C2A(name, _temp)); \
    THANDLE_INITIALIZE_MOVE(real_RC_STRING)(&g.name, &MU_C2A(name, _temp));

#define RC_STRING_TEST_CLEANUP_RC_STRING(name, value) \
    THANDLE_ASSIGN(real_RC_STRING)(&g.name, NULL);

#define RC_STRING_TEST_DECL(...) \
    static struct G_TAG \
    { \
        MU_FOR_EACH_2(RC_STRING_TEST_DECL_MEMBER, __VA_ARGS__) \
    } g; \
    static void rc_string_test_init_statics(void) \
    { \
        MU_FOR_EACH_2(RC_STRING_TEST_INIT_RC_STRING, __VA_ARGS__) \
    } \
    static void rc_string_test_cleanup_statics(void) \
    { \
        MU_FOR_EACH_2(RC_STRING_TEST_CLEANUP_RC_STRING, __VA_ARGS__) \
    }


/* Another version which can generate this:

static struct x_TAG
{
    THANDLE(RC_STRING) foo[COUNT];
} x;

from this:

RC_STRING_TEST_ARRAY_DECL(
    x, foo, COUNT,
    "string1",
    "string2",
    "string3")

Note that COUNT must be a compile-time constant in C (so an integer or DEFINE to integer)
*/

#ifdef __cplusplus
#define RC_STRING_TEST_ARRAY_DECL_MEMBER(name, count) THANDLE(RC_STRING) name[count] = {};
#else
#define RC_STRING_TEST_ARRAY_DECL_MEMBER(name, count) THANDLE(RC_STRING) name[count];
#endif

#define RC_STRING_TEST_INIT_ARRAY_RC_STRING(struct_name, field_name, value) \
    { \
        THANDLE(RC_STRING) MU_C2A(field_name, _temp) = real_rc_string_create(value); \
        ASSERT_IS_NOT_NULL(MU_C2A(field_name, _temp)); \
        ASSERT_IS_TRUE(i < struct_name.count);\
        THANDLE_INITIALIZE_MOVE(real_RC_STRING)(&struct_name.field_name[i++], &MU_C2A(field_name, _temp)); \
    }

#define RC_STRING_TEST_CLEANUP_ARRAY_RC_STRING(name, value) \
    THANDLE_ASSIGN(real_RC_STRING)(&g.name, NULL);

#define RC_STRING_TEST_ARRAY_DECL(struct_name, field_name, array_count, ...) \
    static struct MU_C2(struct_name, _TAG) \
    { \
        RC_STRING_TEST_ARRAY_DECL_MEMBER(field_name, array_count) \
        uint32_t count; \
    } struct_name; \
    static void MU_C2(rc_string_test_init_array_, struct_name)(void) \
    { \
        struct_name.count = array_count; \
        uint32_t i = 0; \
        MU_FOR_EACH_1_KEEP_2(RC_STRING_TEST_INIT_ARRAY_RC_STRING, struct_name, field_name, __VA_ARGS__) \
    } \
    static void MU_C2(rc_string_test_cleanup_array_, struct_name)(void) \
    { \
        for (uint32_t i = 0; i < struct_name.count; ++i) \
        { \
            THANDLE_ASSIGN(real_RC_STRING)(&struct_name.field_name[i], NULL); \
        } \
    }

#ifdef __cplusplus
}
#endif

#endif /* RC_STRING_TEST_DECL_H */
