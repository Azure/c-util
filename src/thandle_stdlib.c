// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include "azure_c_util/thandle_stdlib.h"

/*this file exists to "not allow" thandle mockable calls to "malloc". When THANDLE calls malloc - and malloc is mocked - an unexpected actual call to malloc appears*/
/*having thandle use its own malloc mechanism will hide the unexpected call to "malloc"*/
/*for testing purposes (of thandle itself) we want to switch between the malloc from gballoc.h and the malloc of stdlib.h*/

void* THANDLE_MALLOC_FUNCTION(size_t size)
{
    return malloc(size);
}

void THANDLE_FREE_FUNCTION(void* ptr)
{
    free(ptr);
}
