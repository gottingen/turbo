//
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

#include <turbo/flags/parse.h>

#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/base/internal/scoped_set_env.h>
#include <turbo/flags/config.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/internal/parse.h>
#include <turbo/flags/internal/usage.h>
#include <turbo/flags/reflection.h>
#include <turbo/log/log.h>
#include <turbo/strings/str_cat.h>
#include <turbo/strings/string_view.h>
#include <turbo/strings/substitute.h>
#include <turbo/container/span.h>

#ifdef _WIN32
#include <windows.h>
#endif

// Define 125 similar flags to test kMaxHints for flag suggestions.
#define FLAG_MULT(x) F3(x)
#define TEST_FLAG_HEADER FLAG_HEADER_

#define F(name) TURBO_FLAG(int, name, 0, "")

#define F1(name) \
  F(name##1);    \
  F(name##2);    \
  F(name##3);    \
  F(name##4);    \
  F(name##5)
/**/
#define F2(name) \
  F1(name##1);   \
  F1(name##2);   \
  F1(name##3);   \
  F1(name##4);   \
  F1(name##5)
/**/
#define F3(name) \
  F2(name##1);   \
  F2(name##2);   \
  F2(name##3);   \
  F2(name##4);   \
  F2(name##5)
/**/

FLAG_MULT(TEST_FLAG_HEADER);

namespace {

using turbo::base_internal::ScopedSetEnv;

struct UDT {
  UDT() = default;
  UDT(const UDT&) = default;
  UDT& operator=(const UDT&) = default;
  UDT(int v) : value(v) {}  // NOLINT

  int value;
};

bool turbo_parse_flag(std::string_view in, UDT* udt, std::string* err) {
  if (in == "A") {
    udt->value = 1;
    return true;
  }
  if (in == "AAA") {
    udt->value = 10;
    return true;
  }

  *err = "Use values A, AAA instead";
  return false;
}
std::string turbo_unparse_flag(const UDT& udt) {
  return udt.value == 1 ? "A" : "AAA";
}

std::string GetTestTmpDirEnvVar(const char* const env_var_name) {
#ifdef _WIN32
  char buf[MAX_PATH];
  auto get_res = GetEnvironmentVariableA(env_var_name, buf, sizeof(buf));
  if (get_res >= sizeof(buf) || get_res == 0) {
    return "";
  }

  return std::string(buf, get_res);
#else
  const char* val = ::getenv(env_var_name);
  if (val == nullptr) {
    return "";
  }

  return val;
#endif
}

const std::string& GetTestTempDir() {
  static std::string* temp_dir_name = []() -> std::string* {
    std::string* res = new std::string(GetTestTmpDirEnvVar("TEST_TMPDIR"));

    if (res->empty()) {
      *res = GetTestTmpDirEnvVar("TMPDIR");
    }

    if (res->empty()) {
#ifdef _WIN32
      char temp_path_buffer[MAX_PATH];

      auto len = GetTempPathA(MAX_PATH, temp_path_buffer);
      if (len < MAX_PATH && len != 0) {
        std::string temp_dir_name = temp_path_buffer;
        if (!turbo::ends_with(temp_dir_name, "\\")) {
          temp_dir_name.push_back('\\');
        }
        turbo::str_append(&temp_dir_name, "parse_test.", GetCurrentProcessId());
        if (CreateDirectoryA(temp_dir_name.c_str(), nullptr)) {
          *res = temp_dir_name;
        }
      }
#else
      char temp_dir_template[] = "/tmp/parse_test.XXXXXX";
      if (auto* unique_name = ::mkdtemp(temp_dir_template)) {
        *res = unique_name;
      }
#endif
    }

    if (res->empty()) {
      LOG(FATAL) << "Failed to make temporary directory for data files";
    }

#ifdef _WIN32
    *res += "\\";
#else
    *res += "/";
#endif

    return res;
  }();

  return *temp_dir_name;
}

struct FlagfileData {
  const std::string_view file_name;
  const turbo::span<const char* const> file_lines;
};

// clang-format off
constexpr const char* const ff1_data[] = {
    "# comment    ",
    "  # comment  ",
    "",
    "     ",
    "--int_flag=-1",
    "  --string_flag=q2w2  ",
    "  ##   ",
    "  --double_flag=0.1",
    "--bool_flag=Y  "
};

constexpr const char* const ff2_data[] = {
    "# Setting legacy flag",
    "--legacy_int=1111",
    "--legacy_bool",
    "--nobool_flag",
    "--legacy_str=aqsw",
    "--int_flag=100",
    "   ## ============="
};
// clang-format on

// Builds flags_file flag in the flagfile_flag buffer and returns it. This
// function also creates a temporary flags_file based on FlagfileData input.
// We create a flags_file in a temporary directory with the name specified in
// FlagfileData and populate it with lines specified in FlagfileData. If $0 is
// referenced in any of the lines in FlagfileData they are replaced with
// temporary directory location. This way we can test inclusion of one flags_file
// from another flags_file.
const char* GetFlagfileFlag(const std::vector<FlagfileData>& ffd,
                            std::string& flagfile_flag) {
  flagfile_flag = "--flags_file=";
  std::string_view separator;
  for (const auto& flagfile_data : ffd) {
    std::string flagfile_name =
        turbo::str_cat(GetTestTempDir(), flagfile_data.file_name);

    std::ofstream flagfile_out(flagfile_name);
    for (auto line : flagfile_data.file_lines) {
      flagfile_out << turbo::substitute(line, GetTestTempDir()) << "\n";
    }

    turbo::str_append(&flagfile_flag, separator, flagfile_name);
    separator = ",";
  }

  return flagfile_flag.c_str();
}

}  // namespace

TURBO_FLAG(int, int_flag, 1, "");
TURBO_FLAG(double, double_flag, 1.1, "");
TURBO_FLAG(std::string, string_flag, "a", "");
TURBO_FLAG(bool, bool_flag, false, "");
TURBO_FLAG(UDT, udt_flag, -1, "");
TURBO_RETIRED_FLAG(int, legacy_int, 1, "");
TURBO_RETIRED_FLAG(bool, legacy_bool, false, "");
TURBO_RETIRED_FLAG(std::string, legacy_str, "l", "");

namespace {

namespace flags = turbo::flags_internal;
using testing::AllOf;
using testing::ElementsAreArray;
using testing::HasSubstr;

class ParseTest : public testing::Test {
 public:
  ~ParseTest() override { flags::SetFlagsHelpMode(flags::HelpMode::kNone); }

  void SetUp() override {
#if TURBO_FLAGS_STRIP_NAMES
    GTEST_SKIP() << "This test requires flag names to be present";
#endif
  }

 private:
  turbo::FlagSaver flag_saver_;
};

// --------------------------------------------------------------------

template <int N>
flags::HelpMode InvokeParseTurboOnlyImpl(const char* (&in_argv)[N]) {
  std::vector<char*> positional_args;
  std::vector<turbo::UnrecognizedFlag> unrecognized_flags;

  return flags::parse_turbo_flags_only_impl(N, const_cast<char**>(in_argv),
                                         positional_args, unrecognized_flags,
                                         flags::UsageFlagsAction::kHandleUsage);
}

// --------------------------------------------------------------------

template <int N>
void InvokeParseTurboOnly(const char* (&in_argv)[N]) {
  std::vector<char*> positional_args;
  std::vector<turbo::UnrecognizedFlag> unrecognized_flags;

  turbo::parse_turbo_flags_only(2, const_cast<char**>(in_argv), positional_args,
                             unrecognized_flags);
}

// --------------------------------------------------------------------

template <int N>
std::vector<char*> InvokeParseCommandLineImpl(const char* (&in_argv)[N]) {
  return flags::ParseCommandLineImpl(
      N, const_cast<char**>(in_argv), flags::UsageFlagsAction::kHandleUsage,
      flags::OnUndefinedFlag::kAbortIfUndefined, std::cerr);
}

// --------------------------------------------------------------------

template <int N>
std::vector<char*> InvokeParse(const char* (&in_argv)[N]) {
  return turbo::parse_command_line(N, const_cast<char**>(in_argv));
}

// --------------------------------------------------------------------

template <int N>
void TestParse(const char* (&in_argv)[N], int int_flag_value,
               double double_flag_val, std::string_view string_flag_val,
               bool bool_flag_val, int exp_position_args = 0) {
  auto out_args = InvokeParse(in_argv);

  EXPECT_EQ(out_args.size(), 1 + exp_position_args);
  EXPECT_STREQ(out_args[0], "testbin");

  EXPECT_EQ(turbo::get_flag(FLAGS_int_flag), int_flag_value);
  EXPECT_NEAR(turbo::get_flag(FLAGS_double_flag), double_flag_val, 0.0001);
  EXPECT_EQ(turbo::get_flag(FLAGS_string_flag), string_flag_val);
  EXPECT_EQ(turbo::get_flag(FLAGS_bool_flag), bool_flag_val);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestEmptyArgv) {
  const char* in_argv[] = {"testbin"};

  auto out_args = InvokeParse(in_argv);

  EXPECT_EQ(out_args.size(), 1);
  EXPECT_STREQ(out_args[0], "testbin");
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestValidIntArg) {
  const char* in_args1[] = {
      "testbin",
      "--int_flag=10",
  };
  TestParse(in_args1, 10, 1.1, "a", false);

  const char* in_args2[] = {
      "testbin",
      "-int_flag=020",
  };
  TestParse(in_args2, 20, 1.1, "a", false);

  const char* in_args3[] = {
      "testbin",
      "--int_flag",
      "-30",
  };
  TestParse(in_args3, -30, 1.1, "a", false);

  const char* in_args4[] = {
      "testbin",
      "-int_flag",
      "0x21",
  };
  TestParse(in_args4, 33, 1.1, "a", false);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestValidDoubleArg) {
  const char* in_args1[] = {
      "testbin",
      "--double_flag=2.3",
  };
  TestParse(in_args1, 1, 2.3, "a", false);

  const char* in_args2[] = {
      "testbin",
      "--double_flag=0x1.2",
  };
  TestParse(in_args2, 1, 1.125, "a", false);

  const char* in_args3[] = {
      "testbin",
      "--double_flag",
      "99.7",
  };
  TestParse(in_args3, 1, 99.7, "a", false);

  const char* in_args4[] = {
      "testbin",
      "--double_flag",
      "0x20.1",
  };
  TestParse(in_args4, 1, 32.0625, "a", false);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestValidStringArg) {
  const char* in_args1[] = {
      "testbin",
      "--string_flag=aqswde",
  };
  TestParse(in_args1, 1, 1.1, "aqswde", false);

  const char* in_args2[] = {
      "testbin",
      "-string_flag=a=b=c",
  };
  TestParse(in_args2, 1, 1.1, "a=b=c", false);

  const char* in_args3[] = {
      "testbin",
      "--string_flag",
      "zaxscd",
  };
  TestParse(in_args3, 1, 1.1, "zaxscd", false);

  const char* in_args4[] = {
      "testbin",
      "-string_flag",
      "--int_flag",
  };
  TestParse(in_args4, 1, 1.1, "--int_flag", false);

  const char* in_args5[] = {
      "testbin",
      "--string_flag",
      "--no_a_flag=11",
  };
  TestParse(in_args5, 1, 1.1, "--no_a_flag=11", false);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestValidBoolArg) {
  const char* in_args1[] = {
      "testbin",
      "--bool_flag",
  };
  TestParse(in_args1, 1, 1.1, "a", true);

  const char* in_args2[] = {
      "testbin",
      "--nobool_flag",
  };
  TestParse(in_args2, 1, 1.1, "a", false);

  const char* in_args3[] = {
      "testbin",
      "--bool_flag=true",
  };
  TestParse(in_args3, 1, 1.1, "a", true);

  const char* in_args4[] = {
      "testbin",
      "-bool_flag=false",
  };
  TestParse(in_args4, 1, 1.1, "a", false);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestValidUDTArg) {
  const char* in_args1[] = {
      "testbin",
      "--udt_flag=A",
  };
  InvokeParse(in_args1);

  EXPECT_EQ(turbo::get_flag(FLAGS_udt_flag).value, 1);

  const char* in_args2[] = {"testbin", "--udt_flag", "AAA"};
  InvokeParse(in_args2);

  EXPECT_EQ(turbo::get_flag(FLAGS_udt_flag).value, 10);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestValidMultipleArg) {
  const char* in_args1[] = {
      "testbin",           "--bool_flag",       "--int_flag=2",
      "--double_flag=0.1", "--string_flag=asd",
  };
  TestParse(in_args1, 2, 0.1, "asd", true);

  const char* in_args2[] = {
      "testbin", "--string_flag=", "--nobool_flag", "--int_flag",
      "-011",    "--double_flag",  "-1e-2",
  };
  TestParse(in_args2, -11, -0.01, "", false);

  const char* in_args3[] = {
      "testbin",          "--int_flag",         "-0", "--string_flag", "\"\"",
      "--bool_flag=true", "--double_flag=1e18",
  };
  TestParse(in_args3, 0, 1e18, "\"\"", true);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestPositionalArgs) {
  const char* in_args1[] = {
      "testbin",
      "p1",
      "p2",
  };
  TestParse(in_args1, 1, 1.1, "a", false, 2);

  auto out_args1 = InvokeParse(in_args1);

  EXPECT_STREQ(out_args1[1], "p1");
  EXPECT_STREQ(out_args1[2], "p2");

  const char* in_args2[] = {
      "testbin",
      "--int_flag=2",
      "p1",
  };
  TestParse(in_args2, 2, 1.1, "a", false, 1);

  auto out_args2 = InvokeParse(in_args2);

  EXPECT_STREQ(out_args2[1], "p1");

  const char* in_args3[] = {"testbin", "p1",          "--int_flag=3",
                            "p2",      "--bool_flag", "true"};
  TestParse(in_args3, 3, 1.1, "a", true, 3);

  auto out_args3 = InvokeParse(in_args3);

  EXPECT_STREQ(out_args3[1], "p1");
  EXPECT_STREQ(out_args3[2], "p2");
  EXPECT_STREQ(out_args3[3], "true");

  const char* in_args4[] = {
      "testbin",
      "--",
      "p1",
      "p2",
  };
  TestParse(in_args4, 3, 1.1, "a", true, 2);

  auto out_args4 = InvokeParse(in_args4);

  EXPECT_STREQ(out_args4[1], "p1");
  EXPECT_STREQ(out_args4[2], "p2");

  const char* in_args5[] = {
      "testbin", "p1", "--int_flag=4", "--", "--bool_flag", "false", "p2",
  };
  TestParse(in_args5, 4, 1.1, "a", true, 4);

  auto out_args5 = InvokeParse(in_args5);

  EXPECT_STREQ(out_args5[1], "p1");
  EXPECT_STREQ(out_args5[2], "--bool_flag");
  EXPECT_STREQ(out_args5[3], "false");
  EXPECT_STREQ(out_args5[4], "p2");
}

// --------------------------------------------------------------------

using ParseDeathTest = ParseTest;

TEST_F(ParseDeathTest, TestUndefinedArg) {
  const char* in_args1[] = {
      "testbin",
      "--undefined_flag",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args1),
                            "Unknown command line flag 'undefined_flag'");

  const char* in_args2[] = {
      "testbin",
      "--noprefixed_flag",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args2),
                            "Unknown command line flag 'noprefixed_flag'");

  const char* in_args3[] = {
      "testbin",
      "--Int_flag=1",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args3),
                            "Unknown command line flag 'Int_flag'");
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestInvalidBoolFlagFormat) {
  const char* in_args1[] = {
      "testbin",
      "--bool_flag=",
  };
  EXPECT_DEATH_IF_SUPPORTED(
      InvokeParse(in_args1),
      "Missing the value after assignment for the boolean flag 'bool_flag'");

  const char* in_args2[] = {
      "testbin",
      "--nobool_flag=true",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args2),
               "Negative form with assignment is not valid for the boolean "
               "flag 'bool_flag'");
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestInvalidNonBoolFlagFormat) {
  const char* in_args1[] = {
      "testbin",
      "--nostring_flag",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args1),
               "Negative form is not valid for the flag 'string_flag'");

  const char* in_args2[] = {
      "testbin",
      "--int_flag",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args2),
               "Missing the value for the flag 'int_flag'");
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestInvalidUDTFlagFormat) {
  const char* in_args1[] = {
      "testbin",
      "--udt_flag=1",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args1),
               "Illegal value '1' specified for flag 'udt_flag'; Use values A, "
               "AAA instead");

  const char* in_args2[] = {
      "testbin",
      "--udt_flag",
      "AA",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args2),
               "Illegal value 'AA' specified for flag 'udt_flag'; Use values "
               "A, AAA instead");
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestFlagSuggestions) {
  const char* in_args1[] = {
      "testbin",
      "--legacy_boo",
  };
  EXPECT_DEATH_IF_SUPPORTED(
      InvokeParse(in_args1),
      "Unknown command line flag 'legacy_boo'. Did you mean: legacy_bool ?");

  const char* in_args2[] = {"testbin", "--foo", "--undef_ok=foo1"};
  EXPECT_DEATH_IF_SUPPORTED(
      InvokeParse(in_args2),
      "Unknown command line flag 'foo'. Did you mean: foo1 \\(undef_ok\\)?");

  const char* in_args3[] = {
      "testbin",
      "--nolegacy_ino",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args3),
                            "Unknown command line flag 'nolegacy_ino'. Did "
                            "you mean: nolegacy_bool, legacy_int ?");
}

// --------------------------------------------------------------------

TEST_F(ParseTest, GetHints) {
  EXPECT_THAT(turbo::flags_internal::GetMisspellingHints("legacy_boo"),
              testing::ContainerEq(std::vector<std::string>{"legacy_bool"}));
  EXPECT_THAT(turbo::flags_internal::GetMisspellingHints("nolegacy_itn"),
              testing::ContainerEq(std::vector<std::string>{"legacy_int"}));
  EXPECT_THAT(turbo::flags_internal::GetMisspellingHints("nolegacy_int1"),
              testing::ContainerEq(std::vector<std::string>{"legacy_int"}));
  EXPECT_THAT(turbo::flags_internal::GetMisspellingHints("nolegacy_int"),
              testing::ContainerEq(std::vector<std::string>{"legacy_int"}));
  EXPECT_THAT(turbo::flags_internal::GetMisspellingHints("nolegacy_ino"),
              testing::ContainerEq(
                  std::vector<std::string>{"nolegacy_bool", "legacy_int"}));
  EXPECT_THAT(
      turbo::flags_internal::GetMisspellingHints("FLAG_HEADER_000").size(), 100);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestLegacyFlags) {
  const char* in_args1[] = {
      "testbin",
      "--legacy_int=11",
  };
  TestParse(in_args1, 1, 1.1, "a", false);

  const char* in_args2[] = {
      "testbin",
      "--legacy_bool",
  };
  TestParse(in_args2, 1, 1.1, "a", false);

  const char* in_args3[] = {
      "testbin",       "--legacy_int", "22",           "--int_flag=2",
      "--legacy_bool", "true",         "--legacy_str", "--string_flag=qwe",
  };
  TestParse(in_args3, 2, 1.1, "a", false, 1);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestSimpleValidFlagfile) {
  std::string flagfile_flag;

  const char* in_args1[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff1", turbo::MakeConstSpan(ff1_data)}},
                      flagfile_flag),
  };
  TestParse(in_args1, -1, 0.1, "q2w2  ", true);

  const char* in_args2[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff2", turbo::MakeConstSpan(ff2_data)}},
                      flagfile_flag),
  };
  TestParse(in_args2, 100, 0.1, "q2w2  ", false);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestValidMultiFlagfile) {
  std::string flagfile_flag;

  const char* in_args1[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff2", turbo::MakeConstSpan(ff2_data)},
                       {"parse_test.ff1", turbo::MakeConstSpan(ff1_data)}},
                      flagfile_flag),
  };
  TestParse(in_args1, -1, 0.1, "q2w2  ", true);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestFlagfileMixedWithRegularFlags) {
  std::string flagfile_flag;

  const char* in_args1[] = {
      "testbin", "--int_flag=3",
      GetFlagfileFlag({{"parse_test.ff1", turbo::MakeConstSpan(ff1_data)}},
                      flagfile_flag),
      "-double_flag=0.2"};
  TestParse(in_args1, -1, 0.2, "q2w2  ", true);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestFlagfileInFlagfile) {
  std::string flagfile_flag;

  constexpr const char* const ff3_data[] = {
      "--flags_file=$0/parse_test.ff1",
      "--flags_file=$0/parse_test.ff2",
  };

  GetFlagfileFlag({{"parse_test.ff2", turbo::MakeConstSpan(ff2_data)},
                   {"parse_test.ff1", turbo::MakeConstSpan(ff1_data)}},
                      flagfile_flag);

  const char* in_args1[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff3", turbo::MakeConstSpan(ff3_data)}},
                      flagfile_flag),
  };
  TestParse(in_args1, 100, 0.1, "q2w2  ", false);
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestInvalidFlagfiles) {
  std::string flagfile_flag;

  constexpr const char* const ff4_data[] = {
    "--unknown_flag=10"
  };

  const char* in_args1[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff4",
                        turbo::MakeConstSpan(ff4_data)}}, flagfile_flag),
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args1),
               "Unknown command line flag 'unknown_flag'");

  constexpr const char* const ff5_data[] = {
    "--int_flag 10",
  };

  const char* in_args2[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff5",
                        turbo::MakeConstSpan(ff5_data)}}, flagfile_flag),
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args2),
               "Unknown command line flag 'int_flag 10'");

  constexpr const char* const ff6_data[] = {
      "--int_flag=10", "--", "arg1", "arg2", "arg3",
  };

  const char* in_args3[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff6", turbo::MakeConstSpan(ff6_data)}},
                      flagfile_flag),
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args3),
               "Flagfile can't contain position arguments or --");

  const char* in_args4[] = {
      "testbin",
      "--flags_file=invalid_flag_file",
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args4),
                            "Can't open flags_file invalid_flag_file");

  constexpr const char* const ff7_data[] = {
      "--int_flag=10",
      "*bin*",
      "--str_flag=aqsw",
  };

  const char* in_args5[] = {
      "testbin",
      GetFlagfileFlag({{"parse_test.ff7", turbo::MakeConstSpan(ff7_data)}},
                      flagfile_flag),
  };
  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args5),
               "Unexpected line in the flags_file .*: \\*bin\\*");
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestReadingRequiredFlagsFromEnv) {
  const char* in_args1[] = {"testbin",
                            "--from_env=int_flag,bool_flag,string_flag"};

  ScopedSetEnv set_int_flag("FLAGS_int_flag", "33");
  ScopedSetEnv set_bool_flag("FLAGS_bool_flag", "True");
  ScopedSetEnv set_string_flag("FLAGS_string_flag", "AQ12");

  TestParse(in_args1, 33, 1.1, "AQ12", true);
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestReadingUnsetRequiredFlagsFromEnv) {
  const char* in_args1[] = {"testbin", "--from_env=int_flag"};

  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args1),
               "FLAGS_int_flag not found in environment");
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestRecursiveFlagsFromEnv) {
  const char* in_args1[] = {"testbin", "--from_env=try_from_env"};

  ScopedSetEnv set_try_from_env("FLAGS_try_from_env", "int_flag");

  EXPECT_DEATH_IF_SUPPORTED(InvokeParse(in_args1),
                            "Infinite recursion on flag try_from_env");
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestReadingOptionalFlagsFromEnv) {
  const char* in_args1[] = {
      "testbin", "--try_from_env=int_flag,bool_flag,string_flag,other_flag"};

  ScopedSetEnv set_int_flag("FLAGS_int_flag", "17");
  ScopedSetEnv set_bool_flag("FLAGS_bool_flag", "Y");

  TestParse(in_args1, 17, 1.1, "a", true);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestReadingFlagsFromEnvMoxedWithRegularFlags) {
  const char* in_args1[] = {
      "testbin",
      "--bool_flag=T",
      "--try_from_env=int_flag,bool_flag",
      "--int_flag=-21",
  };

  ScopedSetEnv set_int_flag("FLAGS_int_flag", "-15");
  ScopedSetEnv set_bool_flag("FLAGS_bool_flag", "F");

  TestParse(in_args1, -21, 1.1, "a", false);
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestSimpleHelpFlagHandling) {
  const char* in_args1[] = {
      "testbin",
      "--help",
  };

  EXPECT_EQ(InvokeParseTurboOnlyImpl(in_args1), flags::HelpMode::kImportant);
  EXPECT_EXIT(InvokeParse(in_args1), testing::ExitedWithCode(1), "");

  const char* in_args2[] = {
      "testbin",
      "--help",
      "--int_flag=3",
  };

  EXPECT_EQ(InvokeParseTurboOnlyImpl(in_args2), flags::HelpMode::kImportant);
  EXPECT_EQ(turbo::get_flag(FLAGS_int_flag), 3);

  const char* in_args3[] = {"testbin", "--help", "some_positional_arg"};

  EXPECT_EQ(InvokeParseTurboOnlyImpl(in_args3), flags::HelpMode::kImportant);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestSubstringHelpFlagHandling) {
  const char* in_args1[] = {
      "testbin",
      "--help=abcd",
  };

  EXPECT_EQ(InvokeParseTurboOnlyImpl(in_args1), flags::HelpMode::kMatch);
  EXPECT_EQ(flags::GetFlagsHelpMatchSubstr(), "abcd");
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, TestVersionHandling) {
  const char* in_args1[] = {
      "testbin",
      "--version",
  };

  EXPECT_EQ(InvokeParseTurboOnlyImpl(in_args1), flags::HelpMode::kVersion);
}

