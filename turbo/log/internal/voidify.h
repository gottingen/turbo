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
// File: log/internal/voidify.h
// -----------------------------------------------------------------------------
//
// This class is used to explicitly ignore values in the conditional logging
// macros. This avoids compiler warnings like "value computed is not used" and
// "statement has no effect".

#ifndef TURBO_LOG_INTERNAL_VOIDIFY_H_
#define TURBO_LOG_INTERNAL_VOIDIFY_H_

#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

class Voidify final {
 public:
  // This has to be an operator with a precedence lower than << but higher than
  // ?:
  template <typename T>
  void operator&&(const T&) const&& {}
};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_VOIDIFY_H_
