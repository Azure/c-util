// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef TLOG_H
#define TLOG_H

#include "macro_utils/macro_utils.h"

#include "c_pal/thandle_ll.h"
#include "c_util/tlog_ll.h"

#include "umock_c/umock_c_prod.h"

/*TLOG is-a THANDLE.*/
/*given a type "T" TLOG(T) expands to the name of the type. */
#define TLOG(T) TLOG_LL(T)

#define TLOG_CREATE_DECLARE(T) TLOG_LL_CREATE_DECLARE(T, T)
#define TLOG_PUSH_DECLARE(T) TLOG_LL_PUSH_DECLARE(T, T)
#define TLOG_POP_DECLARE(T) TLOG_LL_POP_DECLARE(T, T)

#define TLOG_CREATE_DEFINE(T) TLOG_LL_CREATE_DEFINE(T, T)
#define TLOG_PUSH_DEFINE(T) TLOG_LL_PUSH_DEFINE(T, T)
#define TLOG_POP_DEFINE(T) TLOG_LL_POP_DEFINE(T, T)

#define TLOG_FREE_DEFINE(T) TLOG_LL_FREE_DEFINE(T, T)

#define TLOG_CREATE(C) TLOG_LL_CREATE(C)

#define TLOG_PUSH(C) TLOG_LL_PUSH(C)
#define TLOG_POP(C) TLOG_LL_POP(C)

#define TLOG_INITIALIZE(T) TLOG_LL_INITIALIZE(T)
#define TLOG_ASSIGN(T) TLOG_LL_ASSIGN(T)
#define TLOG_MOVE(T) TLOG_LL_MOVE(T)
#define TLOG_INITIALIZE_MOVE(T) TLOG_LL_INITIALIZE_MOVE(T)

#define TLOG_TARGET_HANDLE(T) TLOG_LL_TARGET_HANDLE(T)
#define TLOG_TARGET_FUNC(T) TLOG_LL_TARGET_FUNC(T)

/*macro to be used in headers*/                                                                                     \
#define TLOG_TYPE_DECLARE(T, ...)                                                                                 \
    /*hint: have TLOG_DEFINE_TYPE(T) before TLOG_TYPE_DECLARE                               */                  \
    /*hint: have THANDLE_TYPE_DECLARE(TLOG_TYPEDEF_NAME(T)) before TLOG_TYPE_DECLARE               */           \
    TLOG_CREATE_DECLARE(T)                                                                                        \
    TLOG_PUSH_DECLARE(T)                                                                                          \
    TLOG_POP_DECLARE(T)                                                                                           \

#define TLOG_TYPE_DEFINE(T, ...)                                                                                  \
    /*hint: have THANDLE_TYPE_DEFINE(TLOG_TYPEDEF_NAME(T)) before  TLOG_TYPE_DEFINE                */           \
    TLOG_FREE_DEFINE(T)                                                                                           \
    TLOG_CREATE_DEFINE(T)                                                                                         \
    TLOG_PUSH_DEFINE(T)                                                                                           \
    TLOG_POP_DEFINE(T)                                                                                            \

#endif // TLOG_H
