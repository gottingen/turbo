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

#include <turbo/flags/internal/usage.h>

#include <stdint.h>

#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <turbo/flags/config.h>
#include <turbo/flags/flag.h>
#include <turbo/flags/internal/parse.h>
#include <turbo/flags/internal/program_name.h>
#include <turbo/flags/reflection.h>
#include <turbo/flags/usage.h>
#include <turbo/flags/usage_config.h>
#include <turbo/strings/match.h>
#include <turbo/strings/string_view.h>

TURBO_FLAG(int, usage_reporting_test_flag_01, 101,
          "usage_reporting_test_flag_01 help message");
TURBO_FLAG(bool, usage_reporting_test_flag_02, false,
          "usage_reporting_test_flag_02 help message");
TURBO_FLAG(double, usage_reporting_test_flag_03, 1.03,
          "usage_reporting_test_flag_03 help message");
TURBO_FLAG(int64_t, usage_reporting_test_flag_04, 1000000000000004L,
          "usage_reporting_test_flag_04 help message");
TURBO_FLAG(std::string, usage_reporting_test_flag_07, "\r\n\f\v\a\b\t ",
          "usage_reporting_test_flag_07 help \r\n\f\v\a\b\t ");

static const char kTestUsageMessage[] = "Custom usage message";

struct UDT {
  UDT() = default;
  UDT(const UDT&) = default;
  UDT& operator=(const UDT&) = default;
};
static bool turbo_parse_flag(turbo::string_view, UDT*, std::string*) {
  return true;
}
static std::string turbo_unparse_flag(const UDT&) { return "UDT{}"; }

TURBO_FLAG(UDT, usage_reporting_test_flag_05, {},
          "usage_reporting_test_flag_05 help message");

TURBO_FLAG(
    std::string, usage_reporting_test_flag_06, {},
    "usage_reporting_test_flag_06 help message.\n"
    "\n"
    "Some more help.\n"
    "Even more long long long long long long long long long long long long "
    "help message.");

namespace {

namespace flags = turbo::flags_internal;

static std::string NormalizeFileName(turbo::string_view fname) {
#ifdef _WIN32
  std::string normalized(fname);
  std::replace(normalized.begin(), normalized.end(), '\\', '/');
  fname = normalized;
#endif

  auto turbo_pos = fname.rfind("turbo/");
  if (turbo_pos != turbo::string_view::npos) {
    fname = fname.substr(turbo_pos);
  }
  return std::string(fname);
}

class UsageReportingTest : public testing::Test {
 protected:
  UsageReportingTest() {
    // Install default config for the use on this unit test.
    // Binary may install a custom config before tests are run.
    turbo::FlagsUsageConfig default_config;
    default_config.normalize_filename = &NormalizeFileName;
    turbo::SetFlagsUsageConfig(default_config);
  }
  ~UsageReportingTest() override {
    flags::SetFlagsHelpMode(flags::HelpMode::kNone);
    flags::SetFlagsHelpMatchSubstr("");
    flags::SetFlagsHelpFormat(flags::HelpFormat::kHumanReadable);
  }
  void SetUp() override {
#if TURBO_FLAGS_STRIP_NAMES
    GTEST_SKIP() << "This test requires flag names to be present";
#endif
  }

