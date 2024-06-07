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

#include <string>

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

// --------------------------------------------------------------------
// Program name

namespace turbo::flags_internal {

    // Returns program invocation name or "UNKNOWN" if `SetProgramInvocationName()`
    // is never called. At the moment this is always set to argv[0] as part of
    // library initialization.
    std::string ProgramInvocationName();

    // Returns base name for program invocation name. For example, if
    //   ProgramInvocationName() == "a/b/mybinary"
    // then
    //   ShortProgramInvocationName() == "mybinary"
    std::string ShortProgramInvocationName();

    // Sets program invocation name to a new value. Should only be called once
    // during program initialization, before any threads are spawned.
    void SetProgramInvocationName(std::string_view prog_name_str);

}  // namespace turbo::flags_internal
