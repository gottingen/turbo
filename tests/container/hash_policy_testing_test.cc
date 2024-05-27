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

#include <tests/container/hash_policy_testing.h>

#include <gtest/gtest.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace container_internal {
namespace {

TEST(_, Hash) {
  StatefulTestingHash h1;
  EXPECT_EQ(1, h1.id());
  StatefulTestingHash h2;
  EXPECT_EQ(2, h2.id());
  StatefulTestingHash h1c(h1);
  EXPECT_EQ(1, h1c.id());
  StatefulTestingHash h2m(std::move(h2));
  EXPECT_EQ(2, h2m.id());
  EXPECT_EQ(0, h2.id());
  StatefulTestingHash h3;
  EXPECT_EQ(3, h3.id());
  h3 = StatefulTestingHash();
  EXPECT_EQ(4, h3.id());
  h3 = std::move(h1);
  EXPECT_EQ(1, h3.id());
}

}  // namespace
}  // namespace container_internal
TURBO_NAMESPACE_END
}  // namespace turbo
