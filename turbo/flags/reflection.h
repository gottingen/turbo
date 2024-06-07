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
// File: reflection.h
// -----------------------------------------------------------------------------
//
// This file defines the routines to access and operate on an Turbo Flag's
// reflection handle.

#pragma once

#include <string>

#include <turbo/base/config.h>
#include <turbo/container/flat_hash_map.h>
#include <turbo/flags/commandlineflag.h>
#include <turbo/flags/internal/commandlineflag.h>

namespace turbo {
    namespace flags_internal {
        class FlagSaverImpl;
    }  // namespace flags_internal

    // find_command_line_flag()
    //
    // Returns the reflection handle of an Turbo flag of the specified name, or
    // `nullptr` if not found. This function will emit a warning if the name of a
    // 'retired' flag is specified.
    turbo::CommandLineFlag *find_command_line_flag(std::string_view name);

    // Returns current state of the Flags registry in a form of mapping from flag
    // name to a flag reflection handle.
    turbo::flat_hash_map<std::string_view, turbo::CommandLineFlag *> get_all_flags();

    //------------------------------------------------------------------------------
    // FlagSaver
    //------------------------------------------------------------------------------
    //
    // A FlagSaver object stores the state of flags in the scope where the FlagSaver
    // is defined, allowing modification of those flags within that scope and
    // automatic restoration of the flags to their previous state upon leaving the
    // scope.
    //
    // A FlagSaver can be used within tests to temporarily change the test
    // environment and restore the test case to its previous state.
    //
    // Example:
    //
    //   void MyFunc() {
    //    turbo::FlagSaver fs;
    //    ...
    //    turbo::set_flag(&FLAGS_myFlag, otherValue);
    //    ...
    //  } // scope of FlagSaver left, flags return to previous state
    //
    // This class is thread-safe.

    class FlagSaver {
    public:
        FlagSaver();

        ~FlagSaver();

        FlagSaver(const FlagSaver &) = delete;

        void operator=(const FlagSaver &) = delete;

    private:
        flags_internal::FlagSaverImpl *impl_;
    };


}  // namespace turbo
