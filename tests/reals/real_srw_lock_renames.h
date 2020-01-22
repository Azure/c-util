// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define srw_lock_create real_srw_lock_create
#define srw_lock_destroy real_srw_lock_destroy
#define srw_lock_acquire_exclusive real_srw_lock_acquire_exclusive
#define srw_lock_release_exclusive real_srw_lock_release_exclusive
#define srw_lock_acquire_shared real_srw_lock_acquire_shared
#define srw_lock_release_shared real_srw_lock_release_shared
