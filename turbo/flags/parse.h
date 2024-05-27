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
//
// -----------------------------------------------------------------------------
// File: parse.h
// -----------------------------------------------------------------------------
//
// This file defines the main parsing function for Turbo flags:
// `turbo::ParseCommandLine()`.

#ifndef TURBO_FLAGS_PARSE_H_
#define TURBO_FLAGS_PARSE_H_

#include <string>
#include <vector>

#include <turbo/base/config.h>
#include <turbo/flags/internal/parse.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

// This type represent information about an unrecognized flag in the command
// line.
struct UnrecognizedFlag {
  enum Source { kFromArgv, kFromFlagfile };

  explicit UnrecognizedFlag(Source s, turbo::string_view f)
      : source(s), flag_name(f) {}
  // This field indicates where we found this flag: on the original command line
  // or read in some flag file.
  Source source;
  // Name of the flag we did not recognize in --flag_name=value or --flag_name.
  std::string flag_name;
};

inline bool operator==(const UnrecognizedFlag& lhs,
                       const UnrecognizedFlag& rhs) {
  return lhs.source == rhs.source && lhs.flag_name == rhs.flag_name;
}

namespace flags_internal {

HelpMode ParseTurboFlagsOnlyImpl(
    int argc, char* argv[], std::vector<char*>& positional_args,
    std::vector<UnrecognizedFlag>& unrecognized_flags,
    UsageFlagsAction usage_flag_action);

}  // namespace flags_internal

// ParseTurboFlagsOnly()
//
// Parses a list of command-line arguments, passed in the `argc` and `argv[]`
// parameters, into a set of Turbo Flag values, returning any unparsed
// arguments in `positional_args` and `unrecognized_flags` output parameters.
//
// This function classifies all the arguments (including content of the
// flagfiles, if any) into one of the following groups:
//
//   * arguments specified as "--flag=value" or "--flag value" that match
//     registered or built-in Turbo Flags. These are "Turbo Flag arguments."
//   * arguments specified as "--flag" that are unrecognized as Turbo Flags
//   * arguments that are not specified as "--flag" are positional arguments
//   * arguments that follow the flag-terminating delimiter (`--`) are also
//     treated as positional arguments regardless of their syntax.
//
// All of the deduced Turbo Flag arguments are then parsed into their
// corresponding flag values. If any syntax errors are found in these arguments,
// the binary exits with code 1.
//
// This function also handles Turbo Flags built-in usage flags (e.g. --help)
// if any were present on the command line.
//
// All the remaining positional arguments including original program name
// (argv[0]) are are returned in the `positional_args` output parameter.
//
// All unrecognized flags that are not otherwise ignored are returned in the
// `unrecognized_flags` output parameter. Note that the special `undefok`
// flag allows you to specify flags which can be safely ignored; `undefok`
// specifies these flags as a comma-separated list. Any unrecognized flags
// that appear within `undefok` will therefore be ignored and not included in
// the `unrecognized_flag` output parameter.
//
void ParseTurboFlagsOnly(int argc, char* argv[],
                          std::vector<char*>& positional_args,
                          std::vector<UnrecognizedFlag>& unrecognized_flags);

// ReportUnrecognizedFlags()
//
// Reports an error to `stderr` for all non-ignored unrecognized flags in
// the provided `unrecognized_flags` list.
void ReportUnrecognizedFlags(
    const std::vector<UnrecognizedFlag>& unrecognized_flags);

// ParseCommandLine()
//
// First parses Turbo Flags only from the command line according to the
// description in `ParseTurboFlagsOnly`. In addition this function handles
// unrecognized and usage flags.
//
// If any unrecognized flags are located they are reported using
// `ReportUnrecognizedFlags`.
//
// If any errors detected during command line parsing, this routine reports a
// usage message and aborts the program.
//
// If any built-in usage flags were specified on the command line (e.g.
// `--help`), this function reports help messages and then gracefully exits the
// program.
//
// This function returns all the remaining positional arguments collected by
// `ParseTurboFlagsOnly`.
std::vector<char*> ParseCommandLine(int argc, char* argv[]);

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FLAGS_PARSE_H_
