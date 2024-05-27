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

#include <turbo/base/config.h>

#include <cstdint>

#include <gtest/gtest.h>
#include <turbo/synchronization/internal/thread_pool.h>

namespace {

TEST(ConfigTest, Endianness) {
  union {
    uint32_t value;
    uint8_t data[sizeof(uint32_t)];
  } number;
  number.data[0] = 0x00;
  number.data[1] = 0x01;
  number.data[2] = 0x02;
  number.data[3] = 0x03;
#if defined(TURBO_IS_LITTLE_ENDIAN) && defined(TURBO_IS_BIG_ENDIAN)
#error Both TURBO_IS_LITTLE_ENDIAN and TURBO_IS_BIG_ENDIAN are defined
#elif defined(TURBO_IS_LITTLE_ENDIAN)
  EXPECT_EQ(UINT32_C(0x03020100), number.value);
#elif defined(TURBO_IS_BIG_ENDIAN)
  EXPECT_EQ(UINT32_C(0x00010203), number.value);
#else
#error Unknown endianness
#endif
}

#if defined(TURBO_HAVE_THREAD_LOCAL)
TEST(ConfigTest, ThreadLocal) {
  static thread_local int mine_mine_mine = 16;
  EXPECT_EQ(16, mine_mine_mine);
  {
    turbo::synchronization_internal::ThreadPool pool(1);
    pool.Schedule([&] {
      EXPECT_EQ(16, mine_mine_mine);
      mine_mine_mine = 32;
      EXPECT_EQ(32, mine_mine_mine);
    });
  }
  EXPECT_EQ(16, mine_mine_mine);
}
#endif

}  // namespace
