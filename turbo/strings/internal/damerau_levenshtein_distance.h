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

#ifndef TURBO_STRINGS_INTERNAL_DAMERAU_LEVENSHTEIN_DISTANCE_H_
#define TURBO_STRINGS_INTERNAL_DAMERAU_LEVENSHTEIN_DISTANCE_H_

#include <cstdint>

#include <turbo/strings/string_view.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {
// Calculate DamerauLevenshtein distance between two strings.
// When the distance is larger than cutoff, the code just returns cutoff + 1.
uint8_t CappedDamerauLevenshteinDistance(turbo::string_view s1,
                                         turbo::string_view s2, uint8_t cutoff);

}  // namespace strings_internal
TURBO_NAMESPACE_END
}  // namespace turbo

#endif  // TURBO_STRINGS_INTERNAL_DAMERAU_LEVENSHTEIN_DISTANCE_H_
