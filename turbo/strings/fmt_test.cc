// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "turbo/strings/fmt/format.h"
#include "turbo/strings/inlined_string.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

struct pair {
  int a;
  int b;
};

namespace fmt {
template <> struct formatter<pair> :  formatter<fmt::string_view> {
  auto format(const pair &v, format_context &ctx) -> decltype(ctx.out()) {
    return format_to(ctx.out(), "({},{})", v.a, v.b);
  }
};
} // namespace fmt

TEST(fmt, try) {
  auto s = fmt::format("{}", 42);
  EXPECT_EQ(s, "42");
  auto p = fmt::format("{}",pair{1,2});
  EXPECT_EQ(p, "(1,2)");

  turbo::inlined_string is = "abc";
  auto ip = fmt::format("{}",is);
  EXPECT_EQ(ip, is);

}