// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_THANDLE_H
#define REAL_THANDLE_H

#ifdef THANDLE_H
#error Must include real_thandle.h before thandle.h
#endif

#define THANDLE_INTERLOCKED_EXCHANGE real_interlocked_exchange
#define THANDLE_INTERLOCKED_INCREMENT real_interlocked_increment
#define THANDLE_INTERLOCKED_DECREMENT real_interlocked_decrement

#include "c_util/thandle.h"

#endif //REAL_THANDLE_H
