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

#include <turbo/base/internal/errno_saver.h>

#include <cerrno>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/internal/strerror.h>

namespace {
using ::testing::Eq;

struct ErrnoPrinter {
  int no;
};
std::ostream &operator<<(std::ostream &os, ErrnoPrinter ep) {
  return os << turbo::base_internal::StrError(ep.no) << " [" << ep.no << "]";
}
bool operator==(ErrnoPrinter one, ErrnoPrinter two) { return one.no == two.no; }

TEST(ErrnoSaverTest, Works) {
  errno = EDOM;
  {
    turbo::base_internal::ErrnoSaver errno_saver;
    EXPECT_THAT(ErrnoPrinter{errno}, Eq(ErrnoPrinter{EDOM}));
    errno = ERANGE;
    EXPECT_THAT(ErrnoPrinter{errno}, Eq(ErrnoPrinter{ERANGE}));
    EXPECT_THAT(ErrnoPrinter{errno_saver()}, Eq(ErrnoPrinter{EDOM}));
  }
  EXPECT_THAT(ErrnoPrinter{errno}, Eq(ErrnoPrinter{EDOM}));
}
}  // namespace
