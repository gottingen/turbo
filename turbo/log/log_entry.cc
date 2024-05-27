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

#include <turbo/log/log_entry.h>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL
constexpr int LogEntry::kNoVerbosityLevel;
constexpr int LogEntry::kNoVerboseLevel;
#endif

// https://github.com/abseil/abseil-cpp/issues/1465
// CMake builds on Apple platforms error when libraries are empty.
// Our CMake configuration can avoid this error on header-only libraries,
// but since this library is conditionally empty, including a single
// variable is an easy workaround.
#ifdef __APPLE__
namespace log_internal {
extern const char kAvoidEmptyLogEntryLibraryWarning;
const char kAvoidEmptyLogEntryLibraryWarning = 0;
}  // namespace log_internal
#endif  // __APPLE__

TURBO_NAMESPACE_END
}  // namespace turbo
