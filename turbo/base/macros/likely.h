//
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
// Created by jeff on 24-6-2.
//

#pragma once

#include <turbo/base/config.h>

// TURBO_LIKELY, TURBO_UNLIKELY
//
// Enables the compiler to prioritize compilation using static analysis for
// likely paths within a boolean branch.
//
// Example:
//
//   if (TURBO_LIKELY(expression)) {
//     return result;                        // Faster if more likely
//   } else {
//     return 0;
//   }
//
// Compilers can use the information that a certain branch is not likely to be
// taken (for instance, a CHECK failure) to optimize for the common case in
// the absence of better information (ie. compiling gcc with `-fprofile-arcs`).
//
// Recommendation: Modern CPUs dynamically predict branch execution paths,
// typically with accuracy greater than 97%. As a result, annotating every
// branch in a codebase is likely counterproductive; however, annotating
// specific branches that are both hot and consistently mispredicted is likely
// to yield performance improvements.
#if TURBO_HAVE_BUILTIN(__builtin_expect) || \
    (defined(__GNUC__) && !defined(__clang__))
#define TURBO_UNLIKELY(x) (__builtin_expect(false || (x), false))
#define TURBO_LIKELY(x) (__builtin_expect(false || (x), true))
#else
#define TURBO_UNLIKELY(x) (x)
#define TURBO_LIKELY(x) (x)
#endif
