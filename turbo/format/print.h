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

#ifndef TURBO_FORMAT_PRINT_H_
#define TURBO_FORMAT_PRINT_H_

#include <cstdint>
#include "turbo/format/format.h"
#include "turbo/format/fmt/printf.h"
#include "turbo/format/fmt/os.h"
#include "turbo/format/fmt/ostream.h"
#include "turbo/format/fmt/color.h"

namespace turbo {

    template<typename ...Args>
    void Print(std::string_view fmt,  Args &&... args) {
        fmt::print(stdout, fmt, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void Println(std::string_view fmt,  Args &&... args) {
        fmt::print(stdout, "{}\n", Format(fmt, std::forward<Args>(args)...));
    }

    template<typename ...Args>
    void FPrint(std::FILE *file, std::string_view fmt,  Args &&... args) {
        fmt::print(file, fmt, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void FPrintln(std::FILE *file, std::string_view fmt,  Args &&... args) {
        fmt::print(file, "{}\n", Format(fmt, std::forward<Args>(args)...));
    }

    using fmt::color;
    using fmt::bg;
    using fmt::fg;
    using fmt::text_style;

    static const text_style RedFG = fg(color::red);
    static const text_style GreenFG = fg(color::green);
    static const text_style YellowFG = fg(color::yellow);

    template<typename ...Args>
    void Print(const text_style& ts, std::string_view fmt,  Args &&... args) {
        fmt::print(stdout, ts, fmt, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void Println(const text_style& ts, std::string_view fmt,  Args &&... args) {
        fmt::print(stdout, ts, "{}\n", Format(fmt, std::forward<Args>(args)...));
    }

    template<typename ...Args>
    void Print(const color& c, std::string_view fmt,  Args &&... args) {
        fmt::print(stdout, fg(c), fmt, std::forward<Args>(args)...);
    }

    template<typename ...Args>
    void Println(const color& c, std::string_view fmt,  Args &&... args) {
        fmt::print(stdout, fg(c), "{}\n", Format(fmt, std::forward<Args>(args)...));
    }

}  // namespace turbo

#endif  // TURBO_FORMAT_PRINT_H_
