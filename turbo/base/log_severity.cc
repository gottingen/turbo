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

#include <turbo/base/log_severity.h>

#include <ostream>

#include <turbo/base/attributes.h>
#include <turbo/base/config.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

std::ostream& operator<<(std::ostream& os, turbo::LogSeverity s) {
  if (s == turbo::NormalizeLogSeverity(s)) return os << turbo::LogSeverityName(s);
  return os << "turbo::LogSeverity(" << static_cast<int>(s) << ")";
}

std::ostream& operator<<(std::ostream& os, turbo::LogSeverityAtLeast s) {
  switch (s) {
    case turbo::LogSeverityAtLeast::kInfo:
    case turbo::LogSeverityAtLeast::kWarning:
    case turbo::LogSeverityAtLeast::kError:
    case turbo::LogSeverityAtLeast::kFatal:
      return os << ">=" << static_cast<turbo::LogSeverity>(s);
    case turbo::LogSeverityAtLeast::kInfinity:
      return os << "INFINITY";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, turbo::LogSeverityAtMost s) {
  switch (s) {
    case turbo::LogSeverityAtMost::kInfo:
    case turbo::LogSeverityAtMost::kWarning:
    case turbo::LogSeverityAtMost::kError:
    case turbo::LogSeverityAtMost::kFatal:
      return os << "<=" << static_cast<turbo::LogSeverity>(s);
    case turbo::LogSeverityAtMost::kNegativeInfinity:
      return os << "NEGATIVE_INFINITY";
  }
  return os;
}
TURBO_NAMESPACE_END
}  // namespace turbo
