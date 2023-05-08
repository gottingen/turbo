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

#ifndef TURBO_JSONCONS_CONFIG_JSONCONS_CONFIG_H_
#define TURBO_JSONCONS_CONFIG_JSONCONS_CONFIG_H_

#include <type_traits>
#include <limits>
#include "turbo/jsoncons/config/compiler_support.h"
#include "turbo/jsoncons/config/binary_config.h"
#include "turbo/strings/string_view.h"
#include "turbo/meta/span.h"

namespace turbo {
using std::basic_string_view;
using std::string_view;
using std::wstring_view;
}

#include <optional>

#if !defined(JSONCONS_HAS_STD_ENDIAN)
#include "turbo/jsoncons/detail/endian.h"
namespace turbo {
using turbo::detail::endian;
}
#else
#include <bit>
namespace turbo
{
    using std::endian;
}
#endif


namespace turbo {
namespace binary {

    // native_to_big

    template<typename T, class OutputIt, class Endian=endian>
    typename std::enable_if<Endian::native == Endian::big,void>::type
    native_to_big(T val, OutputIt d_first)
    {
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    template<typename T, class OutputIt, class Endian=endian>
    typename std::enable_if<Endian::native == Endian::little,void>::type
    native_to_big(T val, OutputIt d_first)
    {
        T val2 = byte_swap(val);
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val2, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    // native_to_little

    template<typename T, class OutputIt, class Endian = endian>
    typename std::enable_if<Endian::native == Endian::little,void>::type
    native_to_little(T val, OutputIt d_first)
    {
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    template<typename T, class OutputIt, class Endian=endian>
    typename std::enable_if<Endian::native == Endian::big, void>::type
    native_to_little(T val, OutputIt d_first)
    {
        T val2 = byte_swap(val);
        uint8_t buf[sizeof(T)];
        std::memcpy(buf, &val2, sizeof(T));
        for (auto item : buf)
        {
            *d_first++ = item;
        }
    }

    // big_to_native

    template<class T,class Endian=endian>
    typename std::enable_if<Endian::native == Endian::big,T>::type
    big_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return val;
    }

    template<class T,class Endian=endian>
    typename std::enable_if<Endian::native == Endian::little,T>::type
    big_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return byte_swap(val);
    }

    // little_to_native

    template<class T,class Endian=endian>
    typename std::enable_if<Endian::native == Endian::little,T>::type
    little_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return val;
    }

    template<class T,class Endian=endian>
    typename std::enable_if<Endian::native == Endian::big,T>::type
    little_to_native(const uint8_t* first, std::size_t count)
    {
        if (sizeof(T) > count)
        {
            return T{};
        }
        T val;
        std::memcpy(&val,first,sizeof(T));
        return byte_swap(val);
    }

} // binary
} // jsoncons

namespace turbo {

    template<typename CharT>
    constexpr const CharT* cstring_constant_of_type(const char* c, const wchar_t* w);

    template<> inline
    constexpr const char* cstring_constant_of_type<char>(const char* c, const wchar_t*)
    {
        return c;
    }
    template<> inline
    constexpr const wchar_t* cstring_constant_of_type<wchar_t>(const char*, const wchar_t* w)
    {
        return w;
    }

    template<typename CharT>
    std::basic_string<CharT> string_constant_of_type(const char* c, const wchar_t* w);

    template<> inline
    std::string string_constant_of_type<char>(const char* c, const wchar_t*)
    {
        return std::string(c);
    }
    template<> inline
    std::wstring string_constant_of_type<wchar_t>(const char*, const wchar_t* w)
    {
        return std::wstring(w);
    }

    template<typename CharT>
    turbo::basic_string_view<CharT> string_view_constant_of_type(const char* c, const wchar_t* w);

    template<> inline
    turbo::string_view string_view_constant_of_type<char>(const char* c, const wchar_t*)
    {
        return turbo::string_view(c);
    }
    template<> inline
    turbo::wstring_view string_view_constant_of_type<wchar_t>(const char*, const wchar_t* w)
    {
        return turbo::wstring_view(w);
    }

} // jsoncons

#define JSONCONS_EXPAND(X) X    
#define JSONCONS_QUOTE(Prefix, A) JSONCONS_EXPAND(Prefix ## #A)
#define JSONCONS_WIDEN(A) JSONCONS_EXPAND(L ## A)

#define JSONCONS_CSTRING_CONSTANT(CharT, Str) cstring_constant_of_type<CharT>(Str, JSONCONS_WIDEN(Str))
#define JSONCONS_STRING_CONSTANT(CharT, Str) string_constant_of_type<CharT>(Str, JSONCONS_WIDEN(Str))
#define JSONCONS_STRING_VIEW_CONSTANT(CharT, Str) string_view_constant_of_type<CharT>(Str, JSONCONS_WIDEN(Str))

#if defined(__clang__) 
#define JSONCONS_HAS_STD_REGEX 1
#define JSONCONS_HAS_STATEFUL_ALLOCATOR 1
#elif (defined(__GNUC__) && (__GNUC__ == 4)) && (defined(__GNUC__) && __GNUC_MINOR__ < 9)
// GCC 4.8 has broken regex support: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53631
// gcc 4.8 basic_string doesn't satisfy C++11 allocator requirements
// and gcc doesn't support allocators with no default constructor
#else
#define JSONCONS_HAS_STD_REGEX 1
#define JSONCONS_HAS_STATEFUL_ALLOCATOR 1
#endif

#endif  // TURBO_JSONCONS_CONFIG_JSONCONS_CONFIG_H_


