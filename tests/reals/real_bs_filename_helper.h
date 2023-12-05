// Copyright (c) Microsoft. All rights reserved.

#ifndef REAL_BS_FILENAME_HELPER_H
#define REAL_BS_FILENAME_HELPER_H


#include "macro_utils/macro_utils.h"

#define R2(X) REGISTER_GLOBAL_MOCK_HOOK(X, real_##X);

#define REGISTER_BS_FILENAME_HELPER_GLOBAL_MOCK_HOOK() \
    MU_FOR_EACH_1(R2, \
        bs_filename_append_suffix \
    )

#include "c_util/bs_filename_helper.h"



    char* real_bs_filename_append_suffix(const char* filename, const char* suffix);



#endif //REAL_BS_FILENAME_HELPER_H
