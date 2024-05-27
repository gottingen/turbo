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

#include <memory>

#include <gtest/gtest.h>
#include <turbo/debugging/leak_check.h>
#include <turbo/log/log.h>

namespace {

TEST(LeakCheckTest, LeakMemory) {
  // This test is expected to cause lsan failures on program exit. Therefore the
  // test will be run only by leak_check_test.sh, which will verify a
  // failed exit code.

  char* foo = strdup("lsan should complain about this leaked string");
  LOG(INFO) << "Should detect leaked string " << foo;
}

TEST(LeakCheckTest, LeakMemoryAfterDisablerScope) {
  // This test is expected to cause lsan failures on program exit. Therefore the
  // test will be run only by external_leak_check_test.sh, which will verify a
  // failed exit code.
  { turbo::LeakCheckDisabler disabler; }
  char* foo = strdup("lsan should also complain about this leaked string");
  LOG(INFO) << "Re-enabled leak detection.Should detect leaked string " << foo;
}

}  // namespace
