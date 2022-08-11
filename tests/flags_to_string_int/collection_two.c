// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "c_util/flags_to_string.h"

#include "collection_one.h"

#define FLAG_ONE 1
#define FLAG_TWO 2

#define COLLECTION_ALFA_FLAGS      \
    FLAG_ONE, "FLAG_ONE" ,  \
    FLAG_TWO, "FLAG_TWO"


#define COLLECTION_BETA_FLAGS         \
    (1<<0),         "bit_00",         \
    (1<<1),         "bit_01",         \
    (1<<2),         "bit_02",         \
    (1<<3),         "bit_03",         \
    (1<<4),         "bit_04",         \
    (1<<5),         "bit_05",         \
    (1<<6),         "bit_06",         \
    (1<<7),         "bit_07",         \
    (1<<8),         "bit_08",         \
    (1<<9),         "bit_09",         \
    (1<<10),        "bit_10",         \
    (1<<11),        "bit_11",         \
    (1<<12),        "bit_12",         \
    (1<<13),        "bit_13",         \
    (1<<14),        "bit_14",         \
    (1<<15),        "bit_15",         \
    (1<<16),        "bit_16",         \
    (1<<17),        "bit_17",         \
    (1<<18),        "bit_18",         \
    (1<<19),        "bit_19",         \
    (1<<20),        "bit_20",         \
    (1<<21),        "bit_21",         \
    (1<<22),        "bit_22",         \
    (1<<23),        "bit_23",         \
    (1<<24),        "bit_24",         \
    (1<<25),        "bit_25",         \
    (1<<26),        "bit_26",         \
    (1<<27),        "bit_27",         \
    (1<<28),        "bit_28",         \
    (1<<29),        "bit_29",         \
    (1<<30),        "bit_30",         \
    (1<<31),        "bit_31"



/*file only wants to see that 2 flag stringification can coexist in the same file (one of them defines every bit of the word)*/
FLAGS_TO_STRING_DEFINE_FUNCTION(COLLECTION_ALFA, COLLECTION_ALFA_FLAGS);
FLAGS_TO_STRING_DEFINE_FUNCTION(COLLECTION_BETA, COLLECTION_BETA_FLAGS);
