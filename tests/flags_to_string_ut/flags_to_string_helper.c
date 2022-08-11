// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include "c_util/flags_to_string.h"
#include "flags_to_string_helper.h"

#define ALL_FLAGS_AND_STRINGS \
0x00000001,     "CRYPT_OID_INHIBIT_SIGNATURE_FORMAT_FLAG",               \
0x00000004,     "CRYPT_OID_NO_NULL_ALGORITHM_PARA_FLAG",                 \
0x40000000,     "CRYPT_OID_PUBKEY_ENCRYPT_ONLY_FLAG",                    \
0x80000000,     "CRYPT_OID_PUBKEY_SIGN_ONLY_FLAG",                       \
0x00000002,     "CRYPT_OID_USE_PUBKEY_PARA_FOR_PKCS7_FLAG"               \

FLAGS_TO_STRING_DEFINE_FUNCTION(MYCOLLECTION, ALL_FLAGS_AND_STRINGS)
