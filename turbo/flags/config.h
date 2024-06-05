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

// Determine if we should strip string literals from the Flag objects.
// By default we strip string literals on mobile platforms.
#if !defined(TURBO_FLAGS_STRIP_NAMES)

#if defined(__ANDROID__)
#define TURBO_FLAGS_STRIP_NAMES 1

#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define TURBO_FLAGS_STRIP_NAMES 1
#elif defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED
#define TURBO_FLAGS_STRIP_NAMES 1
#endif  // TARGET_OS_*
#endif

#endif  // !defined(TURBO_FLAGS_STRIP_NAMES)

#if !defined(TURBO_FLAGS_STRIP_NAMES)
// If TURBO_FLAGS_STRIP_NAMES wasn't set on the command line or above,
// the default is not to strip.
#define TURBO_FLAGS_STRIP_NAMES 0
#endif

#if !defined(TURBO_FLAGS_STRIP_HELP)
// By default, if we strip names, we also strip help.
#define TURBO_FLAGS_STRIP_HELP TURBO_FLAGS_STRIP_NAMES
#endif

// These macros represent the "source of truth" for the list of supported
// built-in types.
#define TURBO_FLAGS_INTERNAL_BUILTIN_TYPES(A) \
  A(bool, bool)                              \
  A(short, short)                            \
  A(unsigned short, unsigned_short)          \
  A(int, int)                                \
  A(unsigned int, unsigned_int)              \
  A(long, long)                              \
  A(unsigned long, unsigned_long)            \
  A(long long, long_long)                    \
  A(unsigned long long, unsigned_long_long)  \
  A(double, double)                          \
  A(float, float)

#define TURBO_FLAGS_INTERNAL_SUPPORTED_TYPES(A) \
  TURBO_FLAGS_INTERNAL_BUILTIN_TYPES(A)         \
  A(std::string, std_string)                   \
  A(std::vector<std::string>, std_vector_of_string)
