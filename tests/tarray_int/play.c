// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <inttypes.h>

#include "c_logging/xlogging.h"

#include "c_util/thandle2.h"
#include "c_util/tarray.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"

#include "play_undo_op_types.h"
#include "play_undo_op.h"

#include "play.h"

typedef struct PLAY_HANDLE_DATA_TAG
{
    uint32_t generation;
    TARRAY(UNDO_OP) undo_op; /* **1** TARRAY needs a TARRAY_TYPE_DECLARE. TARRAY_TYPE_DECLARE contains MOCKABLE_FUNCTIONs so it needs to go into a header*/
}PLAY_HANDLE_DATA;

PLAY_HANDLE play_create(uint32_t generation)
{
    PLAY_HANDLE play = malloc(sizeof(PLAY_HANDLE_DATA));
    if (play == NULL)
    {
        LogError("failure in malloc(sizeof(PLAY_HANDLE_DATA))");
    }
    else
    {
        TARRAY(UNDO_OP) temp = TARRAY_CREATE(UNDO_OP)();
        if (temp == NULL)
        {
            LogError("failure in TARRAY_CREATE(UNDO_OP)()");
        }
        else
        {
            TARRAY_INITIALIZE_MOVE(UNDO_OP)(&play->undo_op, &temp);
            play->generation = generation;
            goto allok;
        }
        free(play);
        play = NULL;
allok:;
    }
    return play;
}

void play_destroy(PLAY_HANDLE play)
{
    if (play == NULL)
    {
        LogError("invalid arg");
    }
    else
    {
        TARRAY_ASSIGN(UNDO_OP)(&play->undo_op, NULL);
        free(play);
    }
}

