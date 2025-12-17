// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONSTBUFFER_FORMAT_H
#define CONSTBUFFER_FORMAT_H

#include <stdint.h>

#define CONSTBUFFER_VERSION_OFFSET 0
/*Codes_SRS_CONSTBUFFER_88_001: [ `CONSTBUFFER_VERSION_SIZE` shall be `sizeof(uint8_t)` (1 byte). ]*/
#define CONSTBUFFER_VERSION_SIZE (sizeof(uint8_t))

#define CONSTBUFFER_SIZE_OFFSET (CONSTBUFFER_VERSION_OFFSET + CONSTBUFFER_VERSION_SIZE)
/*Codes_SRS_CONSTBUFFER_88_002: [ `CONSTBUFFER_SIZE_SIZE` shall be `sizeof(uint32_t)` (4 bytes). ]*/
#define CONSTBUFFER_SIZE_SIZE (sizeof(uint32_t))

#define CONSTBUFFER_CONTENT_OFFSET (CONSTBUFFER_SIZE_OFFSET + CONSTBUFFER_SIZE_SIZE)

#define CONSTBUFFER_MIN_SERIALIZATION_SIZE CONSTBUFFER_CONTENT_OFFSET

#endif  /* CONSTBUFFER_FORMAT_H */
