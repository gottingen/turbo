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
// File: usage_config.h
// -----------------------------------------------------------------------------
//
// This file defines the main usage reporting configuration interfaces and
// documents Turbo's supported built-in usage flags. If these flags are found
// when parsing a command-line, Turbo will exit the program and display
// appropriate help messages.

#pragma once

#include <functional>
#include <string>

#include <turbo/base/config.h>
#include <turbo/strings/string_view.h>

// -----------------------------------------------------------------------------
// Built-in Usage Flags
// -----------------------------------------------------------------------------
//
// Turbo supports the following built-in usage flags. When passed, these flags
// exit the program and :
//
// * --help
//     Shows help on important flags for this binary
// * --helpfull
//     Shows help on all flags
// * --helpshort
//     Shows help on only the main module for this program
// * --helppackage
//     Shows help on all modules in the main package
// * --version
//     Shows the version and build info for this binary and exits
// * --only_check_args
//     Exits after checking all flags
// * --helpon
//     Shows help on the modules named by this flag value
// * --helpmatch
//     Shows help on modules whose name contains the specified substring

namespace turbo {

    namespace flags_internal {
        using FlagKindFilter = std::function<bool(std::string_view)>;
    }  // namespace flags_internal

    // FlagsUsageConfig
    //
    // This structure contains the collection of callbacks for changing the behavior
    // of the usage reporting routines in Turbo Flags.
    struct FlagsUsageConfig {
        // Returns true if flags defined in the given source code file should be
        // reported with --helpshort flag. For example, if the file
        // "path/to/my/code.cc" defines the flag "--my_flag", and
        // contains_helpshort_flags("path/to/my/code.cc") returns true, invoking the
        // program with --helpshort will include information about --my_flag in the
        // program output.
        flags_internal::FlagKindFilter contains_helpshort_flags;

        // Returns true if flags defined in the filename should be reported with
        // --help flag. For example, if the file
        // "path/to/my/code.cc" defines the flag "--my_flag", and
        // contains_help_flags("path/to/my/code.cc") returns true, invoking the
        // program with --help will include information about --my_flag in the
        // program output.
        flags_internal::FlagKindFilter contains_help_flags;

        // Returns true if flags defined in the filename should be reported with
        // --helppackage flag. For example, if the file
        // "path/to/my/code.cc" defines the flag "--my_flag", and
        // contains_helppackage_flags("path/to/my/code.cc") returns true, invoking the
        // program with --helppackage will include information about --my_flag in the
        // program output.
        flags_internal::FlagKindFilter contains_helppackage_flags;

        // Generates string containing program version. This is the string reported
        // when user specifies --version in a command line.
        std::function<std::string()> version_string;

        // Normalizes the filename specific to the build system/filesystem used. This
        // routine is used when we report the information about the flag definition
        // location. For instance, if your build resides at some location you do not
        // want to expose in the usage output, you can trim it to show only relevant
        // part.
        // For example:
        //   normalize_filename("/my_company/some_long_path/src/project/file.cc")
        // might produce
        //   "project/file.cc".
        std::function<std::string(std::string_view)> normalize_filename;
    };

    // set_flags_usage_config()
    //
    // Sets the usage reporting configuration callbacks. If any of the callbacks are
    // not set in usage_config instance, then the default value of the callback is
    // used.
    void set_flags_usage_config(FlagsUsageConfig usage_config);

    namespace flags_internal {

        FlagsUsageConfig GetUsageConfig();

        void ReportUsageError(std::string_view msg, bool is_fatal);

    }  // namespace flags_internal
}  // namespace turbo

extern "C" {

// Additional report of fatal usage error message before we std::exit. Error is
// fatal if is_fatal argument to ReportUsageError is true.
void TURBO_INTERNAL_C_SYMBOL(TurboInternalReportFatalUsageError)(
        std::string_view);

}  // extern "C"
