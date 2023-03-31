//
//  Copyright 2019 The Turbo Authors.
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

#include "turbo/flags/internal/usage.h"

#include <stdint.h>

#include <sstream>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/flags/flag.h"
#include "turbo/flags/internal/parse.h"
#include "turbo/flags/internal/path_util.h"
#include "turbo/flags/internal/program_name.h"
#include "turbo/flags/reflection.h"
#include "turbo/flags/usage.h"
#include "turbo/flags/usage_config.h"
#include "turbo/strings/match.h"
#include "turbo/strings/string_piece.h"

TURBO_FLAG(int, usage_reporting_test_flag_01, 101,
          "usage_reporting_test_flag_01 help message");
TURBO_FLAG(bool, usage_reporting_test_flag_02, false,
          "usage_reporting_test_flag_02 help message");
TURBO_FLAG(double, usage_reporting_test_flag_03, 1.03,
          "usage_reporting_test_flag_03 help message");
TURBO_FLAG(int64_t, usage_reporting_test_flag_04, 1000000000000004L,
          "usage_reporting_test_flag_04 help message");

static const char kTestUsageMessage[] = "Custom usage message";

struct UDT {
  UDT() = default;
  UDT(const UDT&) = default;
  UDT& operator=(const UDT&) = default;
};
static bool TurboParseFlag(turbo::string_piece, UDT*, std::string*) {
  return true;
}
static std::string TurboUnparseFlag(const UDT&) { return "UDT{}"; }

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

static std::string NormalizeFileName(turbo::string_piece fname) {
#ifdef _WIN32
  std::string normalized(fname);
  std::replace(normalized.begin(), normalized.end(), '\\', '/');
  fname = normalized;
#endif

  auto turbo_pos = fname.rfind("turbo/");
  if (turbo_pos != turbo::string_piece::npos) {
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

 private:
  turbo::FlagSaver flag_saver_;
};

// --------------------------------------------------------------------

using UsageReportingDeathTest = UsageReportingTest;

TEST_F(UsageReportingDeathTest, TestSetProgramUsageMessage) {
#if !defined(GTEST_HAS_TURBO) || !GTEST_HAS_TURBO
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

  Flags from turbo/flags/internal/usage_test.cc:
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
      message.); default: "";

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)";

  std::stringstream test_buf_01;
  flags::FlagsHelp(test_buf_01, "usage_test.cc",
                   flags::HelpFormat::kHumanReadable, kTestUsageMessage);
  EXPECT_EQ(test_buf_01.str(), usage_test_flags_out);

  std::stringstream test_buf_02;
  flags::FlagsHelp(test_buf_02, "flags/internal/usage_test.cc",
                   flags::HelpFormat::kHumanReadable, kTestUsageMessage);
  EXPECT_EQ(test_buf_02.str(), usage_test_flags_out);

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
  turbo::string_piece test_out_str(test_out);
  EXPECT_TRUE(
      turbo::StartsWith(test_out_str, "usage_test: Custom usage message"));
  EXPECT_TRUE(turbo::StrContains(
      test_out_str, "Flags from turbo/flags/internal/usage_test.cc:"));
  EXPECT_TRUE(
      turbo::StrContains(test_out_str, "-usage_reporting_test_flag_01 "));
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestNoUsageFlags) {
  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), -1);
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_helpshort) {
  flags::SetFlagsHelpMode(flags::HelpMode::kShort);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), 1);
  EXPECT_EQ(test_buf.str(),
            R"(usage_test: Custom usage message

  Flags from turbo/flags/internal/usage_test.cc:
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
      message.); default: "";

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_help_simple) {
  flags::SetFlagsHelpMode(flags::HelpMode::kImportant);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), 1);
  EXPECT_EQ(test_buf.str(),
            R"(usage_test: Custom usage message

  Flags from turbo/flags/internal/usage_test.cc:
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
      message.); default: "";

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
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), 1);
  EXPECT_EQ(test_buf.str(),
            R"(usage_test: Custom usage message

  Flags from turbo/flags/internal/usage_test.cc:
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
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), 1);
  EXPECT_EQ(test_buf.str(),
            R"(usage_test: Custom usage message

  Flags from turbo/flags/internal/usage_test.cc:
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
      message.); default: "";

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_helppackage) {
  flags::SetFlagsHelpMode(flags::HelpMode::kPackage);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), 1);
  EXPECT_EQ(test_buf.str(),
            R"(usage_test: Custom usage message

  Flags from turbo/flags/internal/usage_test.cc:
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
      message.); default: "";

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_version) {
  flags::SetFlagsHelpMode(flags::HelpMode::kVersion);

  std::stringstream test_buf;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), 0);
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
  EXPECT_EQ(flags::HandleUsageFlags(test_buf, kTestUsageMessage), 0);
  EXPECT_EQ(test_buf.str(), "");
}

// --------------------------------------------------------------------

TEST_F(UsageReportingTest, TestUsageFlag_helpon) {
  flags::SetFlagsHelpMode(flags::HelpMode::kMatch);
  flags::SetFlagsHelpMatchSubstr("/bla-bla.");

  std::stringstream test_buf_01;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf_01, kTestUsageMessage), 1);
  EXPECT_EQ(test_buf_01.str(),
            R"(usage_test: Custom usage message

No flags matched.

Try --helpfull to get a list of all flags or --help=substring shows help for
flags which include specified substring in either in the name, or description or
path.
)");

  flags::SetFlagsHelpMatchSubstr("/usage_test.");

  std::stringstream test_buf_02;
  EXPECT_EQ(flags::HandleUsageFlags(test_buf_02, kTestUsageMessage), 1);
  EXPECT_EQ(test_buf_02.str(),
            R"(usage_test: Custom usage message

  Flags from turbo/flags/internal/usage_test.cc:
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
      message.); default: "";

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
#if !defined(GTEST_HAS_TURBO) || !GTEST_HAS_TURBO
  // GoogleTest calls turbo::SetProgramUsageMessage() already.
  turbo::SetProgramUsageMessage(kTestUsageMessage);
#endif
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
