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

#include <turbo/strings/substitute.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <turbo/base/config.h>
#include <turbo/base/internal/raw_logging.h>
#include <turbo/base/nullability.h>
#include <turbo/strings/ascii.h>
#include <turbo/strings/escaping.h>
#include <turbo/strings/internal/resize_uninitialized.h>
#include <turbo/strings/numbers.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace substitute_internal {

void SubstituteAndAppendArray(
    turbo::Nonnull<std::string*> output, std::string_view format,
    turbo::Nullable<const std::string_view*> args_array, size_t num_args) {
  // Determine total size needed.
  size_t size = 0;
  for (size_t i = 0; i < format.size(); i++) {
    if (format[i] == '$') {
      if (i + 1 >= format.size()) {
#ifndef NDEBUG
        TURBO_RAW_LOG(FATAL,
                     "Invalid turbo::substitute() format string: \"%s\".",
                     turbo::c_encode(format).c_str());
#endif
        return;
      } else if (turbo::ascii_isdigit(
                     static_cast<unsigned char>(format[i + 1]))) {
        int index = format[i + 1] - '0';
        if (static_cast<size_t>(index) >= num_args) {
#ifndef NDEBUG
          TURBO_RAW_LOG(
              FATAL,
              "Invalid turbo::substitute() format string: asked for \"$"
              "%d\", but only %d args were given.  Full format string was: "
              "\"%s\".",
              index, static_cast<int>(num_args), turbo::c_encode(format).c_str());
#endif
          return;
        }
        size += args_array[index].size();
        ++i;  // Skip next char.
      } else if (format[i + 1] == '$') {
        ++size;
        ++i;  // Skip next char.
      } else {
#ifndef NDEBUG
        TURBO_RAW_LOG(FATAL,
                     "Invalid turbo::substitute() format string: \"%s\".",
                     turbo::c_encode(format).c_str());
#endif
        return;
      }
    } else {
      ++size;
    }
  }

  if (size == 0) return;

  // Build the string.
  size_t original_size = output->size();
  TURBO_INTERNAL_CHECK(
      size <= std::numeric_limits<size_t>::max() - original_size,
      "size_t overflow");
  strings_internal::STLStringResizeUninitializedAmortized(output,
                                                          original_size + size);
  char* target = &(*output)[original_size];
  for (size_t i = 0; i < format.size(); i++) {
    if (format[i] == '$') {
      if (turbo::ascii_isdigit(static_cast<unsigned char>(format[i + 1]))) {
        const std::string_view src = args_array[format[i + 1] - '0'];
        target = std::copy(src.begin(), src.end(), target);
        ++i;  // Skip next char.
      } else if (format[i + 1] == '$') {
        *target++ = '$';
        ++i;  // Skip next char.
      }
    } else {
      *target++ = format[i];
    }
  }

  assert(target == output->data() + output->size());
}

Arg::Arg(turbo::Nullable<const void*> value) {
  static_assert(sizeof(scratch_) >= sizeof(value) * 2 + 2,
                "fix sizeof(scratch_)");
  if (value == nullptr) {
    piece_ = "NULL";
  } else {
    char* ptr = scratch_ + sizeof(scratch_);
    uintptr_t num = reinterpret_cast<uintptr_t>(value);
    do {
      *--ptr = turbo::numbers_internal::kHexChar[num & 0xf];
      num >>= 4;
    } while (num != 0);
    *--ptr = 'x';
    *--ptr = '0';
    piece_ = std::string_view(
        ptr, static_cast<size_t>(scratch_ + sizeof(scratch_) - ptr));
  }
}

// TODO(jorg): Don't duplicate so much code between here and str_cat.cc
Arg::Arg(Hex hex) {
  char* const end = &scratch_[numbers_internal::kFastToBufferSize];
  char* writer = end;
  uint64_t value = hex.value;
  do {
    *--writer = turbo::numbers_internal::kHexChar[value & 0xF];
    value >>= 4;
  } while (value != 0);

  char* beg;
  if (end - writer < hex.width) {
    beg = end - hex.width;
    std::fill_n(beg, writer - beg, hex.fill);
  } else {
    beg = writer;
  }

  piece_ = std::string_view(beg, static_cast<size_t>(end - beg));
}

// TODO(jorg): Don't duplicate so much code between here and str_cat.cc
Arg::Arg(Dec dec) {
  assert(dec.width <= numbers_internal::kFastToBufferSize);
  char* const end = &scratch_[numbers_internal::kFastToBufferSize];
  char* const minfill = end - dec.width;
  char* writer = end;
  uint64_t value = dec.value;
  bool neg = dec.neg;
  while (value > 9) {
    *--writer = '0' + (value % 10);
    value /= 10;
  }
  *--writer = '0' + static_cast<char>(value);
  if (neg) *--writer = '-';

  ptrdiff_t fillers = writer - minfill;
  if (fillers > 0) {
    // Tricky: if the fill character is ' ', then it's <fill><+/-><digits>
    // But...: if the fill character is '0', then it's <+/-><fill><digits>
    bool add_sign_again = false;
    if (neg && dec.fill == '0') {  // If filling with '0',
      ++writer;                    // ignore the sign we just added
      add_sign_again = true;       // and re-add the sign later.
    }
    writer -= fillers;
    std::fill_n(writer, fillers, dec.fill);
    if (add_sign_again) *--writer = '-';
  }

  piece_ = std::string_view(writer, static_cast<size_t>(end - writer));
}

}  // namespace substitute_internal
TURBO_NAMESPACE_END
}  // namespace turbo
