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

#include <turbo/flags/internal/private_handle_accessor.h>

#include <memory>
#include <string>

#include <turbo/base/config.h>
#include <turbo/flags/commandlineflag.h>
#include <turbo/flags/internal/commandlineflag.h>
#include <turbo/strings/string_view.h>

namespace turbo::flags_internal {

    FlagFastTypeId PrivateHandleAccessor::TypeId(const CommandLineFlag &flag) {
        return flag.type_id();
    }

    std::unique_ptr<FlagStateInterface> PrivateHandleAccessor::SaveState(
            CommandLineFlag &flag) {
        return flag.SaveState();
    }

    bool PrivateHandleAccessor::is_specified_on_commandLine(
            const CommandLineFlag &flag) {
        return flag.is_specified_on_commandLine();
    }

    bool PrivateHandleAccessor::validate_input_value(const CommandLineFlag &flag,
                                                   turbo::string_view value) {
        return flag.validate_input_value(value);
    }

    void PrivateHandleAccessor::check_default_value_parsing_roundtrip(
            const CommandLineFlag &flag) {
        flag.check_default_value_parsing_roundtrip();
    }

    bool PrivateHandleAccessor::parse_from(CommandLineFlag &flag,
                                          turbo::string_view value,
                                          flags_internal::FlagSettingMode set_mode,
                                          flags_internal::ValueSource source,
                                          std::string &error) {
        return flag.parse_from(value, set_mode, source, error);
    }

}  // namespace turbo::flags_internal

