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

#ifndef TURBO_BASE_INTERNAL_SCOPED_SET_ENV_H_
#define TURBO_BASE_INTERNAL_SCOPED_SET_ENV_H_

#include <string>

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace base_internal {

class ScopedSetEnv {
 public:
  ScopedSetEnv(const char* var_name, const char* new_value);
  ~ScopedSetEnv();

 private:
  std::string var_name_;
  std::string old_value_;

  // True if the environment variable was initially not set.
  bool was_unset_;
};

}  // namespace base_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_BASE_INTERNAL_SCOPED_SET_ENV_H_
