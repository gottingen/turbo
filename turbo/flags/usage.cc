//
// Copyright 2019 The Turbo Authors.
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
#include "turbo/flags/usage.h"

#include <stdlib.h>

#include <string>
#include <mutex>

#include "turbo/base/internal/raw_logging.h"
#include "turbo/base/const_init.h"
#include "turbo/flags/internal/usage.h"
#include "turbo/platform/port.h"
#include "turbo/platform/thread_annotations.h"
#include "turbo/strings/string_view.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {
namespace {
TURBO_CONST_INIT std::mutex usage_message_guard;
TURBO_CONST_INIT std::string* program_usage_message
    TURBO_GUARDED_BY(usage_message_guard) = nullptr;
}  // namespace
}  // namespace flags_internal

// --------------------------------------------------------------------
// Sets the "usage" message to be used by help reporting routines.
void SetProgramUsageMessage(std::string_view new_usage_message) {
  std::unique_lock<std::mutex> lock(flags_internal::usage_message_guard);

  if (flags_internal::program_usage_message != nullptr) {
    TURBO_INTERNAL_LOG(FATAL, "SetProgramUsageMessage() called twice.");
    std::exit(1);
  }

  flags_internal::program_usage_message = new std::string(new_usage_message);
}

// --------------------------------------------------------------------
// Returns the usage message set by SetProgramUsageMessage().
// Note: We able to return std::string_view here only because calling
// SetProgramUsageMessage twice is prohibited.
std::string_view ProgramUsageMessage() {
  std::unique_lock<std::mutex> l(flags_internal::usage_message_guard);

  return flags_internal::program_usage_message != nullptr
             ? std::string_view(*flags_internal::program_usage_message)
             : "Warning: SetProgramUsageMessage() never called";
}

TURBO_NAMESPACE_END
}  // namespace turbo
