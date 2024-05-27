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

#include <turbo/algorithm/algorithm.h>

#include <algorithm>
#include <list>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/config.h>

namespace {

class LinearSearchTest : public testing::Test {
 protected:
  LinearSearchTest() : container_{1, 2, 3} {}

  static bool Is3(int n) { return n == 3; }
  static bool Is4(int n) { return n == 4; }

  std::vector<int> container_;
};

TEST_F(LinearSearchTest, linear_search) {
  EXPECT_TRUE(turbo::linear_search(container_.begin(), container_.end(), 3));
  EXPECT_FALSE(turbo::linear_search(container_.begin(), container_.end(), 4));
}

TEST_F(LinearSearchTest, linear_searchConst) {
  const std::vector<int> *const const_container = &container_;
  EXPECT_TRUE(
      turbo::linear_search(const_container->begin(), const_container->end(), 3));
  EXPECT_FALSE(
      turbo::linear_search(const_container->begin(), const_container->end(), 4));
}

}  // namespace
