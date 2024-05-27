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

#ifndef TURBO_FLAGS_INTERNAL_PARSE_H_
#define TURBO_FLAGS_INTERNAL_PARSE_H_

#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include <turbo/base/config.h>
#include <turbo/flags/declare.h>
#include <turbo/flags/internal/usage.h>
#include <turbo/strings/string_view.h>

TURBO_DECLARE_FLAG(std::vector<std::string>, flagfile);
TURBO_DECLARE_FLAG(std::vector<std::string>, fromenv);
TURBO_DECLARE_FLAG(std::vector<std::string>, tryfromenv);
TURBO_DECLARE_FLAG(std::vector<std::string>, undefok);

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {

enum class UsageFlagsAction { kHandleUsage, kIgnoreUsage };
enum class OnUndefinedFlag {
  kIgnoreUndefined,
  kReportUndefined,
  kAbortIfUndefined
};

// This is not a public interface. This interface exists to expose the ability
// to change help output stream in case of parsing errors. This is used by
// internal unit tests to validate expected outputs.
// When this was written, `EXPECT_EXIT` only supported matchers on stderr,
// but not on stdout.
std::vector<char*> ParseCommandLineImpl(
    int argc, char* argv[], UsageFlagsAction usage_flag_action,
    OnUndefinedFlag undef_flag_action,
    std::ostream& error_help_output = std::cout);

// --------------------------------------------------------------------
// Inspect original command line

// Returns true if flag with specified name was either present on the original
// command line or specified in flag file present on the original command line.
bool WasPresentOnCommandLine(turbo::string_view flag_name);

// Return existing flags similar to the parameter, in order to help in case of
// misspellings.
std::vector<std::string> GetMisspellingHints(turbo::string_view flag);

}  // namespace flags_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FLAGS_INTERNAL_PARSE_H_
