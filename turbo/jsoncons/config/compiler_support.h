// Copyright 2013-2023 Daniel Parker
// Copyright 2023 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef JSONCONS_COMPILER_SUPPORT_HPP
#define JSONCONS_COMPILER_SUPPORT_HPP

#include <stdexcept>
#include <string>
#include <cmath>
#include <exception>
#include <ostream>
#include "turbo/platform/port.h"

#if defined (__clang__)
#define JSONCONS_CLANG_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#endif

#if defined(ANDROID) || defined(__ANDROID__)
#if __ANDROID_API__ >= 21
#define JSONCONS_HAS_STRTOLD_L
#else
#define JSONCONS_NO_LOCALECONV
#endif
#endif

#if defined(_MSC_VER)
#define JSONCONS_HAS_MSC_STRTOD_L
#define JSONCONS_HAS_FOPEN_S
#endif

#if defined(JSONCONS_HAS_STD_FROM_CHARS) && JSONCONS_HAS_STD_FROM_CHARS
#include <charconv>
#endif

#if !defined(JSONCONS_HAS_2017)
#  if defined(__clang__)
#   if (__cplusplus >= 201703)
#     define JSONCONS_HAS_2017 1
#   endif // (__cplusplus >= 201703)
#  endif // defined(__clang__)
#  if defined(__GNUC__)
#   if (__GNUC__ >= 7)
#    if (__cplusplus >= 201703)
#     define JSONCONS_HAS_2017 1
#    endif // (__cplusplus >= 201703)
#   endif // (__GNUC__ >= 7)
#  endif // defined(__GNUC__)
#  if defined(_MSC_VER)
#   if (_MSC_VER >= 1910 && _MSVC_LANG >= 201703)
#    define JSONCONS_HAS_2017 1
#   endif // (_MSC_VER >= 1910 && MSVC_LANG >= 201703)
#  endif // defined(_MSC_VER)
#endif


#if !defined(JSONCONS_HAS_STD_STRING_VIEW)
#  if (defined JSONCONS_HAS_2017)
#    if defined(__clang__)
#      if __has_include(<string_view>)
#        define JSONCONS_HAS_STD_STRING_VIEW 1
#     endif // __has_include(<string_view>)
#   else
#      define JSONCONS_HAS_STD_STRING_VIEW 1
#   endif
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_STD_STRING_VIEW)

#if !defined(JSONCONS_HAS_STD_BYTE)
#  if (defined JSONCONS_HAS_2017)
#    define JSONCONS_HAS_STD_BYTE 1
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_STD_BYTE)

#if !defined(JSONCONS_HAS_STD_OPTIONAL)
#  if (defined JSONCONS_HAS_2017)
#    if defined(__clang__)
#      if __has_include(<optional>)
#        define JSONCONS_HAS_STD_OPTIONAL 1
#     endif // __has_include(<string_view>)
#   else
#      define JSONCONS_HAS_STD_OPTIONAL 1
#   endif
#  endif // defined(JSONCONS_HAS_2017)
#endif // !defined(JSONCONS_HAS_STD_OPTIONAL)


#if (!defined(JSONCONS_NO_EXCEPTIONS))
// Check if exceptions are disabled.
#  if defined( __cpp_exceptions) && __cpp_exceptions == 0
#   define JSONCONS_NO_EXCEPTIONS 1
#  endif
#endif

#if !defined(JSONCONS_NO_EXCEPTIONS)

#if defined(__GNUC__) && !__EXCEPTIONS
# define JSONCONS_NO_EXCEPTIONS 1
#elif defined(_MSC_VER)
#if defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS == 0
# define JSONCONS_NO_EXCEPTIONS 1
#elif !defined(_CPPUNWIND)
# define JSONCONS_NO_EXCEPTIONS 1
#endif
#endif
#endif

// allow to disable exceptions
#if !defined(JSONCONS_NO_EXCEPTIONS)
    #define JSONCONS_THROW(exception) throw exception
    #define JSONCONS_RETHROW throw
    #define JSONCONS_TRY try
    #define JSONCONS_CATCH(exception) catch(exception)
#else
    #define JSONCONS_THROW(exception) std::terminate()
    #define JSONCONS_RETHROW std::terminate()
    #define JSONCONS_TRY if (true)
    #define JSONCONS_CATCH(exception) if (false)
#endif


#endif // JSONCONS_COMPILER_SUPPORT_HPP
