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

#include <cstdint>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using turbo::strings_internal::CappedDamerauLevenshteinDistance;

TEST(Distance, TestDistances) {
  EXPECT_THAT(CappedDamerauLevenshteinDistance("ab", "ab", 6), uint8_t{0});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("a", "b", 6), uint8_t{1});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("ca", "abc", 6), uint8_t{3});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abcd", "ad", 6), uint8_t{2});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abcd", "cadb", 6), uint8_t{4});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abcd", "bdac", 6), uint8_t{4});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("ab", "ab", 0), uint8_t{0});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("", "", 0), uint8_t{0});
  // combinations for 3-character strings:
  // 1, 2, 3 removals, insertions or replacements and transpositions
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abc", "abc", 6), uint8_t{0});
  for (auto res :
       {"", "ca", "efg", "ea", "ce", "ceb", "eca", "cae", "cea", "bea"}) {
    EXPECT_THAT(CappedDamerauLevenshteinDistance("abc", res, 6), uint8_t{3});
    EXPECT_THAT(CappedDamerauLevenshteinDistance(res, "abc", 6), uint8_t{3});
  }
  for (auto res :
       {"a",   "b",   "c",   "ba",  "cb",  "bca", "cab", "cba", "ace",
        "efc", "ebf", "aef", "ae",  "be",  "eb",  "ec",  "ecb", "bec",
        "bce", "cbe", "ace", "eac", "aeb", "bae", "eab", "eba"}) {
    EXPECT_THAT(CappedDamerauLevenshteinDistance("abc", res, 6), uint8_t{2});
    EXPECT_THAT(CappedDamerauLevenshteinDistance(res, "abc", 6), uint8_t{2});
  }
  for (auto res : {"ab", "ac", "bc", "acb", "bac", "ebc", "aec", "abe"}) {
    EXPECT_THAT(CappedDamerauLevenshteinDistance("abc", res, 6), uint8_t{1});
    EXPECT_THAT(CappedDamerauLevenshteinDistance(res, "abc", 6), uint8_t{1});
  }
}

TEST(Distance, TestCutoff) {
  // Returning cutoff + 1 if the value is larger than cutoff or string longer
  // than MAX_SIZE.
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abcd", "a", 3), uint8_t{3});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abcd", "a", 2), uint8_t{3});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abcd", "a", 1), uint8_t{2});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("abcdefg", "a", 2), uint8_t{3});
  EXPECT_THAT(CappedDamerauLevenshteinDistance("a", "abcde", 2), uint8_t{3});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(102, 'a'),
                                               std::string(102, 'a'), 105),
              uint8_t{101});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(100, 'a'),
                                               std::string(100, 'a'), 100),
              uint8_t{0});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(100, 'a'),
                                               std::string(100, 'b'), 100),
              uint8_t{100});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(100, 'a'),
                                               std::string(99, 'a'), 2),
              uint8_t{1});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(100, 'a'),
                                               std::string(101, 'a'), 2),
              uint8_t{3});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(100, 'a'),
                                               std::string(101, 'a'), 2),
              uint8_t{3});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(UINT8_MAX + 1, 'a'),
                                               std::string(UINT8_MAX + 1, 'b'),
                                               UINT8_MAX),
              uint8_t{101});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(UINT8_MAX - 1, 'a'),
                                               std::string(UINT8_MAX - 1, 'b'),
                                               UINT8_MAX),
              uint8_t{101});
  EXPECT_THAT(
      CappedDamerauLevenshteinDistance(std::string(UINT8_MAX, 'a'),
                                       std::string(UINT8_MAX, 'b'), UINT8_MAX),
      uint8_t{101});
  EXPECT_THAT(CappedDamerauLevenshteinDistance(std::string(UINT8_MAX - 1, 'a'),
                                               std::string(UINT8_MAX - 1, 'a'),
                                               UINT8_MAX),
              uint8_t{101});
}
}  // namespace
