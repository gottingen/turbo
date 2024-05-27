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
// -----------------------------------------------------------------------------
// File: policy_checks.h
// -----------------------------------------------------------------------------
//
// This header enforces a minimum set of policies at build time, such as the
// supported compiler and library versions. Unsupported configurations are
// reported with `#error`. This enforcement is best effort, so successfully
// compiling this header does not guarantee a supported configuration.

#ifndef TURBO_BASE_POLICY_CHECKS_H_
#define TURBO_BASE_POLICY_CHECKS_H_

// Included for the __GLIBC_PREREQ macro used below.
#include <limits.h>

// Included for the _STLPORT_VERSION macro used below.
#if defined(__cplusplus)
#include <cstddef>
#endif

// -----------------------------------------------------------------------------
// Operating System Check
// -----------------------------------------------------------------------------

#if defined(__CYGWIN__)
#error "Cygwin is not supported."
#endif

// -----------------------------------------------------------------------------
// Toolchain Check
// -----------------------------------------------------------------------------

// We support Visual Studio 2019 (MSVC++ 16.0) and later.
// This minimum will go up.
#if defined(_MSC_VER) && _MSC_VER < 1920 && !defined(__clang__)
#error "This package requires Visual Studio 2019 (MSVC++ 16.0) or higher."
#endif

// We support GCC 7 and later.
// This minimum will go up.
#if defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ < 7
#error "This package requires GCC 7 or higher."
#endif
#endif

// We support Apple Xcode clang 4.2.1 (version 421.11.65) and later.
// This corresponds to Apple Xcode version 4.5.
// This minimum will go up.
#if defined(__apple_build_version__) && __apple_build_version__ < 4211165
#error "This package requires __apple_build_version__ of 4211165 or higher."
#endif

// -----------------------------------------------------------------------------
// C++ Version Check
// -----------------------------------------------------------------------------

// Enforce C++14 as the minimum.
#if defined(_MSVC_LANG)
#if _MSVC_LANG < 201402L
#error "C++ versions less than C++14 are not supported."
#endif  // _MSVC_LANG < 201402L
#elif defined(__cplusplus)
#if __cplusplus < 201402L
#error "C++ versions less than C++14 are not supported."
#endif  // __cplusplus < 201402L
#endif

// -----------------------------------------------------------------------------
// Standard Library Check
// -----------------------------------------------------------------------------

#if defined(_STLPORT_VERSION)
#error "STLPort is not supported."
#endif

// -----------------------------------------------------------------------------
// `char` Size Check
// -----------------------------------------------------------------------------

// Turbo currently assumes CHAR_BIT == 8. If you would like to use Turbo on a
// platform where this is not the case, please provide us with the details about
// your platform so we can consider relaxing this requirement.
#if CHAR_BIT != 8
#error "Turbo assumes CHAR_BIT == 8."
#endif

// -----------------------------------------------------------------------------
// `int` Size Check
// -----------------------------------------------------------------------------

// Turbo currently assumes that an int is 4 bytes. If you would like to use
// Turbo on a platform where this is not the case, please provide us with the
// details about your platform so we can consider relaxing this requirement.
#if INT_MAX < 2147483647
#error "Turbo assumes that int is at least 4 bytes. "
#endif

#endif  // TURBO_BASE_POLICY_CHECKS_H_
