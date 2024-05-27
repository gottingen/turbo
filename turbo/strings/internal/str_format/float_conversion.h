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

#ifndef TURBO_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_
#define TURBO_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_

#include <turbo/strings/internal/str_format/extension.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace str_format_internal {

bool ConvertFloatImpl(float v, const FormatConversionSpecImpl &conv,
                      FormatSinkImpl *sink);

bool ConvertFloatImpl(double v, const FormatConversionSpecImpl &conv,
                      FormatSinkImpl *sink);

bool ConvertFloatImpl(long double v, const FormatConversionSpecImpl &conv,
                      FormatSinkImpl *sink);

}  // namespace str_format_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_STR_FORMAT_FLOAT_CONVERSION_H_
