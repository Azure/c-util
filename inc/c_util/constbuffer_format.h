// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef CONSTBUFFER_FORMAT_H
#define CONSTBUFFER_FORMAT_H

#include <stdint.h>

#define CONSTBUFFER_VERSION_OFFSET 0
#define CONSTBUFFER_VERSION_SIZE (sizeof(uint8_t))

#define CONSTBUFFER_SIZE_OFFSET (CONSTBUFFER_VERSION_OFFSET + CONSTBUFFER_VERSION_SIZE)
#define CONSTBUFFER_SIZE_SIZE (sizeof(uint32_t))

#define CONSTBUFFER_CONTENT_OFFSET (CONSTBUFFER_SIZE_OFFSET + CONSTBUFFER_SIZE_SIZE)

#endif  /* CONSTBUFFER_FORMAT_H */
