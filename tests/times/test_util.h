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

#ifndef TURBO_TIME_INTERNAL_TEST_UTIL_H_
#define TURBO_TIME_INTERNAL_TEST_UTIL_H_

#include <string>

#include <turbo/time/time.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace time_internal {

// Loads the named timezone, but dies on any failure.
turbo::TimeZone LoadTimeZone(const std::string& name);

}  // namespace time_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_TIME_INTERNAL_TEST_UTIL_H_
