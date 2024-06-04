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


#if defined(_WIN32) || defined(__CYGWIN__)
// Windows

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#else
#pragma GCC diagnostic ignored "-Wattributes"
#endif

#if defined(__cplusplus) && defined(__GNUC__) && !defined(__clang__)
// Use C++ attribute syntax where possible to avoid GCC parser bug
// (https://stackoverflow.com/questions/57993818/gcc-how-to-combine-attribute-dllexport-and-nodiscard-in-a-struct-de)
#define TURBO_DLL_EXPORT [[gnu::dllexport]]
#define TURBO_DLL_IMPORT [[gnu::dllimport]]
#else
#define TURBO_DLL_EXPORT __declspec(dllexport)
#define TURBO_DLL_IMPORT __declspec(dllimport)
#endif

// _declspec(dllexport) even when the #included by a non-arrow source
#define TURBO_FORCE_EXPORT TURBO_DLL_EXPORT

#ifdef TURBO_STATIC
#define TURBO_EXPORT
#define TURBO_FRIEND_EXPORT
#define TURBO_TEMPLATE_EXPORT
#elif defined(TURBO_EXPORTING)
#define TURBO_EXPORT TURBO_DLL_EXPORT
// For some reason [[gnu::dllexport]] doesn't work well with friend declarations
#define TURBO_FRIEND_EXPORT __declspec(dllexport)
#define TURBO_TEMPLATE_EXPORT TURBO_DLL_EXPORT
#else
#define TURBO_EXPORT TURBO_DLL_IMPORT
#define TURBO_FRIEND_EXPORT __declspec(dllimport)
#define TURBO_TEMPLATE_EXPORT TURBO_DLL_IMPORT
#endif

#define TURBO_NO_EXPORT

#else

// Non-Windows

#if defined(__cplusplus) && (defined(__GNUC__) || defined(__clang__))
#ifndef TURBO_EXPORT
#define TURBO_EXPORT [[gnu::visibility("default")]]
#endif
#ifndef TURBO_NO_EXPORT
#define TURBO_NO_EXPORT [[gnu::visibility("hidden")]]
#endif
#else
// Not C++, or not gcc/clang
#ifndef TURBO_EXPORT
#define TURBO_EXPORT
#endif
#ifndef TURBO_NO_EXPORT
#define TURBO_NO_EXPORT
#endif
#endif

#define TURBO_FRIEND_EXPORT
#define TURBO_TEMPLATE_EXPORT

// [[gnu::visibility("default")]] even when #included by a non-arrow source
#define TURBO_FORCE_EXPORT [[gnu::visibility("default")]]

#endif  // Non-Windows
// TURBO_DLL
//
// When building Turbo as a DLL, this macro expands to `__declspec(dllexport)`
// so we can annotate symbols appropriately as being exported. When used in
// headers consuming a DLL, this macro expands to `__declspec(dllimport)` so
// that consumers know the symbol is defined inside the DLL. In all other cases,
// the macro expands to nothing.
#if defined(_MSC_VER)
#if defined(TURBO_BUILD_DLL)
#define TURBO_DLL TURBO_DLL_EXPORT
#elif defined(TURBO_CONSUME_DLL)
#define TURBO_DLL TURBO_DLL_IMPORT
#else
#define TURBO_DLL
#endif
#else
#define TURBO_DLL
#endif  // defined(_MSC_VER)