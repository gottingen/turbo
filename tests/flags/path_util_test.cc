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

#include <turbo/flags/internal/path_util.h>

#include <gtest/gtest.h>

namespace {

namespace flags = turbo::flags_internal;

TEST(FlagsPathUtilTest, TestBasename) {
  EXPECT_EQ(flags::Basename(""), "");
  EXPECT_EQ(flags::Basename("a.cc"), "a.cc");
  EXPECT_EQ(flags::Basename("dir/a.cc"), "a.cc");
  EXPECT_EQ(flags::Basename("dir1/dir2/a.cc"), "a.cc");
  EXPECT_EQ(flags::Basename("../dir1/dir2/a.cc"), "a.cc");
  EXPECT_EQ(flags::Basename("/dir1/dir2/a.cc"), "a.cc");
  EXPECT_EQ(flags::Basename("/dir1/dir2/../dir3/a.cc"), "a.cc");
}

// --------------------------------------------------------------------

TEST(FlagsPathUtilTest, TestPackage) {
  EXPECT_EQ(flags::Package(""), "");
  EXPECT_EQ(flags::Package("a.cc"), "");
  EXPECT_EQ(flags::Package("dir/a.cc"), "dir/");
  EXPECT_EQ(flags::Package("dir1/dir2/a.cc"), "dir1/dir2/");
  EXPECT_EQ(flags::Package("../dir1/dir2/a.cc"), "../dir1/dir2/");
  EXPECT_EQ(flags::Package("/dir1/dir2/a.cc"), "/dir1/dir2/");
  EXPECT_EQ(flags::Package("/dir1/dir2/../dir3/a.cc"), "/dir1/dir2/../dir3/");
}

}  // namespace
