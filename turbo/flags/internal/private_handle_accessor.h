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

#pragma once

#include <memory>
#include <string>

#include <turbo/base/config.h>
#include <turbo/flags/commandlineflag.h>
#include <turbo/flags/internal/commandlineflag.h>
#include <turbo/strings/string_view.h>

namespace turbo::flags_internal {


    // This class serves as a trampoline to access private methods of
    // CommandLineFlag. This class is intended for use exclusively internally inside
    // of the Turbo Flags implementation.
    class PrivateHandleAccessor {
    public:
        // Access to CommandLineFlag::TypeId.
        static FlagFastTypeId TypeId(const CommandLineFlag &flag);

        // Access to CommandLineFlag::SaveState.
        static std::unique_ptr<FlagStateInterface> SaveState(CommandLineFlag &flag);

        // Access to CommandLineFlag::is_specified_on_commandLine.
        static bool is_specified_on_commandLine(const CommandLineFlag &flag);

        // Access to CommandLineFlag::validate_input_value.
        static bool validate_input_value(const CommandLineFlag &flag,
                                       turbo::string_view value);

        // Access to CommandLineFlag::check_default_value_parsing_roundtrip.
        static void check_default_value_parsing_roundtrip(const CommandLineFlag &flag);

        static bool parse_from(CommandLineFlag &flag, turbo::string_view value,
                              flags_internal::FlagSettingMode set_mode,
                              flags_internal::ValueSource source, std::string &error);
    };

}  // namespace turbo::flags_internal
