//
//  Copyright 2022 The Turbo Authors.
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

#include "turbo/flags/internal/program_name.h"

#include <string>

#include "turbo/platform/attributes.h"
#include "turbo/platform/config.h"
#include "turbo/platform/const_init.h"
#include "turbo/platform/thread_annotations.h"
#include "turbo/flags/internal/path_util.h"
#include "turbo/strings/string_view.h"
#include "turbo/synchronization/mutex.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace flags_internal {

TURBO_CONST_INIT static turbo::Mutex program_name_guard(turbo::kConstInit);
TURBO_CONST_INIT static std::string* program_name
    TURBO_GUARDED_BY(program_name_guard) = nullptr;

std::string ProgramInvocationName() {
  turbo::MutexLock l(&program_name_guard);

  return program_name ? *program_name : "UNKNOWN";
}

std::string ShortProgramInvocationName() {
  turbo::MutexLock l(&program_name_guard);

  return program_name ? std::string(flags_internal::Basename(*program_name))
                      : "UNKNOWN";
}

void SetProgramInvocationName(std::string_view prog_name_str) {
  turbo::MutexLock l(&program_name_guard);

  if (!program_name)
    program_name = new std::string(prog_name_str);
  else
    program_name->assign(prog_name_str.data(), prog_name_str.size());
}

}  // namespace flags_internal
TURBO_NAMESPACE_END
}  // namespace turbo
