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

#pragma once

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

// --------------------------------------------------------------------
// Usage reporting interfaces

namespace turbo {

    // Sets the "usage" message to be used by help reporting routines.
    // For example:
    //  turbo::set_program_usage_message(
    //      turbo::str_cat("This program does nothing.  Sample usage:\n", argv[0],
    //                   " <uselessarg1> <uselessarg2>"));
    // Do not include commandline flags in the usage: we do that for you!
    // Note: Calling set_program_usage_message twice will trigger a call to std::exit.
    void set_program_usage_message(std::string_view new_usage_message);

    // Returns the usage message set by set_program_usage_message().
    std::string_view program_usage_message();

}  // namespace turbo

