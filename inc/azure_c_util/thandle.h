#ifndef THANDLE_H
#define THANDLE_H

#include <stdlib.h>

#include "azure_macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#include "azure_c_util/xlogging.h"
#include "refcount_os.h"

/*the incomplete unassignable type*/
#define THANDLE(T) const T* const

#define THANDLE_EXTRA_FIELDS(type) \
    volatile COUNT_TYPE, refCount, \
    void(*dispose)(type*) , \

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this is the name of the type that has T wrapped*/
#define THANDLE_WRAPPER_TYPE_NAME(T) MU_C2(T, _WRAPPER)

/*given a previous type T, THANDLE_MALLOC introduces a new name that mimics "malloc for T"*/
/*the new name is used to define the name of a static function that allocates memory*/
#define THANDLE_MALLOC(T) MU_C2(T,_MALLOC)

/*given a previous type T, THANDLE_FREE introduces a new name that mimics "free for T"*/
/*the new name is used to define the name of a static function that free memory allocated by THANDLE_MALLOC*/
#define THANDLE_FREE(T) MU_C2(T,_FREE)

/*given a previous type T, THANDLE_DEC_REF introduces a new name that mimics "dec_ref for T"*/
/*the new name is used to define the name of a static function that decrements the ref count of T and if 0, releases it*/
#define THANDLE_DEC_REF(T) MU_C2(T,_DEC_REF)

/*given a previous type T, THANDLE_INC_REF introduces a new name that mimics "inc_ref for T"*/
/*the new name is used to define the name of a static function that increments the ref count of T*/
#define THANDLE_INC_REF(T) MU_C2(T,_INC_REF)

/*given a previous type T, THANDLE_ASSIGN introduces a new name for a function that does T1=T2 (with inc/dec refs)*/
#define THANDLE_ASSIGN(T) MU_C2(T,_ASSIGN)

/*given a previous type T, THANDLE_INITIALIZE introduces a new name for a function that does T1=T2 (with inc ref, and considers T1 as uninitialized memory)*/
#define THANDLE_INITIALIZE(T) MU_C2(T,_INITIALIZE)

/*given a previous type T introduced by MU_DEFINE_STRUCT(T, T_FIELDS), this introduces:
1. the type that wraps it
2. THANDLE_MALLOC macro to create it, initialize refCount to 0, and remember the dispose function*/

