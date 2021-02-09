// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

typedef struct RC_STRING_TAG real_RC_STRING;

#define RC_STRING real_RC_STRING

#define rc_string_create                    real_rc_string_create
#define rc_string_create_with_move_memory   real_rc_string_create_with_move_memory
#define rc_string_create_with_custom_free   real_rc_string_create_with_custom_free
#define rc_string_recreate                  real_rc_string_recreate