// --------------------------------------------------------------------

TEST_F(ParseTest, TestCheckArgsHandling) {
  const char* in_args1[] = {"testbin", "--only_check_args", "--int_flag=211"};

  EXPECT_EQ(InvokeParseTurboOnlyImpl(in_args1), flags::HelpMode::kOnlyCheckArgs);
  EXPECT_EXIT(InvokeParseTurboOnly(in_args1), testing::ExitedWithCode(0), "");
  EXPECT_EXIT(InvokeParse(in_args1), testing::ExitedWithCode(0), "");

  const char* in_args2[] = {"testbin", "--only_check_args", "--unknown_flag=a"};

  EXPECT_EQ(InvokeParseTurboOnlyImpl(in_args2), flags::HelpMode::kOnlyCheckArgs);
  EXPECT_EXIT(InvokeParseTurboOnly(in_args2), testing::ExitedWithCode(0), "");
  EXPECT_EXIT(InvokeParse(in_args2), testing::ExitedWithCode(1), "");
}

// --------------------------------------------------------------------

TEST_F(ParseTest, WasPresentOnCommandLine) {
  const char* in_args1[] = {
      "testbin",        "arg1", "--bool_flag",
      "--int_flag=211", "arg2", "--double_flag=1.1",
      "--string_flag",  "asd",  "--",
      "--some_flag",    "arg4",
  };

  InvokeParse(in_args1);

  EXPECT_TRUE(flags::WasPresentOnCommandLine("bool_flag"));
  EXPECT_TRUE(flags::WasPresentOnCommandLine("int_flag"));
  EXPECT_TRUE(flags::WasPresentOnCommandLine("double_flag"));
  EXPECT_TRUE(flags::WasPresentOnCommandLine("string_flag"));
  EXPECT_FALSE(flags::WasPresentOnCommandLine("some_flag"));
  EXPECT_FALSE(flags::WasPresentOnCommandLine("another_flag"));
}