#define THANDLE_TYPE_DEFINE(T) \
    MU_DEFINE_STRUCT(THANDLE_WRAPPER_TYPE_NAME(T), T, data, THANDLE_EXTRA_FIELDS(T));                                                                                   \
    static T* THANDLE_MALLOC(T)(void(*dispose)(T*))                                                                                                                     \
    {                                                                                                                                                                   \
        T* result;                                                                                                                                                      \
        /*Codes_SRS_THANDLE_02_013: [ THANDLE_MALLOC shall allocate memory. ]*/                                                                                         \
        THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = (THANDLE_WRAPPER_TYPE_NAME(T)*)malloc(sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                                        \
        if (handle_impl == NULL)                                                                                                                                        \
        {                                                                                                                                                               \
            /*Codes_SRS_THANDLE_02_015: [ If malloc fails then THANDLE_MALLOC shall fail and return NULL. ]*/                                                           \
            LogError("error in malloc(sizeof(THANDLE_WRAPPER_TYPE_NAME(" MU_TOSTRING(T) "))=%zu)",                                                                      \
                sizeof(THANDLE_WRAPPER_TYPE_NAME(T)));                                                                                                                  \
            result = NULL;                                                                                                                                              \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_THANDLE_02_014: [ THANDLE_MALLOC shall initialize the reference count to 1, store dispose and return a T* . ]*/                                 \
            handle_impl->dispose = dispose;                                                                                                                             \
            INIT_REF_VAR(handle_impl->refCount);                                                                                                                        \
            result = &(handle_impl->data);                                                                                                                              \
        }                                                                                                                                                               \
        return result;                                                                                                                                                  \
    }                                                                                                                                                                   \
    static void THANDLE_FREE(T)(T* t)                                                                                                                                   \
    {                                                                                                                                                                   \
        /*Codes_SRS_THANDLE_02_016: [ If t is NULL then THANDLE_FREE shall return. ]*/                                                                                  \
        if (t == NULL)                                                                                                                                                  \
        {                                                                                                                                                               \
            LogError("invalid arg " MU_TOSTRING(T) "* t=%p", t);                                                                                                        \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_THANDLE_02_017: [ THANDLE_FREE shall free the allocated memory by THANDLE_MALLOC. ]*/                                                           \
            THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = CONTAINING_RECORD(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                       \
            free(handle_impl);                                                                                                                                          \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \
    void THANDLE_DEC_REF(T)(THANDLE(T) t)                                                                                                                               \
    {                                                                                                                                                                   \
        /*Codes_SRS_THANDLE_02_001: [ If t is NULL then THANDLE_DEC_REF shall return. ]*/                                                                               \
        if(t == NULL)                                                                                                                                                   \
        {                                                                                                                                                               \
            LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") t=%p", t);                                                                                           \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_THANDLE_02_002: [ THANDLE_DEC_REF shall decrement the ref count of t. ]*/                                                                       \
            THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = CONTAINING_RECORD(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                       \
            if (DEC_REF_VAR(handle_impl->refCount) == DEC_RETURN_ZERO)                                                                                                  \
            {                                                                                                                                                           \
                /*Codes_SRS_THANDLE_02_003: [ If the ref count of t reaches 0 then THANDLE_DEC_REF shall call dispose (if not NULL) and free the used memory. ]*/       \
                if(handle_impl->dispose!=NULL)                                                                                                                          \
                {                                                                                                                                                       \
                    handle_impl->dispose(&handle_impl->data);                                                                                                           \
                }                                                                                                                                                       \
                THANDLE_FREE(T)(&handle_impl->data);                                                                                                                    \
            }                                                                                                                                                           \
                                                                                                                                                                        \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \
    void THANDLE_INC_REF(T)(THANDLE(T) t)                                                                                                                               \
    {                                                                                                                                                                   \
        /*Codes_SRS_THANDLE_02_004: [ If t is NULL then THANDLE_INC_REF shall return. ]*/                                                                               \
        if(t == NULL)                                                                                                                                                   \
        {                                                                                                                                                               \
            LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") t=%p", t);                                                                                           \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_THANDLE_02_005: [ THANDLE_INC_REF shall increment the reference count of t. ]*/                                                                 \
            THANDLE_WRAPPER_TYPE_NAME(T)* handle_impl = CONTAINING_RECORD(t, THANDLE_WRAPPER_TYPE_NAME(T), data);                                                       \
            INC_REF_VAR(handle_impl->refCount);                                                                                                                         \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \
    void THANDLE_ASSIGN(T)(THANDLE(T) * t1, THANDLE(T) t2 )                                                                                                             \
    {                                                                                                                                                                   \
        /*Codes_SRS_THANDLE_02_006: [ If t1 is NULL then THANDLE_ASSIGN shall return. ]*/                                                                               \
        if(t1 == NULL)                                                                                                                                                  \
        {                                                                                                                                                               \
            LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") * t1=%p, THANDLE(" MU_TOSTRING(T) ") t2=%p", t1, t2 );                                               \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            if (*t1 == NULL)                                                                                                                                            \
            {                                                                                                                                                           \
                if (t2 == NULL)                                                                                                                                         \
                {                                                                                                                                                       \
                    /*Codes_SRS_THANDLE_02_007: [ If *t1 is NULL and t2 is NULL then THANDLE_ASSIGN shall return. ]*/                                                   \
                    /*so nothing to do, leave them as they are*/                                                                                                        \
                }                                                                                                                                                       \
                else                                                                                                                                                    \
                {                                                                                                                                                       \
                    /*Codes_SRS_THANDLE_02_008: [ If *t1 is NULL and t2 is not NULL then THANDLE_ASSIGN shall increment the reference count of t2 and store t2 in *t1. ]*/ \
                    THANDLE_INC_REF(T)(t2);                                                                                                                             \
                    * (T const**)t1 = t2;                                                                                                                               \
                }                                                                                                                                                       \
            }                                                                                                                                                           \
            else                                                                                                                                                        \
            {                                                                                                                                                           \
                if (t2 == NULL)                                                                                                                                         \
                {                                                                                                                                                       \
                    /*Codes_SRS_THANDLE_02_009: [ If *t1 is not NULL and t2 is NULL then THANDLE_ASSIGN shall decrement the reference count of *t1 and store NULL in *t1. ]*/ \
                    THANDLE_DEC_REF(T)(*t1);                                                                                                                            \
                    * (T const**)t1 = t2;                                                                                                                               \
                }                                                                                                                                                       \
                else                                                                                                                                                    \
                {                                                                                                                                                       \
                    /*Codes_SRS_THANDLE_02_010: [ If *t1 is not NULL and t2 is not NULL then THANDLE_ASSIGN shall increment the reference count of t2, shall decrement the reference count of *t1 and store t2 in *t1. ]*/ \
                    THANDLE_INC_REF(T)(t2);                                                                                                                             \
                    THANDLE_DEC_REF(T)(*t1);                                                                                                                            \
                    * (T const**)t1 = t2;                                                                                                                               \
                }                                                                                                                                                       \
            }                                                                                                                                                           \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \
    void THANDLE_INITIALIZE(T)(THANDLE(T) * t1, THANDLE(T) t2 )                                                                                                         \
    {                                                                                                                                                                   \
        /*Codes_SRS_THANDLE_02_011: [ If t1 is NULL then THANDLE_INITIALIZE shall return. ]*/                                                                           \
        if(t1 == NULL)                                                                                                                                                  \
        {                                                                                                                                                               \
            LogError("invalid argument THANDLE(" MU_TOSTRING(T) ") * t1=%p, THANDLE(" MU_TOSTRING(T) ") t2=%p", t1, t2 );                                               \
        }                                                                                                                                                               \
        else                                                                                                                                                            \
        {                                                                                                                                                               \
            /*Codes_SRS_THANDLE_02_012: [ THANDLE_INITIALIZE shall increment the reference count of t2 and store it in *t1. ]*/                                         \
            THANDLE_INC_REF(T)(t2);                                                                                                                                     \
            * (T const**)t1 = t2;                                                                                                                                       \
        }                                                                                                                                                               \
    }                                                                                                                                                                   \

/*macro to be used in headers*/                                                                                       \
/*introduces an incomplete type based on a MU_DEFINE_STRUCT(LL...) that has been THANDLE_TYPE_DEFINE(LL);*/           \
#define THANDLE_TYPE_DECLARE(T)                                                                                       \
    typedef struct MU_C2(T,_TAG) T;  /*sort of DECLARE_STRUCT, but it doesn't exist in macro_utils.h  */              \
    MOCKABLE_FUNCTION(, void, THANDLE_DEC_REF(T), THANDLE(T), t);                                                     \
    MOCKABLE_FUNCTION(, void, THANDLE_INC_REF(T), THANDLE(T), t);                                                     \
    MOCKABLE_FUNCTION(, void, THANDLE_ASSIGN(T), THANDLE(T) *, t1, THANDLE(T), t2 );                                  \
    MOCKABLE_FUNCTION(, void, THANDLE_INITIALIZE(T), THANDLE(T) *, t1, THANDLE(T), t2 );                              \

#endif /*THANDLE_H*/

