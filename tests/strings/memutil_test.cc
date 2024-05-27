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

// Unit test for memutil.cc

#include <turbo/strings/internal/memutil.h>

#include <cstdlib>

#include <gtest/gtest.h>

namespace {

TEST(MemUtil, memcasecmp) {
  // check memutil functions
  const char a[] = "hello there";

  EXPECT_EQ(turbo::strings_internal::memcasecmp(a, "heLLO there",
                                               sizeof("hello there") - 1),
            0);
  EXPECT_EQ(turbo::strings_internal::memcasecmp(a, "heLLO therf",
                                               sizeof("hello there") - 1),
            -1);
  EXPECT_EQ(turbo::strings_internal::memcasecmp(a, "heLLO therf",
                                               sizeof("hello there") - 2),
            0);
  EXPECT_EQ(turbo::strings_internal::memcasecmp(a, "whatever", 0), 0);
}

}  // namespace
