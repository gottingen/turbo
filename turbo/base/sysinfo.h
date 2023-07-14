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

#ifndef TURBO_BASE_SYSINFO_H_
#define TURBO_BASE_SYSINFO_H_

#include "turbo/platform/port.h"
#include <cstdio>

namespace turbo {

    TURBO_DLL int pid() noexcept;

    // Determine if the terminal supports colors
    // Source: https://github.com/agauniyal/rang/
    TURBO_DLL bool is_color_terminal() noexcept;

    // Determine if the terminal attached
    // Source: https://github.com/agauniyal/rang/
    TURBO_DLL bool in_terminal(std::FILE *file) noexcept;

    // Return current thread id as size_t (from thread local storage)
    TURBO_DLL size_t thread_id() noexcept;

}  // namespace turbo

#endif  // TURBO_BASE_SYSINFO_H_
