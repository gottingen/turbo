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
// This file includes routines to find out characteristics
// of the machine a program is running on.  It is undoubtedly
// system-dependent.

// Functions listed here that accept a pid_t as an argument act on the
// current process if the pid_t argument is 0
// All functions here are thread-hostile due to file caching unless
// commented otherwise.

#ifndef TURBO_BASE_INTERNAL_SYSINFO_H_
#define TURBO_BASE_INTERNAL_SYSINFO_H_

#ifndef _WIN32
#include <sys/types.h>
#endif

#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/port.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

// Nominal core processor cycles per second of each processor.   This is _not_
// necessarily the frequency of the CycleClock counter (see cycleclock.h)
// Thread-safe.
double NominalCPUFrequency();

// Number of logical processors (hyperthreads) in system. Thread-safe.
int NumCPUs();

// Return the thread id of the current thread, as told by the system.
// No two currently-live threads implemented by the OS shall have the same ID.
// Thread ids of exited threads may be reused.   Multiple user-level threads
// may have the same thread ID if multiplexed on the same OS thread.
//
// On Linux, you may send a signal to the resulting ID with kill().  However,
// it is recommended for portability that you use pthread_kill() instead.
#ifdef _WIN32
// On Windows, process id and thread id are of the same type according to the
// return types of GetProcessId() and GetThreadId() are both DWORD, an unsigned
// 32-bit type.
using pid_t = uint32_t;
#endif
pid_t GetTID();

// Like GetTID(), but caches the result in thread-local storage in order
// to avoid unnecessary system calls. Note that there are some cases where
// one must call through to GetTID directly, which is why this exists as a
// separate function. For example, GetCachedTID() is not safe to call in
// an asynchronous signal-handling context nor right after a call to fork().
pid_t GetCachedTID();

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_SYSINFO_H_
