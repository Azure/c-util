// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*PLAY is a module that pretends to be a production module that will embed a TARRAY in its handle*/

#ifndef PLAY_H
#define PLAY_H


#include <stdint.h>


#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_prod.h"



    typedef struct PLAY_HANDLE_DATA_TAG* PLAY_HANDLE;

    MOCKABLE_FUNCTION(, PLAY_HANDLE, play_create, uint32_t, generation);
    MOCKABLE_FUNCTION(, void, play_destroy, PLAY_HANDLE, play);




#endif /* PLAY_H */
