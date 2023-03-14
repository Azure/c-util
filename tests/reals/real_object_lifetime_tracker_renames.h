// Copyright (c) Microsoft. All rights reserved.

#define object_lifetime_tracker_create real_object_lifetime_tracker_create
#define object_lifetime_tracker_destroy real_object_lifetime_tracker_destroy
#define object_lifetime_tracker_register_object real_object_lifetime_tracker_register_object
#define object_lifetime_tracker_unregister_object real_object_lifetime_tracker_unregister_object
#define object_lifetime_tracker_destroy_all_objects_for_key real_object_lifetime_tracker_destroy_all_objects_for_key

#define OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT real_OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT
#define KEY_MATCH_FUNCTION_RESULT real_KEY_MATCH_FUNCTION_RESULT
#define OBJECT_MATCH_FUNCTION_RESULT real_OBJECT_MATCH_FUNCTION_RESULT
