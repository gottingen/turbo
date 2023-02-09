// Copyright 2022 The Turbo Authors.
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
//
// -----------------------------------------------------------------------------
// File: log/internal/structured.h
// -----------------------------------------------------------------------------

#ifndef TURBO_LOG_INTERNAL_STRUCTURED_H_
#define TURBO_LOG_INTERNAL_STRUCTURED_H_

#include <ostream>

#include "turbo/log/internal/log_message.h"
#include "turbo/platform/port.h"
#include "turbo/strings/string_view.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace log_internal {

class TURBO_MUST_USE_RESULT AsLiteralImpl final {
 public:
  explicit AsLiteralImpl(turbo::string_view str) : str_(str) {}
  AsLiteralImpl(const AsLiteralImpl&) = default;
  AsLiteralImpl& operator=(const AsLiteralImpl&) = default;

 private:
  turbo::string_view str_;

  friend std::ostream& operator<<(std::ostream& os, AsLiteralImpl as_literal) {
    return os << as_literal.str_;
  }
  void AddToMessage(log_internal::LogMessage& m) {
    m.CopyToEncodedBuffer(str_, log_internal::LogMessage::StringType::kLiteral);
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
