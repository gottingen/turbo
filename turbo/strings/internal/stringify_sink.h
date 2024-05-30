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

#ifndef TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_
#define TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_

#include <string>
#include <type_traits>
#include <utility>

#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace strings_internal {
class StringifySink {
 public:
  void Append(size_t count, char ch);

  void Append(string_view v);

  // Support `turbo::format(&sink, format, args...)`.
  friend void TurboFormatFlush(StringifySink* sink, turbo::string_view v) {
    sink->Append(v);
  }

 private:
  template <typename T>
  friend string_view ExtractStringification(StringifySink& sink, const T& v);

  std::string buffer_;
};

template <typename T>
string_view ExtractStringification(StringifySink& sink, const T& v) {
  turbo_stringify(sink, v);
  return sink.buffer_;
}

}  // namespace strings_internal

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_
