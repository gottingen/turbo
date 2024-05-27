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

#include <turbo/types/bad_any_cast.h>

#ifndef TURBO_USES_STD_ANY

#include <cstdlib>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

bad_any_cast::~bad_any_cast() = default;

const char* bad_any_cast::what() const noexcept { return "Bad any cast"; }

namespace any_internal {

void ThrowBadAnyCast() {
#ifdef TURBO_HAVE_EXCEPTIONS
  throw bad_any_cast();
#else
  TURBO_RAW_LOG(FATAL, "Bad any cast");
  std::abort();
#endif
}

}  // namespace any_internal
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
extern const char kAvoidEmptyBadAnyCastLibraryWarning;
const char kAvoidEmptyBadAnyCastLibraryWarning = 0;
}  // namespace types_internal
TURBO_NAMESPACE_END
}  // namespace turbo
#endif  // __APPLE__

#endif  // TURBO_USES_STD_ANY
