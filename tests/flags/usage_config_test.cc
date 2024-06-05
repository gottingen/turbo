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

#include <turbo/flags/usage_config.h>

#include <string>

#include <gtest/gtest.h>
#include <turbo/flags/internal/path_util.h>
#include <turbo/flags/internal/program_name.h>
#include <turbo/strings/match.h>
#include <turbo/strings/string_view.h>

namespace {

class FlagsUsageConfigTest : public testing::Test {
 protected:
  void SetUp() override {
    // Install Default config for the use on this unit test.
    // Binary may install a custom config before tests are run.
    turbo::FlagsUsageConfig default_config;
    turbo::set_flags_usage_config(default_config);
  }
};

namespace flags = turbo::flags_internal;

bool TstContainsHelpshortFlags(turbo::string_view f) {
  return turbo::starts_with(flags::Basename(f), "progname.");
}

bool TstContainsHelppackageFlags(turbo::string_view f) {
  return turbo::ends_with(flags::Package(f), "aaa/");
}

bool TstContainsHelpFlags(turbo::string_view f) {
  return turbo::ends_with(flags::Package(f), "zzz/");
}

std::string TstVersionString() { return "program 1.0.0"; }

std::string TstNormalizeFilename(turbo::string_view filename) {
  return std::string(filename.substr(2));
}

void TstReportUsageMessage(turbo::string_view msg) {}

// --------------------------------------------------------------------

TEST_F(FlagsUsageConfigTest, TestGetSetFlagsUsageConfig) {
  EXPECT_TRUE(flags::GetUsageConfig().contains_helpshort_flags);
  EXPECT_TRUE(flags::GetUsageConfig().contains_help_flags);
  EXPECT_TRUE(flags::GetUsageConfig().contains_helppackage_flags);
  EXPECT_TRUE(flags::GetUsageConfig().version_string);
  EXPECT_TRUE(flags::GetUsageConfig().normalize_filename);

  turbo::FlagsUsageConfig empty_config;
  empty_config.contains_helpshort_flags = &TstContainsHelpshortFlags;
  empty_config.contains_help_flags = &TstContainsHelpFlags;
  empty_config.contains_helppackage_flags = &TstContainsHelppackageFlags;
  empty_config.version_string = &TstVersionString;
  empty_config.normalize_filename = &TstNormalizeFilename;
  turbo::set_flags_usage_config(empty_config);

  EXPECT_TRUE(flags::GetUsageConfig().contains_helpshort_flags);
  EXPECT_TRUE(flags::GetUsageConfig().contains_help_flags);
  EXPECT_TRUE(flags::GetUsageConfig().contains_helppackage_flags);
  EXPECT_TRUE(flags::GetUsageConfig().version_string);
  EXPECT_TRUE(flags::GetUsageConfig().normalize_filename);
}

// --------------------------------------------------------------------

TEST_F(FlagsUsageConfigTest, TestContainsHelpshortFlags) {
#if defined(_WIN32)
  flags::SetProgramInvocationName("usage_config_test.exe");
#else
  flags::SetProgramInvocationName("usage_config_test");
#endif

  auto config = flags::GetUsageConfig();
  EXPECT_TRUE(config.contains_helpshort_flags("adir/cd/usage_config_test.cc"));
  EXPECT_TRUE(
      config.contains_helpshort_flags("aaaa/usage_config_test-main.cc"));
  EXPECT_TRUE(config.contains_helpshort_flags("abc/usage_config_test_main.cc"));
  EXPECT_FALSE(config.contains_helpshort_flags("usage_config_main.cc"));

  turbo::FlagsUsageConfig empty_config;
  empty_config.contains_helpshort_flags = &TstContainsHelpshortFlags;
  turbo::set_flags_usage_config(empty_config);

  EXPECT_TRUE(
      flags::GetUsageConfig().contains_helpshort_flags("aaa/progname.cpp"));
  EXPECT_FALSE(
      flags::GetUsageConfig().contains_helpshort_flags("aaa/progmane.cpp"));
}

// --------------------------------------------------------------------

TEST_F(FlagsUsageConfigTest, TestContainsHelpFlags) {
  flags::SetProgramInvocationName("usage_config_test");

  auto config = flags::GetUsageConfig();
  EXPECT_TRUE(config.contains_help_flags("zzz/usage_config_test.cc"));
  EXPECT_TRUE(
      config.contains_help_flags("bdir/a/zzz/usage_config_test-main.cc"));
  EXPECT_TRUE(
      config.contains_help_flags("//aqse/zzz/usage_config_test_main.cc"));
  EXPECT_FALSE(config.contains_help_flags("zzz/aa/usage_config_main.cc"));

  turbo::FlagsUsageConfig empty_config;
  empty_config.contains_help_flags = &TstContainsHelpFlags;
  turbo::set_flags_usage_config(empty_config);

  EXPECT_TRUE(flags::GetUsageConfig().contains_help_flags("zzz/main-body.c"));
  EXPECT_FALSE(
      flags::GetUsageConfig().contains_help_flags("zzz/dir/main-body.c"));
}

// --------------------------------------------------------------------

TEST_F(FlagsUsageConfigTest, TestContainsHelppackageFlags) {
  flags::SetProgramInvocationName("usage_config_test");

  auto config = flags::GetUsageConfig();
  EXPECT_TRUE(config.contains_helppackage_flags("aaa/usage_config_test.cc"));
  EXPECT_TRUE(
      config.contains_helppackage_flags("bbdir/aaa/usage_config_test-main.cc"));
  EXPECT_TRUE(config.contains_helppackage_flags(
      "//aqswde/aaa/usage_config_test_main.cc"));
  EXPECT_FALSE(config.contains_helppackage_flags("aadir/usage_config_main.cc"));

  turbo::FlagsUsageConfig empty_config;
  empty_config.contains_helppackage_flags = &TstContainsHelppackageFlags;
  turbo::set_flags_usage_config(empty_config);

  EXPECT_TRUE(
      flags::GetUsageConfig().contains_helppackage_flags("aaa/main-body.c"));
  EXPECT_FALSE(
      flags::GetUsageConfig().contains_helppackage_flags("aadir/main-body.c"));
}

// --------------------------------------------------------------------

TEST_F(FlagsUsageConfigTest, TestVersionString) {
  flags::SetProgramInvocationName("usage_config_test");

#ifdef NDEBUG
  std::string expected_output = "usage_config_test\n";
#else
  std::string expected_output =
      "usage_config_test\nDebug build (NDEBUG not #defined)\n";
#endif

  EXPECT_EQ(flags::GetUsageConfig().version_string(), expected_output);

  turbo::FlagsUsageConfig empty_config;
  empty_config.version_string = &TstVersionString;
  turbo::set_flags_usage_config(empty_config);

  EXPECT_EQ(flags::GetUsageConfig().version_string(), "program 1.0.0");
}

// --------------------------------------------------------------------

TEST_F(FlagsUsageConfigTest, TestNormalizeFilename) {
  // This tests the default implementation.
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("a/a.cc"), "a/a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("/a/a.cc"), "a/a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("///a/a.cc"), "a/a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("/"), "");

  // This tests that the custom implementation is called.
  turbo::FlagsUsageConfig empty_config;
  empty_config.normalize_filename = &TstNormalizeFilename;
  turbo::set_flags_usage_config(empty_config);

  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("a/a.cc"), "a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("aaa/a.cc"), "a/a.cc");

  // This tests that the default implementation is called.
  empty_config.normalize_filename = nullptr;
  turbo::set_flags_usage_config(empty_config);

  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("a/a.cc"), "a/a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("/a/a.cc"), "a/a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("///a/a.cc"), "a/a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("\\a\\a.cc"), "a\\a.cc");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("//"), "");
  EXPECT_EQ(flags::GetUsageConfig().normalize_filename("\\\\"), "");
}

}  // namespace
