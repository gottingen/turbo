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
// File: commandlineflag.h
// -----------------------------------------------------------------------------
//
// This header file defines the `CommandLineFlag`, which acts as a type-erased
// handle for accessing metadata about the Turbo Flag in question.
//
// Because an actual Turbo flag is of an unspecified type, you should not
// manipulate or interact directly with objects of that type. Instead, use the
// CommandLineFlag type as an intermediary.

#pragma once

#include <memory>
#include <string>

#include <turbo/base/config.h>
#include <turbo/base/internal/fast_type_id.h>
#include <turbo/flags/internal/commandlineflag.h>
#include <turbo/strings/string_view.h>
#include <turbo/types/optional.h>

namespace turbo::flags_internal {
    class PrivateHandleAccessor;
}  // namespace turbo::flags_internal

namespace turbo {
// CommandLineFlag
//
// This type acts as a type-erased handle for an instance of an Turbo Flag and
// holds reflection information pertaining to that flag. Use CommandLineFlag to
// access a flag's name, location, help string etc.
//
// To obtain an turbo::CommandLineFlag, invoke `turbo::find_command_line_flag()`
// passing it the flag name string.
//
// Example:
//
//   // Obtain reflection handle for a flag named "flagname".
//   const turbo::CommandLineFlag* my_flag_data =
//        turbo::find_command_line_flag("flagname");
//
//   // Now you can get flag info from that reflection handle.
//   std::string flag_location = my_flag_data->Filename();
//   ...

// These are only used as constexpr global objects.
// They do not use a virtual destructor to simplify their implementation.
// They are not destroyed except at program exit, so leaks do not matter.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

    class CommandLineFlag {
    public:
        constexpr CommandLineFlag() = default;

        // Not copyable/assignable.
        CommandLineFlag(const CommandLineFlag &) = delete;

        CommandLineFlag &operator=(const CommandLineFlag &) = delete;

        // turbo::CommandLineFlag::IsOfType()
        //
        // Return true iff flag has type T.
        template<typename T>
        inline bool IsOfType() const {
            return TypeId() == base_internal::FastTypeId<T>();
        }

        // turbo::CommandLineFlag::TryGet()
        //
        // Attempts to retrieve the flag value. Returns value on success,
        // turbo::nullopt otherwise.
        template<typename T>
        turbo::optional<T> TryGet() const {
            if (IsRetired() || !IsOfType<T>()) {
                return turbo::nullopt;
            }

            // Implementation notes:
            //
            // We are wrapping a union around the value of `T` to serve three purposes:
            //
            //  1. `U.value` has correct size and alignment for a value of type `T`
            //  2. The `U.value` constructor is not invoked since U's constructor does
            //     not do it explicitly.
            //  3. The `U.value` destructor is invoked since U's destructor does it
            //     explicitly. This makes `U` a kind of RAII wrapper around non default
            //     constructible value of T, which is destructed when we leave the
            //     scope. We do need to destroy U.value, which is constructed by
            //     CommandLineFlag::Read even though we left it in a moved-from state
            //     after std::move.
            //
            // All of this serves to avoid requiring `T` being default constructible.
            union U {
                T value;

                U() {}

                ~U() { value.~T(); }
            };
            U u;

            Read(&u.value);
            // allow retired flags to be "read", so we can report invalid access.
            if (IsRetired()) {
                return turbo::nullopt;
            }
            return std::move(u.value);
        }

        // turbo::CommandLineFlag::Name()
        //
        // Returns name of this flag.
        virtual turbo::string_view Name() const = 0;

        // turbo::CommandLineFlag::Filename()
        //
        // Returns name of the file where this flag is defined.
        virtual std::string Filename() const = 0;

        // turbo::CommandLineFlag::Help()
        //
        // Returns help message associated with this flag.
        virtual std::string Help() const = 0;

        // turbo::CommandLineFlag::IsRetired()
        //
        // Returns true iff this object corresponds to retired flag.
        virtual bool IsRetired() const;

        // turbo::CommandLineFlag::DefaultValue()
        //
        // Returns the default value for this flag.
        virtual std::string DefaultValue() const = 0;

        // turbo::CommandLineFlag::CurrentValue()
        //
        // Returns the current value for this flag.
        virtual std::string CurrentValue() const = 0;

        // turbo::CommandLineFlag::ParseFrom()
        //
        // Sets the value of the flag based on specified string `value`. If the flag
        // was successfully set to new value, it returns true. Otherwise, sets `error`
        // to indicate the error, leaves the flag unchanged, and returns false.
        bool ParseFrom(turbo::string_view value, std::string *error);

    protected:
        ~CommandLineFlag() = default;

    private:
        friend class flags_internal::PrivateHandleAccessor;

        // Sets the value of the flag based on specified string `value`. If the flag
        // was successfully set to new value, it returns true. Otherwise, sets `error`
        // to indicate the error, leaves the flag unchanged, and returns false. There
        // are three ways to set the flag's value:
        //  * Update the current flag value
        //  * Update the flag's default value
        //  * Update the current flag value if it was never set before
        // The mode is selected based on `set_mode` parameter.
        virtual bool ParseFrom(turbo::string_view value,
                               flags_internal::FlagSettingMode set_mode,
                               flags_internal::ValueSource source,
                               std::string &error) = 0;

        // Returns id of the flag's value type.
        virtual flags_internal::FlagFastTypeId TypeId() const = 0;

        // Interface to save flag to some persistent state. Returns current flag state
        // or nullptr if flag does not support saving and restoring a state.
        virtual std::unique_ptr<flags_internal::FlagStateInterface> SaveState() = 0;

        // Copy-construct a new value of the flag's type in a memory referenced by
        // the dst based on the current flag's value.
        virtual void Read(void *dst) const = 0;

        // To be deleted. Used to return true if flag's current value originated from
        // command line.
        virtual bool IsSpecifiedOnCommandLine() const = 0;

        // Validates supplied value using validator or parseflag routine
        virtual bool ValidateInputValue(turbo::string_view value) const = 0;

        // Checks that flags default value can be converted to string and back to the
        // flag's value type.
        virtual void CheckDefaultValueParsingRoundtrip() const = 0;
    };

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

}  // namespace turbo
