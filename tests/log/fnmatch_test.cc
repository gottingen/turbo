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

#include <turbo/log/internal/fnmatch.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {
using ::testing::IsFalse;
using ::testing::IsTrue;

TEST(FNMatchTest, Works) {
  using turbo::log_internal::FNMatch;
  EXPECT_THAT(FNMatch("foo", "foo"), IsTrue());
  EXPECT_THAT(FNMatch("foo", "bar"), IsFalse());
  EXPECT_THAT(FNMatch("foo", "fo"), IsFalse());
  EXPECT_THAT(FNMatch("foo", "foo2"), IsFalse());
  EXPECT_THAT(FNMatch("bar/foo.ext", "bar/foo.ext"), IsTrue());
  EXPECT_THAT(FNMatch("*ba*r/fo*o.ext*", "bar/foo.ext"), IsTrue());
  EXPECT_THAT(FNMatch("bar/foo.ext", "bar/baz.ext"), IsFalse());
  EXPECT_THAT(FNMatch("bar/foo.ext", "bar/foo"), IsFalse());
  EXPECT_THAT(FNMatch("bar/foo.ext", "bar/foo.ext.zip"), IsFalse());
  EXPECT_THAT(FNMatch("ba?/*.ext", "bar/foo.ext"), IsTrue());
  EXPECT_THAT(FNMatch("ba?/*.ext", "baZ/FOO.ext"), IsTrue());
  EXPECT_THAT(FNMatch("ba?/*.ext", "barr/foo.ext"), IsFalse());
  EXPECT_THAT(FNMatch("ba?/*.ext", "bar/foo.ext2"), IsFalse());
  EXPECT_THAT(FNMatch("ba?/*", "bar/foo.ext2"), IsTrue());
  EXPECT_THAT(FNMatch("ba?/*", "bar/"), IsTrue());
  EXPECT_THAT(FNMatch("ba?/?", "bar/"), IsFalse());
  EXPECT_THAT(FNMatch("ba?/*", "bar"), IsFalse());
  EXPECT_THAT(FNMatch("?x", "zx"), IsTrue());
  EXPECT_THAT(FNMatch("*b", "aab"), IsTrue());
  EXPECT_THAT(FNMatch("a*b", "aXb"), IsTrue());
  EXPECT_THAT(FNMatch("", ""), IsTrue());
  EXPECT_THAT(FNMatch("", "a"), IsFalse());
  EXPECT_THAT(FNMatch("ab*", "ab"), IsTrue());
  EXPECT_THAT(FNMatch("ab**", "ab"), IsTrue());
  EXPECT_THAT(FNMatch("ab*?", "ab"), IsFalse());
  EXPECT_THAT(FNMatch("*", "bbb"), IsTrue());
  EXPECT_THAT(FNMatch("*", ""), IsTrue());
  EXPECT_THAT(FNMatch("?", ""), IsFalse());
  EXPECT_THAT(FNMatch("***", "**p"), IsTrue());
  EXPECT_THAT(FNMatch("**", "*"), IsTrue());
  EXPECT_THAT(FNMatch("*?", "*"), IsTrue());
}

}  // namespace
