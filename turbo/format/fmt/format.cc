// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "turbo/format/fmt/format-inl.h"

FMT_BEGIN_NAMESPACE
namespace detail {

template TURBO_DLL auto dragonbox::to_decimal(float x) noexcept
    -> dragonbox::decimal_fp<float>;
template TURBO_DLL auto dragonbox::to_decimal(double x) noexcept
    -> dragonbox::decimal_fp<double>;

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template TURBO_DLL locale_ref::locale_ref(const std::locale& loc);
template TURBO_DLL auto locale_ref::get<std::locale>() const -> std::locale;
#endif

// Explicit instantiations for char.

template TURBO_DLL auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<char>;
template TURBO_DLL auto decimal_point_impl(locale_ref) -> char;

template TURBO_DLL void buffer<char>::append(const char*, const char*);

template TURBO_DLL void vformat_to(buffer<char>&, string_view,
                                 typename vformat_args<>::type, locale_ref);

// Explicit instantiations for wchar_t.

template TURBO_DLL auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<wchar_t>;
template TURBO_DLL auto decimal_point_impl(locale_ref) -> wchar_t;

template TURBO_DLL void buffer<wchar_t>::append(const wchar_t*, const wchar_t*);

}  // namespace detail
FMT_END_NAMESPACE
