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

// Deprecated symbols markup
#if (defined(__cplusplus) && __cplusplus >= 201402L)
#define JSONCONS_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#endif

#if !defined(JSONCONS_DEPRECATED_MSG) && defined(__GNUC__) && defined(__has_extension)
#if __has_extension(attribute_deprecated_with_message)
#define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#endif
#endif

#if !defined(JSONCONS_DEPRECATED_MSG) && defined(_MSC_VER)
#if (_MSC_VER) >= 1920
#define JSONCONS_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#else
#define JSONCONS_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#endif
#endif

// Following boost/atomic/detail/config.hpp
#if !defined(JSONCONS_DEPRECATED_MSG) && (\
    (defined(__GNUC__) && ((__GNUC__ + 0) * 100 + (__GNUC_MINOR__ + 0)) >= 405) ||\
    (defined(__SUNPRO_CC) && (__SUNPRO_CC + 0) >= 0x5130))
    #define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#endif

#if !defined(JSONCONS_DEPRECATED_MSG) && defined(__clang__) && defined(__has_extension)
    #if __has_extension(attribute_deprecated_with_message)
        #define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
    #else
        #define JSONCONS_DEPRECATED_MSG(msg) __attribute__((deprecated))
    #endif
#endif

#if !defined(JSONCONS_DEPRECATED_MSG)
#define JSONCONS_DEPRECATED_MSG(msg)
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

#ifndef JSONCONS_HAS_CP14
   #if defined(_MSVC_LANG)
       #if _MSVC_LANG >= 201402L
           #define JSONCONS_HAS_CP14
       #endif
   #elif __cplusplus >= 201402L
        #define JSONCONS_HAS_CP14
   #endif
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

// Follows boost

// gcc and clang
#if (defined(__clang__) || defined(__GNUC__)) && defined(__cplusplus)
#if defined(__SIZEOF_INT128__) && !defined(_MSC_VER)
#  define JSONCONS_HAS_INT128
#endif

#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)
#if defined(__clang_major__) && (__clang_major__ >= 4) && defined(__has_include)
#if __has_include(<quadmath.h>)
#  define JSONCONS_HAS_FLOAT128
#endif
#endif
#endif
#endif

#if defined(__GNUC__)
#if defined(_GLIBCXX_USE_FLOAT128)
# define JSONCONS_HAS_FLOAT128
#endif
#endif

#if defined(__clang__)
#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)
#if (__clang_major__ >= 4) && defined(__has_include)
#if __has_include(<quadmath.h>)
#  define JSONCONS_HAS_FLOAT128
#endif
#endif
#endif
#endif

// Follows boost config/detail/suffix.hpp
#if defined(JSONCONS_HAS_INT128) && defined(__cplusplus)
namespace turbo{
#  ifdef __GNUC__
   __extension__ typedef __int128 int128_type;
   __extension__ typedef unsigned __int128 uint128_type;
#  else
   typedef __int128 int128_type;
   typedef unsigned __int128 uint128_type;
#  endif
}
#endif
#if defined(JSONCONS_HAS_FLOAT128) && defined(__cplusplus)
namespace turbo {
#  ifdef __GNUC__
   __extension__ typedef __float128 float128_type;
#  else
   typedef __float128 float128_type;
#  endif
}
#endif
/*
namespace turbo {

    class assertion_error : public std::runtime_error
    {
    public:
        assertion_error(const std::string& s) noexcept
            : std::runtime_error(s)
        {
        }
        const char* what() const noexcept override
        {
            return std::runtime_error::what();
        }
    };

} // namespace turbo

#define JSONCONS_STR2(x)  #x
#define JSONCONS_STR(x)  JSONCONS_STR2(x)

#ifdef _DEBUG
#define TURBO_ASSERT(x) if (!(x)) { \
    JSONCONS_THROW(turbo::assertion_error("assertion '" #x "' failed at " __FILE__ ":" \
            JSONCONS_STR(__LINE__))); }
#else
#define TURBO_ASSERT(x) if (!(x)) { \
    JSONCONS_THROW(turbo::assertion_error("assertion '" #x "' failed at  <> :" \
            JSONCONS_STR( 0 ))); }
#endif // _DEBUG
*/
#endif // JSONCONS_COMPILER_SUPPORT_HPP
