// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define CreateEventA mocked_CreateEventA
#define CloseHandle mocked_CloseHandle
#define SetEvent mocked_SetEvent
#define WaitForMultipleObjects mocked_WaitForMultipleObjects

#include "../../src/worker_thread.c"
