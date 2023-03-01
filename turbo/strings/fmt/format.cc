// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "turbo/strings/fmt/format-inl.h"

FMT_BEGIN_NAMESPACE
namespace detail {

template <typename T>
int format_float(char* buf, std::size_t size, const char* format, int precision,
                 T value) {
#ifdef FMT_FUZZ
  if (precision > 100000)
    throw std::runtime_error(
        "fuzz mode - avoid large allocation inside snprintf");
#endif
  // Suppress the warning about nonliteral format string.
  int (*snprintf_ptr)(char*, size_t, const char*, ...) = FMT_SNPRINTF;
  return precision < 0 ? snprintf_ptr(buf, size, format, value)
                       : snprintf_ptr(buf, size, format, precision, value);
}
}  // namespace detail

template struct TURBO_DLL detail::basic_data<void>;

// Workaround a bug in MSVC2013 that prevents instantiation of format_float.
int (*instantiate_format_float)(double, int, detail::float_specs,
                                detail::buffer<char>&) = detail::format_float;

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template TURBO_DLL detail::locale_ref::locale_ref(const std::locale& loc);
template TURBO_DLL std::locale detail::locale_ref::get<std::locale>() const;
#endif

// Explicit instantiations for char.

template TURBO_DLL std::string detail::grouping_impl<char>(locale_ref);
template TURBO_DLL char detail::thousands_sep_impl(locale_ref);
template TURBO_DLL char detail::decimal_point_impl(locale_ref);

template TURBO_DLL void detail::buffer<char>::append(const char*, const char*);

template TURBO_DLL format_context::iterator detail::vformat_to(
    detail::buffer<char>&, string_view, basic_format_args<format_context>);

template TURBO_DLL int detail::snprintf_float(double, int, detail::float_specs,
                                            detail::buffer<char>&);
template TURBO_DLL int detail::snprintf_float(long double, int,
                                            detail::float_specs,
                                            detail::buffer<char>&);
template TURBO_DLL int detail::format_float(double, int, detail::float_specs,
                                          detail::buffer<char>&);
template TURBO_DLL int detail::format_float(long double, int, detail::float_specs,
                                          detail::buffer<char>&);

// Explicit instantiations for wchar_t.

template TURBO_DLL std::string detail::grouping_impl<wchar_t>(locale_ref);
template TURBO_DLL wchar_t detail::thousands_sep_impl(locale_ref);
template TURBO_DLL wchar_t detail::decimal_point_impl(locale_ref);

template TURBO_DLL void detail::buffer<wchar_t>::append(const wchar_t*,
                                                      const wchar_t*);
FMT_END_NAMESPACE
