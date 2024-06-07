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
// File: log/internal/structured.h
// -----------------------------------------------------------------------------

#ifndef TURBO_LOG_INTERNAL_STRUCTURED_H_
#define TURBO_LOG_INTERNAL_STRUCTURED_H_

#include <ostream>

#include <turbo/base/config.h>
#include <turbo/log/internal/log_message.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

class TURBO_MUST_USE_RESULT AsLiteralImpl final {
 public:
  explicit AsLiteralImpl(std::string_view str) : str_(str) {}
  AsLiteralImpl(const AsLiteralImpl&) = default;
  AsLiteralImpl& operator=(const AsLiteralImpl&) = default;

 private:
  std::string_view str_;

  friend std::ostream& operator<<(std::ostream& os, AsLiteralImpl as_literal) {
    return os << as_literal.str_;
  }
  void AddToMessage(log_internal::LogMessage& m) {
    m.CopyToEncodedBuffer<log_internal::LogMessage::StringType::kLiteral>(str_);
  }
  friend log_internal::LogMessage& operator<<(log_internal::LogMessage& m,
                                              AsLiteralImpl as_literal) {
    as_literal.AddToMessage(m);
    return m;
  }
};

}  // namespace log_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_LOG_INTERNAL_STRUCTURED_H_
