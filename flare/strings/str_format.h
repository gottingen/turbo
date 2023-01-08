
/****************************************************************
 * Copyright (c) 2022, liyinbin
 * All rights reserved.
 * Author by liyinbin (jeff.li) lijippy@163.com
 *****************************************************************/

#ifndef FLARE_STRING_STR_FORMAT_H_
#define FLARE_STRING_STR_FORMAT_H_

#include <string_view>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <optional>
#include <limits>
#include <stdarg.h>
#include "flare/strings/fmt/format.h"
#include "flare/strings/internal/ostringstream.h"

namespace flare {

    using string_output_stream = ::flare::strings_internal::string_output_stream;

    template <class... Args>
    std::string string_format(const std::string_view& fmt, Args&&... args) {
        return fmt::format(fmt, std::forward<Args>(args)...);
    }

    // Convert |format| and associated arguments to std::string
    std::string string_printf(const char *format, ...)
    __attribute__ ((format (printf, 1, 2)));

    // Write |format| and associated arguments into |output|
    // Returns 0 on success, -1 otherwise.
    int string_printf(std::string *output, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

    // Write |format| and associated arguments in form of va_list into |output|.
    // Returns 0 on success, -1 otherwise.
    int string_vprintf(std::string *output, const char *format, va_list args);

    // Append |format| and associated arguments to |output|
    // Returns 0 on success, -1 otherwise.
    int string_appendf(std::string *output, const char *format, ...)
    __attribute__ ((format (printf, 2, 3)));

    // Append |format| and associated arguments in form of va_list to |output|.
    // Returns 0 on success, -1 otherwise.
    int string_vappendf(std::string *output, const char *format, va_list args);

}  // namespace flare
#endif  // FLARE_STRING_STR_FORMAT_H_