 private:
  turbo::FlagSaver flag_saver_;
};

// --------------------------------------------------------------------

using UsageReportingDeathTest = UsageReportingTest;

TEST_F(UsageReportingDeathTest, TestSetProgramUsageMessage) {
#if !defined(GTEST_HAS_ABSL) || !GTEST_HAS_ABSL
  // Check for kTestUsageMessage set in main() below.
  EXPECT_EQ(turbo::ProgramUsageMessage(), kTestUsageMessage);
#else
  // Check for part of the usage message set by GoogleTest.
  EXPECT_THAT(turbo::ProgramUsageMessage(),
              ::testing::HasSubstr(
                  "This program contains tests written using Google Test"));
#endif

  EXPECT_DEATH_IF_SUPPORTED(
      turbo::SetProgramUsageMessage("custom usage message"),
      ::testing::HasSubstr("SetProgramUsageMessage() called twice"));
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestFlagHelpHRF_on_flag_01) {
  const auto* flag = turbo::FindCommandLineFlag("usage_reporting_test_flag_01");
  std::stringstream test_buf;

  flags::FlagHelp(test_buf, *flag, flags::HelpFormat::kHumanReadable);
  EXPECT_EQ(
      test_buf.str(),
      R"(    --usage_reporting_test_flag_01 (usage_reporting_test_flag_01 help message);
      default: 101;
)");
}

TEST_F(UsageReportingTest, TestFlagHelpHRF_on_flag_02) {
  const auto* flag = turbo::FindCommandLineFlag("usage_reporting_test_flag_02");
  std::stringstream test_buf;

  flags::FlagHelp(test_buf, *flag, flags::HelpFormat::kHumanReadable);
  EXPECT_EQ(
      test_buf.str(),
      R"(    --usage_reporting_test_flag_02 (usage_reporting_test_flag_02 help message);
      default: false;
)");
}

TEST_F(UsageReportingTest, TestFlagHelpHRF_on_flag_03) {
  const auto* flag = turbo::FindCommandLineFlag("usage_reporting_test_flag_03");
  std::stringstream test_buf;

  flags::FlagHelp(test_buf, *flag, flags::HelpFormat::kHumanReadable);
  EXPECT_EQ(
      test_buf.str(),
      R"(    --usage_reporting_test_flag_03 (usage_reporting_test_flag_03 help message);
      default: 1.03;
)");
}

TEST_F(UsageReportingTest, TestFlagHelpHRF_on_flag_04) {
  const auto* flag = turbo::FindCommandLineFlag("usage_reporting_test_flag_04");
  std::stringstream test_buf;

  flags::FlagHelp(test_buf, *flag, flags::HelpFormat::kHumanReadable);
  EXPECT_EQ(
      test_buf.str(),
      R"(    --usage_reporting_test_flag_04 (usage_reporting_test_flag_04 help message);
      default: 1000000000000004;
)");
}

TEST_F(UsageReportingTest, TestFlagHelpHRF_on_flag_05) {
  const auto* flag = turbo::FindCommandLineFlag("usage_reporting_test_flag_05");
  std::stringstream test_buf;

  flags::FlagHelp(test_buf, *flag, flags::HelpFormat::kHumanReadable);
  EXPECT_EQ(
      test_buf.str(),
      R"(    --usage_reporting_test_flag_05 (usage_reporting_test_flag_05 help message);
      default: UDT{};
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestFlagsHelpHRF) {
  std::string usage_test_flags_out =
      R"(usage_test: Custom usage message

  Flags from turbo/tests/flags/usage_test.cc:
    --usage_reporting_test_flag_01 (usage_reporting_test_flag_01 help message);
      default: 101;
    --usage_reporting_test_flag_02 (usage_reporting_test_flag_02 help message);
      default: false;
    --usage_reporting_test_flag_03 (usage_reporting_test_flag_03 help message);
      default: 1.03;
    --usage_reporting_test_flag_04 (usage_reporting_test_flag_04 help message);
      default: 1000000000000004;
    --usage_reporting_test_flag_05 (usage_reporting_test_flag_05 help message);
      default: UDT{};
    --usage_reporting_test_flag_06 (usage_reporting_test_flag_06 help message.

      Some more help.
      Even more long long long long long long long long long long long long help
      message.); default: "";)"

      "\n    --usage_reporting_test_flag_07 (usage_reporting_test_flag_07 "
      "help\n\n      \f\v\a\b ); default: \"\r\n\f\v\a\b\t \";\n"

      R"(
Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)";

  std::stringstream test_buf_01;
  flags::FlagsHelp(test_buf_01, "usage_test.cc",
                   flags::HelpFormat::kHumanReadable, kTestUsageMessage);
  EXPECT_EQ(test_buf_01.str(), usage_test_flags_out);

  std::stringstream test_buf_02;
  flags::FlagsHelp(test_buf_02, "turbo/tests/flags/usage_test.cc",
                   flags::HelpFormat::kHumanReadable, kTestUsageMessage);
  EXPECT_EQ(test_buf_02.str(), usage_test_flags_out)<< test_buf_02.str();

  std::stringstream test_buf_03;
  flags::FlagsHelp(test_buf_03, "usage_test", flags::HelpFormat::kHumanReadable,
                   kTestUsageMessage);
  EXPECT_EQ(test_buf_03.str(), usage_test_flags_out);

