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

#ifndef TURBO_SYNCHRONIZATION_INTERNAL_WAITER_H_
#define TURBO_SYNCHRONIZATION_INTERNAL_WAITER_H_

#include <turbo/base/config.h>
#include <turbo/synchronization/internal/futex_waiter.h>
#include <turbo/synchronization/internal/pthread_waiter.h>
#include <turbo/synchronization/internal/sem_waiter.h>
#include <turbo/synchronization/internal/stdcpp_waiter.h>
#include <turbo/synchronization/internal/win32_waiter.h>

// May be chosen at compile time via -DTURBO_FORCE_WAITER_MODE=<index>
#define TURBO_WAITER_MODE_FUTEX 0
#define TURBO_WAITER_MODE_SEM 1
#define TURBO_WAITER_MODE_CONDVAR 2
#define TURBO_WAITER_MODE_WIN32 3
#define TURBO_WAITER_MODE_STDCPP 4

#if defined(TURBO_FORCE_WAITER_MODE)
#define TURBO_WAITER_MODE TURBO_FORCE_WAITER_MODE
#elif defined(TURBO_INTERNAL_HAVE_WIN32_WAITER)
#define TURBO_WAITER_MODE TURBO_WAITER_MODE_WIN32
#elif defined(TURBO_INTERNAL_HAVE_FUTEX_WAITER)
#define TURBO_WAITER_MODE TURBO_WAITER_MODE_FUTEX
#elif defined(TURBO_INTERNAL_HAVE_SEM_WAITER)
#define TURBO_WAITER_MODE TURBO_WAITER_MODE_SEM
#elif defined(TURBO_INTERNAL_HAVE_PTHREAD_WAITER)
#define TURBO_WAITER_MODE TURBO_WAITER_MODE_CONDVAR
#elif defined(TURBO_INTERNAL_HAVE_STDCPP_WAITER)
#define TURBO_WAITER_MODE TURBO_WAITER_MODE_STDCPP
#else
#error TURBO_WAITER_MODE is undefined
#endif

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace synchronization_internal {

#if TURBO_WAITER_MODE == TURBO_WAITER_MODE_FUTEX
using Waiter = FutexWaiter;
#elif TURBO_WAITER_MODE == TURBO_WAITER_MODE_SEM
using Waiter = SemWaiter;
#elif TURBO_WAITER_MODE == TURBO_WAITER_MODE_CONDVAR
using Waiter = PthreadWaiter;
#elif TURBO_WAITER_MODE == TURBO_WAITER_MODE_WIN32
using Waiter = Win32Waiter;
#elif TURBO_WAITER_MODE == TURBO_WAITER_MODE_STDCPP
using Waiter = StdcppWaiter;
#endif

}  // namespace synchronization_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_SYNCHRONIZATION_INTERNAL_WAITER_H_
