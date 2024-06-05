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

#include <iosfwd>
#include <ostream>
#include <string>

#include <turbo/base/config.h>
#include <turbo/flags/commandlineflag.h>
#include <turbo/strings/string_view.h>

// --------------------------------------------------------------------
// Usage reporting interfaces

namespace turbo::flags_internal {

    // The format to report the help messages in.
    enum class HelpFormat {
        kHumanReadable,
    };

    // The kind of usage help requested.
    enum class HelpMode {
        kNone,
        kImportant,
        kShort,
        kFull,
        kPackage,
        kMatch,
        kVersion,
        kOnlyCheckArgs
    };

    // Streams the help message describing `flag` to `out`.
    // The default value for `flag` is included in the output.
    void FlagHelp(std::ostream &out, const CommandLineFlag &flag,
                  HelpFormat format = HelpFormat::kHumanReadable);

    // Produces the help messages for all flags matching the filter. A flag matches
    // the filter if it is defined in a file with a filename which includes
    // filter string as a substring. You can use '/' and '.' to restrict the
    // matching to a specific file names. For example:
    //   FlagsHelp(out, "/path/to/file.");
    // restricts help to only flags which resides in files named like:
    //  .../path/to/file.<ext>
    // for any extension 'ext'. If the filter is empty this function produces help
    // messages for all flags.
    void FlagsHelp(std::ostream &out, turbo::string_view filter,
                   HelpFormat format, turbo::string_view program_usage_message);

    // --------------------------------------------------------------------

    // If any of the 'usage' related command line flags (listed on the bottom of
    // this file) has been set this routine produces corresponding help message in
    // the specified output stream and returns HelpMode that was handled. Otherwise
    // it returns HelpMode::kNone.
    HelpMode HandleUsageFlags(std::ostream &out,
                              turbo::string_view program_usage_message);

    // --------------------------------------------------------------------
    // Encapsulates the logic of exiting the binary depending on handled help mode.

    void MaybeExit(HelpMode mode);

    // --------------------------------------------------------------------
    // Globals representing usage reporting flags

    // Returns substring to filter help output (--help=substr argument)
    std::string GetFlagsHelpMatchSubstr();

    // Returns the requested help mode.
    HelpMode GetFlagsHelpMode();

    // Returns the requested help format.
    HelpFormat GetFlagsHelpFormat();

    // These are corresponding setters to the attributes above.
    void SetFlagsHelpMatchSubstr(turbo::string_view);

    void SetFlagsHelpMode(HelpMode);

    void SetFlagsHelpFormat(HelpFormat);

    // Deduces usage flags from the input argument in a form --name=value or
    // --name. argument is already split into name and value before we call this
    // function.
    bool DeduceUsageFlags(turbo::string_view name, turbo::string_view value);

}  // namespace turbo::flags_internal
