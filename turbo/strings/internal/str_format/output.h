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
// Output extension hooks for the Format library.
// `internal::InvokeFlush` calls the appropriate flush function for the
// specified output argument.
// `BufferRawSink` is a simple output sink for a char buffer. Used by SnprintF.
// `FILERawSink` is a std::FILE* based sink. Used by PrintF and FprintF.

#ifndef TURBO_STRINGS_INTERNAL_STR_FORMAT_OUTPUT_H_
#define TURBO_STRINGS_INTERNAL_STR_FORMAT_OUTPUT_H_

#include <cstdio>
#include <ios>
#include <ostream>
#include <string>

#include <turbo/base/port.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace str_format_internal {

// RawSink implementation that writes into a char* buffer.
// It will not overflow the buffer, but will keep the total count of chars
// that would have been written.
class BufferRawSink {
 public:
  BufferRawSink(char* buffer, size_t size) : buffer_(buffer), size_(size) {}

  size_t total_written() const { return total_written_; }
  void Write(std::string_view v);

 private:
  char* buffer_;
  size_t size_;
  size_t total_written_ = 0;
};

// RawSink implementation that writes into a FILE*.
// It keeps track of the total number of bytes written and any error encountered
// during the writes.
class FILERawSink {
 public:
  explicit FILERawSink(std::FILE* output) : output_(output) {}

  void Write(std::string_view v);

  size_t count() const { return count_; }
  int error() const { return error_; }

 private:
  std::FILE* output_;
  int error_ = 0;
  size_t count_ = 0;
};

// Provide RawSink integration with common types from the STL.
inline void TurboFormatFlush(std::string* out, std::string_view s) {
  out->append(s.data(), s.size());
}
inline void TurboFormatFlush(std::ostream* out, std::string_view s) {
  out->write(s.data(), static_cast<std::streamsize>(s.size()));
}

inline void TurboFormatFlush(FILERawSink* sink, std::string_view v) {
  sink->Write(v);
}

inline void TurboFormatFlush(BufferRawSink* sink, std::string_view v) {
  sink->Write(v);
}

// This is a SFINAE to get a better compiler error message when the type
// is not supported.
template <typename T>
auto InvokeFlush(T* out, std::string_view s) -> decltype(TurboFormatFlush(out, s)) {
  TurboFormatFlush(out, s);
}

}  // namespace str_format_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_STR_FORMAT_OUTPUT_H_
