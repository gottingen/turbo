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
// File: clock.h
// -----------------------------------------------------------------------------
//
// This header file contains utility functions for working with the system-wide
// realtime clock. For descriptions of the main time abstractions used within
// this header file, consult the time.h header file.

#pragma once

#include <cstdint>

#include <turbo/base/config.h>
#include <turbo/base/macros.h>
#include <turbo/times/time.h>

namespace turbo {

// GetCurrentTimeNanos()
//
// Returns the current time, expressed as a count of nanoseconds since the Unix
// Epoch (https://en.wikipedia.org/wiki/Unix_time). Prefer `turbo::Time::current_time()` instead
// for all but the most performance-sensitive cases (i.e. when you are calling
// this function hundreds of thousands of times per second).
int64_t GetCurrentTimeNanos();

}  // namespace turbo
