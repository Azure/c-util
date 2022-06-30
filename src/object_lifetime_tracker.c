// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/srw_lock.h"

#include "c_util/doublylinkedlist.h"
#include "c_util/containing_record.h"

#include "c_util/object_lifetime_tracker.h"

typedef struct OBJECT_TAG
{
    void* object;
    DESTROY_OBJECT destroy_object;
    DLIST_ENTRY anchor;
} OBJECT;

typedef struct KEY_TAG
{
    const void* key;
    DLIST_ENTRY objects;
    DLIST_ENTRY anchor;
} KEY;

typedef struct OBJECT_LIFETIME_TRACKER_TAG
{
    DLIST_ENTRY keys;
    SRW_LOCK_HANDLE lock;
    KEY_MATCH_FUNCTION key_match_function;
    OBJECT_MATCH_FUNCTION object_match_function;
} OBJECT_LIFETIME_TRACKER;

typedef struct KEY_MATCH_CONTEXT_TAG
{
    const void* key;
    KEY_MATCH_FUNCTION key_match_function;
    PDLIST_ENTRY found_list_entry;
} KEY_MATCH_CONTEXT;

typedef struct OBJECT_MATCH_CONTEXT_TAG
{
    const void* object;
    OBJECT_MATCH_FUNCTION object_match_function;
    PDLIST_ENTRY found_list_entry;
} OBJECT_MATCH_CONTEXT;

MU_DEFINE_ENUM_STRINGS(KEY_MATCH_FUNCTION_RESULT, KEY_MATCH_FUNCTION_RESULT_VALUES)

MU_DEFINE_ENUM_STRINGS(OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT_VALUES)


static int is_same_key(PDLIST_ENTRY listEntry, void* context, bool* continueProcessing)
{
    int result;
    KEY* key_struct = CONTAINING_RECORD(listEntry, KEY, anchor);
    KEY_MATCH_CONTEXT* key_match_context = context;
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_050: [ is_same_key shall call key_match_function on the obtained key from listEntry and the key in key_match_context. ]*/
    switch (key_match_context->key_match_function(key_struct->key, key_match_context->key))
    {
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_054: [ If key_match_function returns KEY_MATCH_FUNCTION_RESULT_MATCHING: ]*/
    case KEY_MATCH_FUNCTION_RESULT_MATCHING:
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_055: [ is_same_key shall set continueProcessing to false. ]*/
        *continueProcessing = false;
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_056: [ is_same_key shall store listEntry in key_match_context ]*/
        key_match_context->found_list_entry = listEntry;
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_058: [ is_same_key shall succeed and return zero. ]*/
        result = 0;
        break;
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_057: [ If key_match_function returns KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING, is_same_key shall set continueProcessing to true. ]*/
    case KEY_MATCH_FUNCTION_RESULT_NOT_MATCHING:
        *continueProcessing = true;
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_058: [ is_same_key shall succeed and return zero. ]*/
        result = 0;
        break;
    default:
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_059: [ If there are any failures, is_same_key shall fail and return a non-zero value. ]*/
    case KEY_MATCH_FUNCTION_RESULT_ERROR:
        *continueProcessing = false;
        result = MU_FAILURE;
        break;
    }
    return result;
}


