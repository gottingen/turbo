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

namespace turbo::flags_internal {

    // A portable interface that returns the basename of the filename passed as an
    // argument. It is similar to basename(3)
    // <https://linux.die.net/man/3/basename>.
    // For example:
    //     flags_internal::Basename("a/b/prog/file.cc")
    // returns "file.cc"
    //     flags_internal::Basename("file.cc")
    // returns "file.cc"
    inline std::string_view Basename(std::string_view filename) {
        auto last_slash_pos = filename.find_last_of("/\\");

        return last_slash_pos == std::string_view::npos
               ? filename
               : filename.substr(last_slash_pos + 1);
    }

    // A portable interface that returns the directory name of the filename
    // passed as an argument, including the trailing slash.
    // Returns the empty string if a slash is not found in the input file name.
    // For example:
    //      flags_internal::Package("a/b/prog/file.cc")
    // returns "a/b/prog/"
    //      flags_internal::Package("file.cc")
    // returns ""
    inline std::string_view Package(std::string_view filename) {
        auto last_slash_pos = filename.find_last_of("/\\");

        return last_slash_pos == std::string_view::npos
               ? std::string_view()
               : filename.substr(0, last_slash_pos + 1);
    }

}  // namespace turbo::flags_internal
