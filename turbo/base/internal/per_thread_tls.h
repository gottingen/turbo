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

#ifndef TURBO_BASE_INTERNAL_PER_THREAD_TLS_H_
#define TURBO_BASE_INTERNAL_PER_THREAD_TLS_H_

// This header defines two macros:
//
// If the platform supports thread-local storage:
//
// * TURBO_PER_THREAD_TLS_KEYWORD is the C keyword needed to declare a
//   thread-local variable
// * TURBO_PER_THREAD_TLS is 1
//
// Otherwise:
//
// * TURBO_PER_THREAD_TLS_KEYWORD is empty
// * TURBO_PER_THREAD_TLS is 0
//
// Microsoft C supports thread-local storage.
// GCC supports it if the appropriate version of glibc is available,
// which the programmer can indicate by defining TURBO_HAVE_TLS

#include <turbo/base/port.h>  // For TURBO_HAVE_TLS

#if defined(TURBO_PER_THREAD_TLS)
#error TURBO_PER_THREAD_TLS cannot be directly set
#elif defined(TURBO_PER_THREAD_TLS_KEYWORD)
#error TURBO_PER_THREAD_TLS_KEYWORD cannot be directly set
#elif defined(TURBO_HAVE_TLS)
#define TURBO_PER_THREAD_TLS_KEYWORD __thread
#define TURBO_PER_THREAD_TLS 1
#elif defined(_MSC_VER)
#define TURBO_PER_THREAD_TLS_KEYWORD __declspec(thread)
#define TURBO_PER_THREAD_TLS 1
#else
#define TURBO_PER_THREAD_TLS_KEYWORD
#define TURBO_PER_THREAD_TLS 0
#endif

#endif  // TURBO_BASE_INTERNAL_PER_THREAD_TLS_H_
