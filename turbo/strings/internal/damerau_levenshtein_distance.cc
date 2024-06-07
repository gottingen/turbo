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

#include <turbo/strings/internal/damerau_levenshtein_distance.h>

#include <algorithm>
#include <array>
#include <numeric>

#include <turbo/strings/string_view.h>
namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace strings_internal {
// Calculate DamerauLevenshtein (adjacent transpositions) distance
// between two strings,
// https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance. The
// algorithm follows the condition that no substring is edited more than once.
// While this can reduce is larger distance, it's a) a much simpler algorithm
// and b) more realistic for the case that typographic mistakes should be
// detected.
// When the distance is larger than cutoff, or one of the strings has more
// than MAX_SIZE=100 characters, the code returns min(MAX_SIZE, cutoff) + 1.
uint8_t CappedDamerauLevenshteinDistance(std::string_view s1,
                                         std::string_view s2, uint8_t cutoff) {
  const uint8_t MAX_SIZE = 100;
  const uint8_t _cutoff = std::min(MAX_SIZE, cutoff);
  const uint8_t cutoff_plus_1 = static_cast<uint8_t>(_cutoff + 1);

  if (s1.size() > s2.size()) std::swap(s1, s2);
  if (s1.size() + _cutoff < s2.size() || s2.size() > MAX_SIZE)
    return cutoff_plus_1;

  if (s1.empty())
    return static_cast<uint8_t>(s2.size());

  // Lower diagonal bound: y = x - lower_diag
  const uint8_t lower_diag =
      _cutoff - static_cast<uint8_t>(s2.size() - s1.size());
  // Upper diagonal bound: y = x + upper_diag
  const uint8_t upper_diag = _cutoff;

  // d[i][j] is the number of edits required to convert s1[0, i] to s2[0, j]
  std::array<std::array<uint8_t, MAX_SIZE + 2>, MAX_SIZE + 2> d;
  std::iota(d[0].begin(), d[0].begin() + upper_diag + 1, 0);
  d[0][cutoff_plus_1] = cutoff_plus_1;
  for (size_t i = 1; i <= s1.size(); ++i) {
    // Deduce begin of relevant window.
    size_t j_begin = 1;
    if (i > lower_diag) {
      j_begin = i - lower_diag;
      d[i][j_begin - 1] = cutoff_plus_1;
    } else {
      d[i][0] = static_cast<uint8_t>(i);
    }

    // Deduce end of relevant window.
    size_t j_end = i + upper_diag;
    if (j_end > s2.size()) {
      j_end = s2.size();
    } else {
      d[i][j_end + 1] = cutoff_plus_1;
    }

    for (size_t j = j_begin; j <= j_end; ++j) {
      const uint8_t deletion_distance = d[i - 1][j] + 1;
      const uint8_t insertion_distance = d[i][j - 1] + 1;
      const uint8_t mismatched_tail_cost = s1[i - 1] == s2[j - 1] ? 0 : 1;
      const uint8_t mismatch_distance = d[i - 1][j - 1] + mismatched_tail_cost;
      uint8_t transposition_distance = _cutoff + 1;
      if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] && s1[i - 2] == s2[j - 1])
        transposition_distance = d[i - 2][j - 2] + 1;
      d[i][j] = std::min({cutoff_plus_1, deletion_distance, insertion_distance,
                          mismatch_distance, transposition_distance});
    }
  }
  return d[s1.size()][s2.size()];
}

}  // namespace strings_internal

TURBO_NAMESPACE_END
}  // namespace turbo
