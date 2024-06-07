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

#include <turbo/strings/internal/str_format/extension.h>

#include <errno.h>
#include <algorithm>
#include <string>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace str_format_internal {

std::string FlagsToString(Flags v) {
  std::string s;
  s.append(FlagsContains(v, Flags::kLeft) ? "-" : "");
  s.append(FlagsContains(v, Flags::kShowPos) ? "+" : "");
  s.append(FlagsContains(v, Flags::kSignCol) ? " " : "");
  s.append(FlagsContains(v, Flags::kAlt) ? "#" : "");
  s.append(FlagsContains(v, Flags::kZero) ? "0" : "");
  return s;
}

#ifdef TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL

#define TURBO_INTERNAL_X_VAL(id) \
  constexpr turbo::FormatConversionChar FormatConversionCharInternal::id;
TURBO_INTERNAL_CONVERSION_CHARS_EXPAND_(TURBO_INTERNAL_X_VAL, )
#undef TURBO_INTERNAL_X_VAL
// NOLINTNEXTLINE(readability-redundant-declaration)
constexpr turbo::FormatConversionChar FormatConversionCharInternal::kNone;

#define TURBO_INTERNAL_CHAR_SET_CASE(c) \
  constexpr FormatConversionCharSet FormatConversionCharSetInternal::c;
TURBO_INTERNAL_CONVERSION_CHARS_EXPAND_(TURBO_INTERNAL_CHAR_SET_CASE, )
#undef TURBO_INTERNAL_CHAR_SET_CASE

constexpr FormatConversionCharSet FormatConversionCharSetInternal::kStar;
constexpr FormatConversionCharSet FormatConversionCharSetInternal::kIntegral;
constexpr FormatConversionCharSet FormatConversionCharSetInternal::kFloating;
constexpr FormatConversionCharSet FormatConversionCharSetInternal::kNumeric;
constexpr FormatConversionCharSet FormatConversionCharSetInternal::kPointer;

#endif  // TURBO_INTERNAL_NEED_REDUNDANT_CONSTEXPR_DECL

bool FormatSinkImpl::PutPaddedString(std::string_view value, int width,
                                     int precision, bool left) {
  size_t space_remaining = 0;
  if (width >= 0)
    space_remaining = static_cast<size_t>(width);
  size_t n = value.size();
  if (precision >= 0) n = std::min(n, static_cast<size_t>(precision));
    std::string_view shown(value.data(), n);
  space_remaining = Excess(shown.size(), space_remaining);
  if (!left) Append(space_remaining, ' ');
  Append(shown);
  if (left) Append(space_remaining, ' ');
  return true;
}

}  // namespace str_format_internal
TURBO_NAMESPACE_END
}  // namespace turbo
