// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef REAL_THANDLE_H
#define REAL_THANDLE_H

#undef THANDLE_INTERLOCKED_EXCHANGE
#define THANDLE_INTERLOCKED_EXCHANGE real_interlocked_exchange
#undef THANDLE_INTERLOCKED_INCREMENT
#define THANDLE_INTERLOCKED_INCREMENT real_interlocked_increment
#undef THANDLE_INTERLOCKED_DECREMENT
#define THANDLE_INTERLOCKED_DECREMENT real_interlocked_decrement

#include "c_util/thandle.h"

#endif //REAL_THANDLE_H
