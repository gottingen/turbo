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

#include <turbo/strings/has_ostream_operator.h>

#include <ostream>
#include <string>

#include <gtest/gtest.h>
#include <turbo/types/optional.h>

namespace {

struct TypeWithoutOstreamOp {};

struct TypeWithOstreamOp {
  friend std::ostream& operator<<(std::ostream& os, const TypeWithOstreamOp&) {
    return os;
  }
};

TEST(HasOstreamOperatorTest, Works) {
  EXPECT_TRUE(turbo::HasOstreamOperator<int>::value);
  EXPECT_TRUE(turbo::HasOstreamOperator<std::string>::value);
  EXPECT_FALSE(turbo::HasOstreamOperator<turbo::optional<int>>::value);
  EXPECT_FALSE(turbo::HasOstreamOperator<TypeWithoutOstreamOp>::value);
  EXPECT_TRUE(turbo::HasOstreamOperator<TypeWithOstreamOp>::value);
}

}  // namespace