  std::stringstream test_buf_04;
  flags::FlagsHelp(test_buf_04, "flags/invalid_file_name.cc",
                   flags::HelpFormat::kHumanReadable, kTestUsageMessage);
  EXPECT_EQ(test_buf_04.str(),
            R"(usage_test: Custom usage message

No flags matched.

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");

  std::stringstream test_buf_05;
  flags::FlagsHelp(test_buf_05, "", flags::HelpFormat::kHumanReadable,
                   kTestUsageMessage);
  std::string test_out = test_buf_05.str();
  turbo::string_view test_out_str(test_out);
  EXPECT_TRUE(
      turbo::starts_with(test_out_str, "usage_test: Custom usage message"));
  EXPECT_TRUE(turbo::str_contains(
      test_out_str, "Flags from turbo/tests/flags/usage_test.cc:"));
  EXPECT_TRUE(
      turbo::str_contains(test_out_str, "-usage_reporting_test_flag_01 "));
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestNoUsageFlags) {
  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kNone);
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_helpshort) {
  flags::SetFlagsHelpMode(flags::HelpMode::kShort);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kShort);
  EXPECT_EQ(
      test_buf.str(),
      R"(usage_test: Custom usage message

  Flags from turbo/tests/flags/usage_test.cc:
    --usage_reporting_test_flag_01 (usage_reporting_test_flag_01 help message);
      default: 101;
    --usage_reporting_test_flag_02 (usage_reporting_test_flag_02 help message);
      default: false;
    --usage_reporting_test_flag_03 (usage_reporting_test_flag_03 help message);
      default: 1.03;
    --usage_reporting_test_flag_04 (usage_reporting_test_flag_04 help message);
      default: 1000000000000004;
    --usage_reporting_test_flag_05 (usage_reporting_test_flag_05 help message);
      default: UDT{};
    --usage_reporting_test_flag_06 (usage_reporting_test_flag_06 help message.

      Some more help.
      Even more long long long long long long long long long long long long help
      message.); default: "";)"

      "\n    --usage_reporting_test_flag_07 (usage_reporting_test_flag_07 "
      "help\n\n      \f\v\a\b ); default: \"\r\n\f\v\a\b\t \";\n"

      R"(
Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_help_simple) {
  flags::SetFlagsHelpMode(flags::HelpMode::kImportant);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kImportant);
  EXPECT_EQ(
      test_buf.str(),
      R"(usage_test: Custom usage message

  Flags from turbo/tests/flags/usage_test.cc:
    --usage_reporting_test_flag_01 (usage_reporting_test_flag_01 help message);
      default: 101;
    --usage_reporting_test_flag_02 (usage_reporting_test_flag_02 help message);
      default: false;
    --usage_reporting_test_flag_03 (usage_reporting_test_flag_03 help message);
      default: 1.03;
    --usage_reporting_test_flag_04 (usage_reporting_test_flag_04 help message);
      default: 1000000000000004;
    --usage_reporting_test_flag_05 (usage_reporting_test_flag_05 help message);
      default: UDT{};
    --usage_reporting_test_flag_06 (usage_reporting_test_flag_06 help message.

      Some more help.
      Even more long long long long long long long long long long long long help
      message.); default: "";)"

      "\n    --usage_reporting_test_flag_07 (usage_reporting_test_flag_07 "
      "help\n\n      \f\v\a\b ); default: \"\r\n\f\v\a\b\t \";\n"

      R"(
Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_help_one_flag) {
  flags::SetFlagsHelpMode(flags::HelpMode::kMatch);
  flags::SetFlagsHelpMatchSubstr("usage_reporting_test_flag_06");

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kMatch);
  EXPECT_EQ(test_buf.str(),
            R"(usage_test: Custom usage message

  Flags from turbo/tests/flags/usage_test.cc:
    --usage_reporting_test_flag_06 (usage_reporting_test_flag_06 help message.

      Some more help.
      Even more long long long long long long long long long long long long help
      message.); default: "";

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_help_multiple_flag) {
  flags::SetFlagsHelpMode(flags::HelpMode::kMatch);
  flags::SetFlagsHelpMatchSubstr("test_flag");

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kMatch);
  EXPECT_EQ(
      test_buf.str(),
      R"(usage_test: Custom usage message

  Flags from turbo/tests/flags/usage_test.cc:
    --usage_reporting_test_flag_01 (usage_reporting_test_flag_01 help message);
      default: 101;
    --usage_reporting_test_flag_02 (usage_reporting_test_flag_02 help message);
      default: false;
    --usage_reporting_test_flag_03 (usage_reporting_test_flag_03 help message);
      default: 1.03;
    --usage_reporting_test_flag_04 (usage_reporting_test_flag_04 help message);
      default: 1000000000000004;
    --usage_reporting_test_flag_05 (usage_reporting_test_flag_05 help message);
      default: UDT{};
    --usage_reporting_test_flag_06 (usage_reporting_test_flag_06 help message.

      Some more help.
      Even more long long long long long long long long long long long long help
      message.); default: "";)"

      "\n    --usage_reporting_test_flag_07 (usage_reporting_test_flag_07 "
      "help\n\n      \f\v\a\b ); default: \"\r\n\f\v\a\b\t \";\n"

      R"(
Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_helppackage) {
  flags::SetFlagsHelpMode(flags::HelpMode::kPackage);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kPackage);
  EXPECT_EQ(
      test_buf.str(),
      R"(usage_test: Custom usage message

  Flags from turbo/tests/flags/usage_test.cc:
    --usage_reporting_test_flag_01 (usage_reporting_test_flag_01 help message);
      default: 101;
    --usage_reporting_test_flag_02 (usage_reporting_test_flag_02 help message);
      default: false;
    --usage_reporting_test_flag_03 (usage_reporting_test_flag_03 help message);
      default: 1.03;
    --usage_reporting_test_flag_04 (usage_reporting_test_flag_04 help message);
      default: 1000000000000004;
    --usage_reporting_test_flag_05 (usage_reporting_test_flag_05 help message);
      default: UDT{};
    --usage_reporting_test_flag_06 (usage_reporting_test_flag_06 help message.

      Some more help.
      Even more long long long long long long long long long long long long help
      message.); default: "";)"

      "\n    --usage_reporting_test_flag_07 (usage_reporting_test_flag_07 "
      "help\n\n      \f\v\a\b ); default: \"\r\n\f\v\a\b\t \";\n"

      R"(
Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_version) {
  flags::SetFlagsHelpMode(flags::HelpMode::kVersion);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kVersion);
