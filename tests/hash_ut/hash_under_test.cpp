// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// include Murmur hash with C signatures so that they can be nicely hooked with umock_c
// this is due to the fact that the Murmur hash headers are not C/C++ compile friendly
extern "C"
{
#include "MurmurHash2.h" // IWYU pragma: keep
}

#include "../../src/hash.cpp"
