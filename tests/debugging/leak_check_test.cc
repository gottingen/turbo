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

#include <string>

#include <gtest/gtest.h>
#include <turbo/base/config.h>
#include <turbo/debugging/leak_check.h>
#include <turbo/log/log.h>

namespace {

TEST(LeakCheckTest, IgnoreLeakSuppressesLeakedMemoryErrors) {
  if (!turbo::LeakCheckerIsActive()) {
    GTEST_SKIP() << "LeakChecker is not active";
  }
  auto foo = turbo::IgnoreLeak(new std::string("some ignored leaked string"));
  LOG(INFO) << "Ignoring leaked string " << foo;
}

TEST(LeakCheckTest, LeakCheckDisablerIgnoresLeak) {
  if (!turbo::LeakCheckerIsActive()) {
    GTEST_SKIP() << "LeakChecker is not active";
  }
  turbo::LeakCheckDisabler disabler;
  auto foo = new std::string("some string leaked while checks are disabled");
  LOG(INFO) << "Ignoring leaked string " << foo;
}

}  // namespace
