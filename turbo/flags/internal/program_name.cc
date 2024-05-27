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

#include <turbo/flags/internal/program_name.h>

#include <string>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>
#include <turbo/base/const_init.h>
#include <turbo/base/thread_annotations.h>
#include <turbo/flags/internal/path_util.h>
#include <turbo/strings/string_view.h>
#include <turbo/synchronization/mutex.h>

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

void SetProgramInvocationName(turbo::string_view prog_name_str) {
  turbo::MutexLock l(&program_name_guard);

  if (!program_name)
    program_name = new std::string(prog_name_str);
  else
    program_name->assign(prog_name_str.data(), prog_name_str.size());
}

}  // namespace flags_internal
TURBO_NAMESPACE_END
}  // namespace turbo