// --------------------------------------------------------------------

TEST_F(ParseTest, ParseTurboFlagsOnlySuccess) {
  const char* in_args[] = {
      "testbin",
      "arg1",
      "--bool_flag",
      "--int_flag=211",
      "arg2",
      "--double_flag=1.1",
      "--undef_flag1",
      "--undef_flag2=123",
      "--string_flag",
      "asd",
      "--",
      "--some_flag",
      "arg4",
  };

  std::vector<char*> positional_args;
  std::vector<turbo::UnrecognizedFlag> unrecognized_flags;

  turbo::parse_turbo_flags_only(13, const_cast<char**>(in_args), positional_args,
                             unrecognized_flags);
  EXPECT_THAT(positional_args,
              ElementsAreArray(
                  {std::string_view("testbin"), std::string_view("arg1"),
                   std::string_view("arg2"), std::string_view("--some_flag"),
                   std::string_view("arg4")}));
  EXPECT_THAT(unrecognized_flags,
              ElementsAreArray(
                  {turbo::UnrecognizedFlag(turbo::UnrecognizedFlag::kFromArgv,
                                          "undef_flag1"),
                   turbo::UnrecognizedFlag(turbo::UnrecognizedFlag::kFromArgv,
                                          "undef_flag2")}));
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, ParseTurboFlagsOnlyFailure) {
  const char* in_args[] = {
      "testbin",
      "--int_flag=21.1",
  };

  EXPECT_DEATH_IF_SUPPORTED(
      InvokeParseTurboOnly(in_args),
      "Illegal value '21.1' specified for flag 'int_flag'");
}

