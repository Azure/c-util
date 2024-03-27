// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_OBJECT_LIFETIME_TRACKER_H
#define REAL_OBJECT_LIFETIME_TRACKER_H

#include "macro_utils/macro_utils.h"
#include "c_util/object_lifetime_tracker.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_OBJECT_LIFETIME_TRACKER_GLOBAL_MOCK_HOOKS() \
    MU_FOR_EACH_1(R2, \
        object_lifetime_tracker_create, \
        object_lifetime_tracker_destroy, \
        object_lifetime_tracker_register_object, \
        object_lifetime_tracker_unregister_object, \
        object_lifetime_tracker_destroy_all_objects_for_key \
    )

#ifdef __cplusplus
#include <cstddef>
extern "C"
{
#else
#include <stddef.h>
#endif

OBJECT_LIFETIME_TRACKER_HANDLE real_object_lifetime_tracker_create(KEY_MATCH_FUNCTION key_match_function, OBJECT_MATCH_FUNCTION object_match_function);
void real_object_lifetime_tracker_destroy(OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker);

OBJECT_LIFETIME_TRACKER_REGISTER_OBJECT_RESULT real_object_lifetime_tracker_register_object(OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker, const void* key, void* object, DESTROY_OBJECT destroy_object, const void* destroy_context);
OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT real_object_lifetime_tracker_unregister_object(OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker, const void* key, void* object);
void real_object_lifetime_tracker_destroy_all_objects_for_key(OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker, const void* key);
OBJECT_LIFETIME_TRACKER_ACT_RESULT real_object_lifetime_tracker_act(OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker, const void* key, void* object, OBJECT_LIFETIME_TRACKER_ACTION_FUNCTION action_function, void* context);

#ifdef __cplusplus
}
#endif

#endif // REAL_OBJECT_LIFETIME_TRACKER_H
