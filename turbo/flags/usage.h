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

#ifndef TURBO_FLAGS_USAGE_H_
#define TURBO_FLAGS_USAGE_H_

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

// --------------------------------------------------------------------
// Usage reporting interfaces

namespace turbo {
TURBO_NAMESPACE_BEGIN

// Sets the "usage" message to be used by help reporting routines.
// For example:
//  turbo::SetProgramUsageMessage(
//      turbo::StrCat("This program does nothing.  Sample usage:\n", argv[0],
//                   " <uselessarg1> <uselessarg2>"));
// Do not include commandline flags in the usage: we do that for you!
// Note: Calling SetProgramUsageMessage twice will trigger a call to std::exit.
void SetProgramUsageMessage(turbo::string_view new_usage_message);

// Returns the usage message set by SetProgramUsageMessage().
turbo::string_view ProgramUsageMessage();

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FLAGS_USAGE_H_
