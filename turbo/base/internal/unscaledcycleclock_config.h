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

#ifndef TURBO_BASE_INTERNAL_UNSCALEDCYCLECLOCK_CONFIG_H_
#define TURBO_BASE_INTERNAL_UNSCALEDCYCLECLOCK_CONFIG_H_

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// The following platforms have an implementation of a hardware counter.
#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__) || \
    defined(__powerpc__) || defined(__ppc__) || defined(_M_IX86) ||     \
    (defined(_M_X64) && !defined(_M_ARM64EC))
#define TURBO_HAVE_UNSCALED_CYCLECLOCK_IMPLEMENTATION 1
#else
#define TURBO_HAVE_UNSCALED_CYCLECLOCK_IMPLEMENTATION 0
#endif

// The following platforms often disable access to the hardware
// counter (through a sandbox) even if the underlying hardware has a
// usable counter. The CycleTimer interface also requires a *scaled*
// CycleClock that runs at atleast 1 MHz. We've found some Android
// ARM64 devices where this is not the case, so we disable it by
// default on Android ARM64.
#if defined(__native_client__) || (defined(__APPLE__)) || \
    (defined(__ANDROID__) && defined(__aarch64__))
#define TURBO_USE_UNSCALED_CYCLECLOCK_DEFAULT 0
#else
#define TURBO_USE_UNSCALED_CYCLECLOCK_DEFAULT 1
#endif

// UnscaledCycleClock is an optional internal feature.
// Use "#if TURBO_USE_UNSCALED_CYCLECLOCK" to test for its presence.
// Can be overridden at compile-time via -DTURBO_USE_UNSCALED_CYCLECLOCK=0|1
#if !defined(TURBO_USE_UNSCALED_CYCLECLOCK)
#define TURBO_USE_UNSCALED_CYCLECLOCK               \
  (TURBO_HAVE_UNSCALED_CYCLECLOCK_IMPLEMENTATION && \
   TURBO_USE_UNSCALED_CYCLECLOCK_DEFAULT)
#endif

#if TURBO_USE_UNSCALED_CYCLECLOCK
// This macro can be used to test if UnscaledCycleClock::Frequency()
// is NominalCPUFrequency() on a particular platform.
#if (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || \
     defined(_M_X64))
#define TURBO_INTERNAL_UNSCALED_CYCLECLOCK_FREQUENCY_IS_CPU_FREQUENCY
#endif
#endif

#endif  // TURBO_BASE_INTERNAL_UNSCALEDCYCLECLOCK_CONFIG_H_
