//
// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_FLAGS_INTERNAL_PRIVATE_HANDLE_ACCESSOR_H_
#define TURBO_FLAGS_INTERNAL_PRIVATE_HANDLE_ACCESSOR_H_

#include <memory>
#include <string>

#include "turbo/flags/commandlineflag.h"
#include "turbo/flags/internal/commandlineflag.h"
#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {

// This class serves as a trampoline to access private methods of
// CommandLineFlag. This class is intended for use exclusively internally inside
// of the Turbo Flags implementation.
class PrivateHandleAccessor {
 public:
  // Access to CommandLineFlag::TypeId.
  static FlagFastTypeId TypeId(const CommandLineFlag& flag);

  // Access to CommandLineFlag::SaveState.
  static std::unique_ptr<FlagStateInterface> SaveState(CommandLineFlag& flag);

  // Access to CommandLineFlag::IsSpecifiedOnCommandLine.
  static bool IsSpecifiedOnCommandLine(const CommandLineFlag& flag);

  // Access to CommandLineFlag::ValidateInputValue.
  static bool ValidateInputValue(const CommandLineFlag& flag,
                                 turbo::string_view value);

  // Access to CommandLineFlag::CheckDefaultValueParsingRoundtrip.
  static void CheckDefaultValueParsingRoundtrip(const CommandLineFlag& flag);

  static bool ParseFrom(CommandLineFlag& flag, turbo::string_view value,
                        flags_internal::FlagSettingMode set_mode,
                        flags_internal::ValueSource source, std::string& error);
};

}  // namespace flags_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_FLAGS_INTERNAL_PRIVATE_HANDLE_ACCESSOR_H_
