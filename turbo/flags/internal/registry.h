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

#include <functional>

#include <turbo/base/config.h>
#include <turbo/flags/commandlineflag.h>
#include <turbo/flags/internal/commandlineflag.h>
#include <turbo/strings/string_view.h>

// --------------------------------------------------------------------
// Global flags registry API.

namespace turbo::flags_internal {

    // Executes specified visitor for each non-retired flag in the registry. While
    // callback are executed, the registry is locked and can't be changed.
    void ForEachFlag(std::function<void(CommandLineFlag &)> visitor);

    //-----------------------------------------------------------------------------

    bool RegisterCommandLineFlag(CommandLineFlag &, const char *filename);

    void FinalizeRegistry();

    //-----------------------------------------------------------------------------
    // Retired registrations:
    //
    // Retired flag registrations are treated specially. A 'retired' flag is
    // provided only for compatibility with automated invocations that still
    // name it.  A 'retired' flag:
    //   - is not bound to a C++ FLAGS_ reference.
    //   - has a type and a value, but that value is intentionally inaccessible.
    //   - does not appear in --help messages.
    //   - is fully supported by _all_ flag parsing routines.
    //   - consumes args normally, and complains about type mismatches in its
    //     argument.
    //   - emits a complaint but does not die (e.g. LOG(ERROR)) if it is
    //     accessed by name through the flags API for parsing or otherwise.
    //
    // The registrations for a flag happen in an unspecified order as the
    // initializers for the namespace-scope objects of a program are run.
    // Any number of weak registrations for a flag can weakly define the flag.
    // One non-weak registration will upgrade the flag from weak to non-weak.
    // Further weak registrations of a non-weak flag are ignored.
    //
    // This mechanism is designed to support moving dead flags into a
    // 'graveyard' library.  An example migration:
    //
    //   0: Remove references to this FLAGS_flagname in the C++ codebase.
    //   1: Register as 'retired' in old_lib.
    //   2: Make old_lib depend on graveyard.
    //   3: Add a redundant 'retired' registration to graveyard.
    //   4: Remove the old_lib 'retired' registration.
    //   5: Eventually delete the graveyard registration entirely.
    //

    // Retire flag with name "name" and type indicated by ops.
    void Retire(const char *name, FlagFastTypeId type_id, char *buf);

    constexpr size_t kRetiredFlagObjSize = 3 * sizeof(void *);
    constexpr size_t kRetiredFlagObjAlignment = alignof(void *);

    // Registered a retired flag with name 'flag_name' and type 'T'.
    template<typename T>
    class RetiredFlag {
    public:
        void Retire(const char *flag_name) {
            flags_internal::Retire(flag_name, base_internal::FastTypeId<T>(), buf_);
        }

    private:
        alignas(kRetiredFlagObjAlignment) char buf_[kRetiredFlagObjSize];
    };

}  // namespace turbo::flags_internal
