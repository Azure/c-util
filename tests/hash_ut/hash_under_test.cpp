// Copyright (c) Microsoft. All rights reserved.

// include Murmur hash with C signatures so that they can be nicely hooked with umock_c
// this is due to the fact that the Murmur hash headers are not C/C++ compile friendly
extern "C"
{
#include "MurmurHash2.h" // IWYU pragma: keep
}

#include "../../src/hash.cpp"