static int is_same_object(PDLIST_ENTRY listEntry, void* context, bool* continueProcessing)
{
    int result;
    OBJECT* object_struct = CONTAINING_RECORD(listEntry, OBJECT, anchor);
    OBJECT_MATCH_CONTEXT* object_match_context = context;
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_068: [ is_same_object shall call object_match_function on the obtained object from listEntry and the object in object_match_context. ]*/
    switch (object_match_context->object_match_function(object_struct->object, object_match_context->object))
    {
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_069: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_MATCHING: ]*/
    case OBJECT_MATCH_FUNCTION_RESULT_MATCHING:
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_070: [ is_same_object shall set continueProcessing to false. ]*/
        *continueProcessing = false;
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_071: [ is_same_object shall store listEntry in object_match_context. ]*/
        object_match_context->found_list_entry = listEntry;
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
        result = 0;
        break;
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_072: [ If object_match_function returns OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING, is_same_object shall set continueProcessing to true. ]*/
    case OBJECT_MATCH_FUNCTION_RESULT_NOT_MATCHING:
        *continueProcessing = true;
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_073: [ is_same_object shall succeed and return zero. ]*/
        result = 0;
        break;
    default:
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_074: [ If there are any failures, is_same_object shall fail and return a non-zero value. ]*/
    case OBJECT_MATCH_FUNCTION_RESULT_ERROR:
        *continueProcessing = false;
        result = MU_FAILURE;
        break;
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker_create, KEY_MATCH_FUNCTION, key_match_function, OBJECT_MATCH_FUNCTION, object_match_function)
{
    OBJECT_LIFETIME_TRACKER_HANDLE result;
    if (
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_001: [If key_match_function is NULL, object_lifetime_tracker_create shall fail and return NULL.]*/
        (key_match_function == NULL) ||
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_067: [ If object_match_function is NULL, object_lifetime_create shall fail and return NULL ]*/
        (object_match_function == NULL)
        )
    {
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_006 : [If there are any failures, object_lifetime_tracker_create shall fail and return NULL.]*/
        LogError("Invalid arguments: KEY_MATCH_FUNCTION key_match_function=%p", key_match_function);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_002 : [object_lifetime_tracker_create shall allocate memory for the object lifetime tracker.]*/
        result = malloc(sizeof(OBJECT_LIFETIME_TRACKER));
        if (result == NULL)
        {
            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_006 : [If there are any failures, object_lifetime_tracker_create shall fail and return NULL.]*/
            LogError("failure in malloc(sizeof(OBJECT_LIFETIME_TRACKER)=%zu);", sizeof(OBJECT_LIFETIME_TRACKER));
        }
        else
        {
            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_003: [object_lifetime_tracker_create shall call srw_lock_create.]*/
            result->lock = srw_lock_create(false, "object_lifetime_tracker");
            if (result->lock == NULL)
            {
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_006 : [If there are any failures, object_lifetime_tracker_create shall fail and return NULL.]*/
                LogError("failure in srw_lock_create(false, \"object_lifetime_tracker\");");
            }
            else
            {
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_004 : [object_lifetime_tracker_create shall call DList_InitializeListHead to initialize a DList for storing keys.]*/
                DList_InitializeListHead(&(result->keys));
                result->key_match_function = key_match_function;
                result->object_match_function = object_match_function;
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_005 : [object_lifetime_tracker_create shall succeed and return the created object lifetime tracker.]*/
                goto all_ok;
            }
            free(result);
            result = NULL;
        }
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, object_lifetime_tracker_destroy, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker)
{
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_007: [If object_lifetime_tracker is NULL, object_lifetime_tracker_destroy shall return.]*/
    if (object_lifetime_tracker == NULL)
    {
        LogError("object_lifetime_tracker is NULL");
    }
    else
    {
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_010 : [object_lifetime_tracker_destroy shall call srw_lock_destroy.]*/
        srw_lock_destroy(object_lifetime_tracker->lock);
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_011 : [object_lifetime_tracker_destroy shall free the memory for the object lifetime tracker.]*/
        free(object_lifetime_tracker);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, object_lifetime_tracker_register_object, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key, void*, object, DESTROY_OBJECT, destroy_object)
{
    int result;
    if (
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_012 : [If object_lifetime_tracker is NULL, object_lifetime_tracker_register_object shall fail and return a non - zero value.]*/
        (object_lifetime_tracker == NULL) ||
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_014 : [If object is NULL, object_lifetime_tracker_register_object shall fail and return a non - zero value.]*/
        (object == NULL) ||
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_015 : [If destroy_object is NULL, object_lifetime_tracker_register_object shall fail and return a non - zero value.]*/
        (destroy_object == NULL)
        )
    {
        LogError("Invalid arguments: OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker=%p, const void* key=%p, void* object=%p, DESTROY_OBJECT destroy_object=%p", object_lifetime_tracker, key, object, destroy_object);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_016: [ object_lifetime_tracker_register_object shall acquire the lock in exclusive mode. ]*/
        srw_lock_acquire_exclusive(object_lifetime_tracker->lock);

        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_060: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the object. ]*/
        OBJECT* object_struct = malloc(sizeof(OBJECT));
        if (object_struct == NULL)
        {
            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_021: [ If there are any failures, object_lifetime_tracker_register_object shall fail and return a non-zero value. ]*/
            LogError("failure in malloc(sizeof(OBJECT)=%zu)", sizeof(OBJECT));
            result = MU_FAILURE;
        }
        else
        {
            object_struct->object = object;
            object_struct->destroy_object = destroy_object;

            KEY_MATCH_CONTEXT key_match_context = { .key = key, .key_match_function = object_lifetime_tracker->key_match_function, .found_list_entry = NULL };
            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_017: [ object_lifetime_tracker_register_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
            if (DList_ForEach(&(object_lifetime_tracker->keys), is_same_key, &key_match_context) != 0)
            {
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_021: [ If there are any failures, object_lifetime_tracker_register_object shall fail and return a non-zero value. ]*/
                LogError("failure in DList_ForEach(&(object_lifetime_tracker->keys), is_same_key, &key_match_context)");
                result = MU_FAILURE;
            }
            else
            {
                KEY* found_key_struct;
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_045: [ If the key is not found in the DList of keys: ]*/
                if (key_match_context.found_list_entry == NULL)
                {
                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_043: [ object_lifetime_tracker_register_object shall allocate memory to store data associated with the key. ]*/
                    KEY* key_struct = malloc(sizeof(KEY));
                    if (key_struct == NULL)
                    {
                        LogError("failure in malloc(sizeof(KEY)=%zu);", sizeof(KEY));
                        found_key_struct = NULL;
                    }
                    else
                    {
                        key_struct->key = key;
                        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_044: [ object_lifetime_tracker_register_object shall initialize a DList to store objects associated with the key by calling DList_InitializeListHead. ]*/
                        DList_InitializeListHead(&(key_struct->objects));
                        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_018: [ object_lifetime_tracker_register_object shall add the given key to the DList of keys by calling DList_InsertHeadList. ]*/
                        DList_InsertHeadList(&(object_lifetime_tracker->keys), &(key_struct->anchor));
                        found_key_struct = key_struct;
                    }
                }
                else
                {
                    // key was found in object_lifetime_tracker->keys
                    found_key_struct = CONTAINING_RECORD(key_match_context.found_list_entry, KEY, anchor);
                }
                if (found_key_struct == NULL)
                {
                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_021: [ If there are any failures, object_lifetime_tracker_register_object shall fail and return a non-zero value. ]*/
                    LogError("failure while creating new key.");
                    result = MU_FAILURE;
                }
                else
                {
                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_019: [ object_lifetime_tracker_register_object shall store the given object and the given destroy_object in the DList of objects for given key by calling DList_InsertHeadList. ]*/
                    DList_InsertHeadList(&(found_key_struct->objects), &(object_struct->anchor));
                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_051: [ object_lifetime_tracker_register_object shall release the lock. ]*/
                    srw_lock_release_exclusive(object_lifetime_tracker->lock);
                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_020: [ object_lifetime_tracker_register_object shall succeed and return zero. ]*/
                    result = 0;
                    goto all_ok;
                }
            }
            free(object_struct);
        }
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_051: [ object_lifetime_tracker_register_object shall release the lock. ]*/
        srw_lock_release_exclusive(object_lifetime_tracker->lock);
    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT, object_lifetime_tracker_unregister_object, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key, void*, object)
{

    OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_RESULT result;
    if (
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_022: [If object_lifetime_tracker is NULL, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR.]*/
        (object_lifetime_tracker == NULL) ||
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_024: [If object is NULL, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR.]*/
        (object == NULL)
        )
    {
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_032: [ If there are any failures, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR. ]*/
        LogError("Invalid arguments: OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker=%p, const void* key=%p, void* object=%p", object_lifetime_tracker, key, object);
        result = OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR;
    }
    else
    {
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_025: [ object_lifetime_tracker_unregister_object shall acquire the lock in exclusive mode. ]*/
        srw_lock_acquire_exclusive(object_lifetime_tracker->lock);

        KEY_MATCH_CONTEXT key_match_context = { .key = key, .key_match_function = object_lifetime_tracker->key_match_function, .found_list_entry = NULL };
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_026: [ object_lifetime_tracker_unregister_object shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
        if (DList_ForEach(&(object_lifetime_tracker->keys), is_same_key, &key_match_context) != 0)
        {
            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_032: [ If there are any failures, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR. ]*/
            LogError("failure in DList_ForEach(&(object_lifetime_tracker->keys), is_same_key, &key_match_context)");
            result = OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR;
        }
        else
        {
            if (key_match_context.found_list_entry == NULL)
            {
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_046: [ If the given key is not found, object_lifetime_tracker_unregister_object shall return OBJECT_LIFETIME_TRACKER_UNREGISTER_KEY_NOT_FOUND. ]*/
                LogError("key=%p was not found in list of keys", key);
                result = OBJECT_LIFETIME_TRACKER_UNREGISTER_KEY_NOT_FOUND;
            }
            else
            {
                KEY* key_struct = CONTAINING_RECORD(key_match_context.found_list_entry, KEY, anchor);
                OBJECT_MATCH_CONTEXT object_match_context = { .object = object, .object_match_function = object_lifetime_tracker->object_match_function, .found_list_entry = NULL };
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_027: [ object_lifetime_tracker_unregister_object shall find the list entry for the given object in the DList of objects for the given key by calling DList_ForEach with is_same_object. ]*/
                if (DList_ForEach(&(key_struct->objects), is_same_object, &object_match_context) != 0)
                {
                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_032: [ If there are any failures, object_lifetime_tracker_unregister_object shall fail and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR. ]*/
                    LogError("failure in DList_ForEach(&(key_struct->objects), is_same_object, &object_match_context)");
                    result = OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_ERROR;
                }
                else
                {
                    if (object_match_context.found_list_entry == NULL)
                    {
                        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_047: [ If the given object is not found, object_lifetime_tracker_unregister_object shall return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_NOT_FOUND. ]*/
                        LogError("object=%p was not found in list of objects", object);
                        result = OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_NOT_FOUND;
                    }
                    else
                    {
                        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_029: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given object from the DList of objects for the given key by calling DList_RemoveEntryList. ]*/
                        int is_object_list_empty = DList_RemoveEntryList(object_match_context.found_list_entry);
                        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_061: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given object. ]*/
                        free(CONTAINING_RECORD(object_match_context.found_list_entry, OBJECT, anchor));

                        if (is_object_list_empty)
                        {
                            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_030: [ If the DList of objects for the given key is empty: ]*/
                            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_062: [ object_lifetime_tracker_unregister_object shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
                            (void)DList_RemoveEntryList(&(key_struct->anchor));
                            /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_063: [ object_lifetime_tracker_unregister_object shall free the memory associated with the given key. ]*/
                            free(key_struct);
                        }
                        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_031: [ object_lifetime_tracker_unregister_object shall succeed and return OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK. ]*/
                        result = OBJECT_LIFETIME_TRACKER_UNREGISTER_OBJECT_OK;
                    }
                }
            }
        }
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_052: [ object_lifetime_tracker_unregister_object shall release the lock. ]*/
        srw_lock_release_exclusive(object_lifetime_tracker->lock);
    }
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, object_lifetime_tracker_destroy_all_objects_for_key, OBJECT_LIFETIME_TRACKER_HANDLE, object_lifetime_tracker, const void*, key)
{
    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_033: [ If object_lifetime_tracker is NULL, object_lifetime_tracker_destroy_all_objects_for_key shall return. ]*/
    if (object_lifetime_tracker == NULL)
    {
        LogError("Invalid arguments: OBJECT_LIFETIME_TRACKER_HANDLE object_lifetime_tracker=%p, const void* key=%p", object_lifetime_tracker, key);
    }
    else
    {
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_035: [ object_lifetime_tracker_destroy_all_objects_for_key shall acquire the lock in exclusive mode. ]*/
        srw_lock_acquire_exclusive(object_lifetime_tracker->lock);

        KEY_MATCH_CONTEXT key_match_context = { .key = key, .key_match_function = object_lifetime_tracker->key_match_function, .found_list_entry = NULL };
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_036: [ object_lifetime_tracker_destroy_all_objects_for_key shall find the list entry for the given key in the DList of keys by calling DList_ForEach with is_same_key. ]*/
        if (DList_ForEach(&(object_lifetime_tracker->keys), is_same_key, &key_match_context) != 0)
        {
            LogError("failure in DList_ForEach(&(object_lifetime_tracker->keys), is_same_key, &key_match_context)");
        }
        else
        {
            if (key_match_context.found_list_entry == NULL)
            {
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_048: [ If the given key is not found, object_lifetime_tracker_destroy_all_objects_for_key shall return. ]*/
                LogError("Could not find key=%p in list of keys", key);
            }
            else
            {
                KEY* key_struct = CONTAINING_RECORD(key_match_context.found_list_entry, KEY, anchor);
                PDLIST_ENTRY removed_entry;
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_038: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entries for all the objects in the DList of objects for the given key by calling DList_RemoveHeadList for each list entry. ]*/
                while ((removed_entry = DList_RemoveHeadList(&(key_struct->objects))) != &(key_struct->objects))
                {
                    OBJECT* object_struct = CONTAINING_RECORD(removed_entry, OBJECT, anchor);

                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_037: [ object_lifetime_tracker_destroy_all_objects_for_key shall destroy all the objects in the DList of objects for the given key in the reverse order in which they were registered. ]*/
                    object_struct->destroy_object(object_struct->object);

                    /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_066: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with all the objects. ]*/
                    free(object_struct);
                }
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_039: [ object_lifetime_tracker_destroy_all_objects_for_key shall remove the list entry for the given key from the DList of keys by calling DList_RemoveEntryList. ]*/
                (void)DList_RemoveEntryList(&(key_struct->anchor));
                /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_065: [ object_lifetime_tracker_destroy_all_objects_for_key shall free the memory associated with the given key. ]*/
                free(key_struct);
            }
        }
        /*Codes_SRS_OBJECT_LIFETIME_TRACKER_43_053: [ object_lifetime_tracker_destroy_all_objects_for_key shall release the lock. ]*/
        srw_lock_release_exclusive(object_lifetime_tracker->lock);
    }
}