#ifndef NDEBUG
  EXPECT_EQ(test_buf.str(), "usage_test\nDebug build (NDEBUG not #defined)\n");
#else
  EXPECT_EQ(test_buf.str(), "usage_test\n");
#endif
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_only_check_args) {
  flags::SetFlagsHelpMode(flags::HelpMode::kOnlyCheckArgs);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage),
            flags::HelpMode::kOnlyCheckArgs);
  EXPECT_EQ(test_buf.str(), "");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_helpon) {
  flags::SetFlagsHelpMode(flags::HelpMode::kMatch);
  flags::SetFlagsHelpMatchSubstr("/bla-bla.");

  std::stringstream test_buf_01;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf_01, kTestUsageMessage),
            flags::HelpMode::kMatch);
  EXPECT_EQ(test_buf_01.str(),
            R"(usage_test: Custom usage message

No flags matched.

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");

  flags::SetFlagsHelpMatchSubstr("/usage_test.");

  std::stringstream test_buf_02;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf_02, kTestUsageMessage),
            flags::HelpMode::kMatch);
  EXPECT_EQ(
      test_buf_02.str(),
      R"(usage_test: Custom usage message

  Flags from turbo/tests/flags/usage_test.cc:
    --usage_reporting_test_flag_01 (usage_reporting_test_flag_01 help message);
      default: 101;
    --usage_reporting_test_flag_02 (usage_reporting_test_flag_02 help message);
      default: false;
    --usage_reporting_test_flag_03 (usage_reporting_test_flag_03 help message);
      default: 1.03;
    --usage_reporting_test_flag_04 (usage_reporting_test_flag_04 help message);
      default: 1000000000000004;
    --usage_reporting_test_flag_05 (usage_reporting_test_flag_05 help message);
      default: UDT{};
    --usage_reporting_test_flag_06 (usage_reporting_test_flag_06 help message.

      Some more help.
      Even more long long long long long long long long long long long long help
      message.); default: "";)"

      "\n    --usage_reporting_test_flag_07 (usage_reporting_test_flag_07 "
      "help\n\n      \f\v\a\b ); default: \"\r\n\f\v\a\b\t \";\n"

      R"(
Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

}  // namespace

int main(int argc, char* argv[]) {
  (void)turbo::GetFlag(FLAGS_undefok);  // Force linking of parse.cc
  flags::SetProgramInvocationName("usage_test");
#if !defined(GTEST_HAS_ABSL) || !GTEST_HAS_ABSL
  // GoogleTest calls turbo::SetProgramUsageMessage() already.
  turbo::SetProgramUsageMessage(kTestUsageMessage);
#endif
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
