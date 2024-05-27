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

#ifndef TURBO_FLAGS_INTERNAL_COMMANDLINEFLAG_H_
#define TURBO_FLAGS_INTERNAL_COMMANDLINEFLAG_H_

#include <turbo/base/config.h>
#include <turbo/base/internal/fast_type_id.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {

// An alias for flag fast type id. This value identifies the flag value type
// similarly to typeid(T), without relying on RTTI being available. In most
// cases this id is enough to uniquely identify the flag's value type. In a few
// cases we'll have to resort to using actual RTTI implementation if it is
// available.
using FlagFastTypeId = turbo::base_internal::FastTypeIdType;

// Options that control SetCommandLineOptionWithMode.
enum FlagSettingMode {
  // update the flag's value unconditionally (can call this multiple times).
  SET_FLAGS_VALUE,
  // update the flag's value, but *only if* it has not yet been updated
  // with SET_FLAGS_VALUE, SET_FLAG_IF_DEFAULT, or "FLAGS_xxx = nondef".
  SET_FLAG_IF_DEFAULT,
  // set the flag's default value to this.  If the flag has not been updated
  // yet (via SET_FLAGS_VALUE, SET_FLAG_IF_DEFAULT, or "FLAGS_xxx = nondef")
  // change the flag's current value to the new default value as well.
  SET_FLAGS_DEFAULT
};

// Options that control ParseFrom: Source of a value.
enum ValueSource {
  // Flag is being set by value specified on a command line.
  kCommandLine,
  // Flag is being set by value specified in the code.
  kProgrammaticChange,
};

// Handle to FlagState objects. Specific flag state objects will restore state
// of a flag produced this flag state from method CommandLineFlag::SaveState().
class FlagStateInterface {
 public:
  virtual ~FlagStateInterface();

  // Restores the flag originated this object to the saved state.
  virtual void Restore() const = 0;
};

}  // namespace flags_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FLAGS_INTERNAL_COMMANDLINEFLAG_H_
