// Copyright (C) 2024 EA group inc.
// Author: Jeff.li lijippy@163.com
// All rights reserved.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
//
// This file is intended solely for spinlock.h.
// It provides ThreadSanitizer annotations for custom mutexes.
// See <sanitizer/tsan_interface.h> for meaning of these annotations.

#ifndef TURBO_BASE_INTERNAL_TSAN_MUTEX_INTERFACE_H_
#define TURBO_BASE_INTERNAL_TSAN_MUTEX_INTERFACE_H_

#include <turbo/base/config.h>

// TURBO_INTERNAL_HAVE_TSAN_INTERFACE
// Macro intended only for internal use.
//
// Checks whether LLVM Thread Sanitizer interfaces are available.
// First made available in LLVM 5.0 (Sep 2017).
#ifdef TURBO_INTERNAL_HAVE_TSAN_INTERFACE
#error "TURBO_INTERNAL_HAVE_TSAN_INTERFACE cannot be directly set."
#endif

#if defined(TURBO_HAVE_THREAD_SANITIZER) && defined(__has_include)
#if __has_include(<sanitizer/tsan_interface.h>)
#define TURBO_INTERNAL_HAVE_TSAN_INTERFACE 1
#endif
#endif

#ifdef TURBO_INTERNAL_HAVE_TSAN_INTERFACE
#include <sanitizer/tsan_interface.h>

#define TURBO_TSAN_MUTEX_CREATE __tsan_mutex_create
#define TURBO_TSAN_MUTEX_DESTROY __tsan_mutex_destroy
#define TURBO_TSAN_MUTEX_PRE_LOCK __tsan_mutex_pre_lock
#define TURBO_TSAN_MUTEX_POST_LOCK __tsan_mutex_post_lock
#define TURBO_TSAN_MUTEX_PRE_UNLOCK __tsan_mutex_pre_unlock
#define TURBO_TSAN_MUTEX_POST_UNLOCK __tsan_mutex_post_unlock
#define TURBO_TSAN_MUTEX_PRE_SIGNAL __tsan_mutex_pre_signal
#define TURBO_TSAN_MUTEX_POST_SIGNAL __tsan_mutex_post_signal
#define TURBO_TSAN_MUTEX_PRE_DIVERT __tsan_mutex_pre_divert
#define TURBO_TSAN_MUTEX_POST_DIVERT __tsan_mutex_post_divert

#else

#define TURBO_TSAN_MUTEX_CREATE(...)
#define TURBO_TSAN_MUTEX_DESTROY(...)
#define TURBO_TSAN_MUTEX_PRE_LOCK(...)
#define TURBO_TSAN_MUTEX_POST_LOCK(...)
#define TURBO_TSAN_MUTEX_PRE_UNLOCK(...)
#define TURBO_TSAN_MUTEX_POST_UNLOCK(...)
#define TURBO_TSAN_MUTEX_PRE_SIGNAL(...)
#define TURBO_TSAN_MUTEX_POST_SIGNAL(...)
#define TURBO_TSAN_MUTEX_PRE_DIVERT(...)
#define TURBO_TSAN_MUTEX_POST_DIVERT(...)

#endif

#endif  // TURBO_BASE_INTERNAL_TSAN_MUTEX_INTERFACE_H_
