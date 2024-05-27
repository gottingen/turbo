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

#include <turbo/strings/internal/str_format/output.h>

#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/strings/cord.h>

namespace turbo {
TURBO_NAMESPACE_BEGIN
namespace {

TEST(InvokeFlush, String) {
  std::string str = "ABC";
  str_format_internal::InvokeFlush(&str, "DEF");
  EXPECT_EQ(str, "ABCDEF");
}

TEST(InvokeFlush, Stream) {
  std::stringstream str;
  str << "ABC";
  str_format_internal::InvokeFlush(&str, "DEF");
  EXPECT_EQ(str.str(), "ABCDEF");
}

TEST(InvokeFlush, Cord) {
  turbo::Cord str("ABC");
  str_format_internal::InvokeFlush(&str, "DEF");
  EXPECT_EQ(str, "ABCDEF");
}

TEST(BufferRawSink, Limits) {
  char buf[16];
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World237xx");
  }
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World237237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World2372x");
  }
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World");
    str_format_internal::InvokeFlush(&bufsink, "237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World237xx");
  }
  {
    std::fill(std::begin(buf), std::end(buf), 'x');
    str_format_internal::BufferRawSink bufsink(buf, sizeof(buf) - 1);
    str_format_internal::InvokeFlush(&bufsink, "Hello World");
    str_format_internal::InvokeFlush(&bufsink, "237237");
    EXPECT_EQ(std::string(buf, sizeof(buf)), "Hello World2372x");
  }
}

}  // namespace
TURBO_NAMESPACE_END
}  // namespace turbo
