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

#include <turbo/strings/has_stringify.h>

#include <string>

#include <gtest/gtest.h>
#include <optional>

namespace {

struct TypeWithoutTurboStringify {};

struct TypeWithTurboStringify {
  template <typename Sink>
  friend void turbo_stringify(Sink&, const TypeWithTurboStringify&) {}
};

TEST(HasTurboStringifyTest, Works) {
  EXPECT_FALSE(turbo::HasTurboStringify<int>::value);
  EXPECT_FALSE(turbo::HasTurboStringify<std::string>::value);
  EXPECT_FALSE(turbo::HasTurboStringify<TypeWithoutTurboStringify>::value);
  EXPECT_TRUE(turbo::HasTurboStringify<TypeWithTurboStringify>::value);
  EXPECT_FALSE(
      turbo::HasTurboStringify<std::optional<TypeWithTurboStringify>>::value);
}

}  // namespace
