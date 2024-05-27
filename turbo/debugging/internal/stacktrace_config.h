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

#pragma once

#include <turbo/base/config.h>

#if defined(TURBO_STACKTRACE_INL_HEADER)
#error TURBO_STACKTRACE_INL_HEADER cannot be directly set

#elif defined(_WIN32)
#define TURBO_STACKTRACE_INL_HEADER \
    "turbo/debugging/internal/stacktrace_win32-inl.inc"

#elif defined(__APPLE__)
#ifdef TURBO_HAVE_THREAD_LOCAL
// Thread local support required for UnwindImpl.
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_generic-inl.inc"
#endif  // defined(TURBO_HAVE_THREAD_LOCAL)

// Emscripten stacktraces rely on JS. Do not use them in standalone mode.
#elif defined(__EMSCRIPTEN__) && !defined(STANDALONE_WASM)
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_emscripten-inl.inc"

#elif defined(__linux__) && !defined(__ANDROID__)

#if defined(NO_FRAME_POINTER) && \
    (defined(__i386__) || defined(__x86_64__) || defined(__aarch64__))
// Note: The libunwind-based implementation is not available to open-source
// users.
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_libunwind-inl.inc"
#define STACKTRACE_USES_LIBUNWIND 1
#elif defined(NO_FRAME_POINTER) && defined(__has_include)
#if __has_include(<execinfo.h>)
// Note: When using glibc this may require -funwind-tables to function properly.
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_generic-inl.inc"
#endif  // __has_include(<execinfo.h>)
#elif defined(__i386__) || defined(__x86_64__)
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_x86-inl.inc"
#elif defined(__ppc__) || defined(__PPC__)
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_powerpc-inl.inc"
#elif defined(__aarch64__)
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_aarch64-inl.inc"
#elif defined(__riscv)
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_riscv-inl.inc"
#elif defined(__has_include)
#if __has_include(<execinfo.h>)
// Note: When using glibc this may require -funwind-tables to function properly.
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_generic-inl.inc"
#endif  // __has_include(<execinfo.h>)
#endif  // defined(__has_include)

#endif  // defined(__linux__) && !defined(__ANDROID__)

// Fallback to the empty implementation.
#if !defined(TURBO_STACKTRACE_INL_HEADER)
#define TURBO_STACKTRACE_INL_HEADER \
  "turbo/debugging/internal/stacktrace_unimplemented-inl.inc"
#endif
