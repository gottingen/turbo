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

#include <turbo/flags/commandlineflag.h>

#include <string>

#include <turbo/base/config.h>
#include <turbo/flags/internal/commandlineflag.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

bool CommandLineFlag::IsRetired() const { return false; }
bool CommandLineFlag::ParseFrom(turbo::string_view value, std::string* error) {
  return ParseFrom(value, flags_internal::SET_FLAGS_VALUE,
                   flags_internal::kProgrammaticChange, *error);
}

TURBO_NAMESPACE_END
}  // namespace turbo