// --------------------------------------------------------------------

TEST_F(ParseTest, UndefOkFlagsAreIgnored) {
  const char* in_args[] = {
      "testbin",           "--undef_flag1",
      "--undef_flag2=123", "--undef_ok=undef_flag2",
      "--undef_flag3",     "value",
  };

  std::vector<char*> positional_args;
  std::vector<turbo::UnrecognizedFlag> unrecognized_flags;

  turbo::parse_turbo_flags_only(6, const_cast<char**>(in_args), positional_args,
                             unrecognized_flags);
  EXPECT_THAT(positional_args, ElementsAreArray({std::string_view("testbin"),
                                                 std::string_view("value")}));
  EXPECT_THAT(unrecognized_flags,
              ElementsAreArray(
                  {turbo::UnrecognizedFlag(turbo::UnrecognizedFlag::kFromArgv,
                                          "undef_flag1"),
                   turbo::UnrecognizedFlag(turbo::UnrecognizedFlag::kFromArgv,
                                          "undef_flag3")}));
}

// --------------------------------------------------------------------

TEST_F(ParseTest, AllUndefOkFlagsAreIgnored) {
  const char* in_args[] = {
      "testbin",
      "--undef_flag1",
      "--undef_flag2=123",
      "--undef_ok=undef_flag2,undef_flag1,undef_flag3",
      "--undef_flag3",
      "value",
      "--",
      "--undef_flag4",
  };

  std::vector<char*> positional_args;
  std::vector<turbo::UnrecognizedFlag> unrecognized_flags;

  turbo::parse_turbo_flags_only(8, const_cast<char**>(in_args), positional_args,
                             unrecognized_flags);
  EXPECT_THAT(positional_args,
              ElementsAreArray({std::string_view("testbin"),
                                std::string_view("value"),
                                std::string_view("--undef_flag4")}));
  EXPECT_THAT(unrecognized_flags, testing::IsEmpty());
}

// --------------------------------------------------------------------

TEST_F(ParseDeathTest, ExitOnUnrecognizedFlagPrintsHelp) {
  const char* in_args[] = {
      "testbin",
      "--undef_flag1",
      "--help=int_flag",
  };

  EXPECT_EXIT(InvokeParseCommandLineImpl(in_args), testing::ExitedWithCode(1),
              AllOf(HasSubstr("Unknown command line flag 'undef_flag1'"),
                    HasSubstr("Try --helpfull to get a list of all flags")));
}

// --------------------------------------------------------------------

}  // namespace
