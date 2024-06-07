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

#include <turbo/flags/internal/program_name.h>

#include <string>

#include <gtest/gtest.h>
#include <turbo/strings/match.h>
#include <turbo/strings/string_view.h>

namespace {

namespace flags = turbo::flags_internal;

TEST(FlagsPathUtilTest, TestProgamNameInterfaces) {
  flags::SetProgramInvocationName("turbo/flags/program_name_test");
  std::string program_name = flags::ProgramInvocationName();
  for (char& c : program_name)
    if (c == '\\') c = '/';

#if !defined(__wasm__) && !defined(__asmjs__)
  const std::string expect_name = "turbo/flags/program_name_test";
  const std::string expect_basename = "program_name_test";
#else
  // For targets that generate javascript or webassembly the invocation name
  // has the special value below.
  const std::string expect_name = "this.program";
  const std::string expect_basename = "this.program";
#endif

  EXPECT_TRUE(turbo::ends_with(program_name, expect_name)) << program_name;
  EXPECT_EQ(flags::ShortProgramInvocationName(), expect_basename);

  flags::SetProgramInvocationName("a/my_test");

  EXPECT_EQ(flags::ProgramInvocationName(), "a/my_test");
  EXPECT_EQ(flags::ShortProgramInvocationName(), "my_test");

  std::string_view not_null_terminated("turbo/aaa/bbb");
  not_null_terminated = not_null_terminated.substr(1, 10);

  flags::SetProgramInvocationName(not_null_terminated);

  EXPECT_EQ(flags::ProgramInvocationName(), "urbo/aaa/b")<< flags::ProgramInvocationName();
  EXPECT_EQ(flags::ShortProgramInvocationName(), "b")<< flags::ShortProgramInvocationName();
}

}  // namespace
