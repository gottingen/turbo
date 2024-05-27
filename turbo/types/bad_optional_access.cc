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

#include <turbo/types/bad_optional_access.h>

#ifndef TURBO_USES_STD_OPTIONAL

#include <cstdlib>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

bad_optional_access::~bad_optional_access() = default;

const char* bad_optional_access::what() const noexcept {
  return "optional has no value";
}

namespace optional_internal {

void throw_bad_optional_access() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw bad_optional_access();
#else
  TURBO_RAW_LOG(FATAL, "Bad optional access");
  abort();
#endif
}

}  // namespace optional_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#else

// https://github.com/abseil/abseil-cpp/issues/1465
// CMake builds on Apple platforms error when libraries are empty.
// Our CMake configuration can avoid this error on header-only libraries,
// but since this library is conditionally empty, including a single
// variable is an easy workaround.
#ifdef __APPLE__
namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace types_internal {
extern const char kAvoidEmptyBadOptionalAccessLibraryWarning;
const char kAvoidEmptyBadOptionalAccessLibraryWarning = 0;
}  // namespace types_internal
TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // __APPLE__

#endif  // TURBO_USES_STD_OPTIONAL
