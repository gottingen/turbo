// Copyright 2022 The Turbo Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_
#define TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_

#include <string>
#include <type_traits>
#include <utility>

#include "turbo/strings/string_view.h"

namespace turbo {
TURBO_NAMESPACE_BEGIN

namespace strings_internal {
class StringifySink {
 public:
  void Append(size_t count, char ch);

  void Append(std::string_view v);

  // Support `turbo::Format(&sink, format, args...)`.
  friend void TurboFormatFlush(StringifySink* sink, std::string_view v) {
    sink->Append(v);
  }

 private:
  template <typename T>
  friend std::string_view ExtractStringification(StringifySink& sink, const T& v);

  std::string buffer_;
};

template <typename T>
std::string_view ExtractStringification(StringifySink& sink, const T& v) {
  TurboStringify(sink, v);
  return sink.buffer_;
}

}  // namespace strings_internal

TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_STRINGIFY_SINK_H_
