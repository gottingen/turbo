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

#include "turbo/flags/reflection.h"

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "turbo/flags/declare.h"
#include "turbo/flags/flag.h"
#include "turbo/flags/internal/commandlineflag.h"
#include "turbo/flags/marshalling.h"
#include "turbo/memory/memory.h"
#include "turbo/strings/str_cat.h"
#include "turbo/strings/str_split.h"

TURBO_FLAG(int, int_flag, 1, "int_flag help");
TURBO_FLAG(std::string, string_flag, "dflt", "string_flag help");
TURBO_RETIRED_FLAG(bool, bool_retired_flag, false, "bool_retired_flag help");

namespace {

class ReflectionTest : public testing::Test {
 protected:
  void SetUp() override { flag_saver_ = turbo::make_unique<turbo::FlagSaver>(); }
  void TearDown() override { flag_saver_.reset(); }

 private:
  std::unique_ptr<turbo::FlagSaver> flag_saver_;
};

// --------------------------------------------------------------------

TEST_F(ReflectionTest, TestFindCommandLineFlag) {
  auto* handle = turbo::FindCommandLineFlag("some_flag");
  EXPECT_EQ(handle, nullptr);

  handle = turbo::FindCommandLineFlag("int_flag");
  EXPECT_NE(handle, nullptr);

  handle = turbo::FindCommandLineFlag("string_flag");
  EXPECT_NE(handle, nullptr);

  handle = turbo::FindCommandLineFlag("bool_retired_flag");
  EXPECT_NE(handle, nullptr);
}

// --------------------------------------------------------------------

TEST_F(ReflectionTest, TestGetAllFlags) {
  auto all_flags = turbo::GetAllFlags();
  EXPECT_NE(all_flags.find("int_flag"), all_flags.end());
  EXPECT_EQ(all_flags.find("bool_retired_flag"), all_flags.end());
  EXPECT_EQ(all_flags.find("some_undefined_flag"), all_flags.end());

  std::vector<turbo::string_piece> flag_names_first_attempt;
  auto all_flags_1 = turbo::GetAllFlags();
  for (auto f : all_flags_1) {
    flag_names_first_attempt.push_back(f.first);
  }

  std::vector<turbo::string_piece> flag_names_second_attempt;
  auto all_flags_2 = turbo::GetAllFlags();
  for (auto f : all_flags_2) {
    flag_names_second_attempt.push_back(f.first);
  }

  EXPECT_THAT(flag_names_first_attempt,
              ::testing::UnorderedElementsAreArray(flag_names_second_attempt));
}

// --------------------------------------------------------------------

struct CustomUDT {
  CustomUDT() : a(1), b(1) {}
  CustomUDT(int a_, int b_) : a(a_), b(b_) {}

  friend bool operator==(const CustomUDT& f1, const CustomUDT& f2) {
    return f1.a == f2.a && f1.b == f2.b;
  }

  int a;
  int b;
};
bool TurboParseFlag(turbo::string_piece in, CustomUDT* f, std::string*) {
  std::vector<turbo::string_piece> parts =
      turbo::StrSplit(in, ':', turbo::SkipWhitespace());

  if (parts.size() != 2) return false;

  if (!turbo::SimpleAtoi(parts[0], &f->a)) return false;

  if (!turbo::SimpleAtoi(parts[1], &f->b)) return false;

  return true;
}
std::string TurboUnparseFlag(const CustomUDT& f) {
  return turbo::StrCat(f.a, ":", f.b);
}

}  // namespace

// --------------------------------------------------------------------

TURBO_FLAG(bool, test_flag_01, true, "");
TURBO_FLAG(int, test_flag_02, 1234, "");
TURBO_FLAG(int16_t, test_flag_03, -34, "");
TURBO_FLAG(uint16_t, test_flag_04, 189, "");
TURBO_FLAG(int32_t, test_flag_05, 10765, "");
TURBO_FLAG(uint32_t, test_flag_06, 40000, "");
TURBO_FLAG(int64_t, test_flag_07, -1234567, "");
TURBO_FLAG(uint64_t, test_flag_08, 9876543, "");
TURBO_FLAG(double, test_flag_09, -9.876e-50, "");
TURBO_FLAG(float, test_flag_10, 1.234e12f, "");
TURBO_FLAG(std::string, test_flag_11, "", "");
TURBO_FLAG(turbo::Duration, test_flag_12, turbo::Minutes(10), "");
static int counter = 0;
TURBO_FLAG(int, test_flag_13, 200, "").OnUpdate([]() { counter++; });
TURBO_FLAG(CustomUDT, test_flag_14, {}, "");

namespace {

TEST_F(ReflectionTest, TestFlagSaverInScope) {
  {
    turbo::FlagSaver s;
    counter = 0;
    turbo::SetFlag(&FLAGS_test_flag_01, false);
    turbo::SetFlag(&FLAGS_test_flag_02, -1021);
    turbo::SetFlag(&FLAGS_test_flag_03, 6009);
    turbo::SetFlag(&FLAGS_test_flag_04, 44);
    turbo::SetFlag(&FLAGS_test_flag_05, +800);
    turbo::SetFlag(&FLAGS_test_flag_06, -40978756);
    turbo::SetFlag(&FLAGS_test_flag_07, 23405);
    turbo::SetFlag(&FLAGS_test_flag_08, 975310);
    turbo::SetFlag(&FLAGS_test_flag_09, 1.00001);
    turbo::SetFlag(&FLAGS_test_flag_10, -3.54f);
    turbo::SetFlag(&FLAGS_test_flag_11, "asdf");
    turbo::SetFlag(&FLAGS_test_flag_12, turbo::Hours(20));
    turbo::SetFlag(&FLAGS_test_flag_13, 4);
    turbo::SetFlag(&FLAGS_test_flag_14, CustomUDT{-1, -2});
  }

  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_01), true);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_02), 1234);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_03), -34);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_04), 189);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_05), 10765);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_06), 40000);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_07), -1234567);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 9876543);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_09), -9.876e-50, 1e-55);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_10), 1.234e12f, 1e5f);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_11), "");
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_12), turbo::Minutes(10));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_13), 200);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_14), CustomUDT{});
  EXPECT_EQ(counter, 2);
}

// --------------------------------------------------------------------

TEST_F(ReflectionTest, TestFlagSaverVsUpdateViaReflection) {
  {
    turbo::FlagSaver s;
    counter = 0;
    std::string error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_01")->ParseFrom("false", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_02")->ParseFrom("-4536", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_03")->ParseFrom("111", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_04")->ParseFrom("909", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_05")->ParseFrom("-2004", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_06")->ParseFrom("1000023", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_07")->ParseFrom("69305", &error))
        << error;
    EXPECT_TRUE(turbo::FindCommandLineFlag("test_flag_08")
                    ->ParseFrom("1000000001", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_09")->ParseFrom("2.09021", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_10")->ParseFrom("-33.1", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_11")->ParseFrom("ADD_FOO", &error))
        << error;
    EXPECT_TRUE(turbo::FindCommandLineFlag("test_flag_12")
                    ->ParseFrom("3h11m16s", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_13")->ParseFrom("0", &error))
        << error;
    EXPECT_TRUE(
        turbo::FindCommandLineFlag("test_flag_14")->ParseFrom("10:1", &error))
        << error;
  }

  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_01), true);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_02), 1234);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_03), -34);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_04), 189);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_05), 10765);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_06), 40000);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_07), -1234567);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 9876543);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_09), -9.876e-50, 1e-55);
  EXPECT_NEAR(turbo::GetFlag(FLAGS_test_flag_10), 1.234e12f, 1e5f);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_11), "");
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_12), turbo::Minutes(10));
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_13), 200);
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_14), CustomUDT{});
  EXPECT_EQ(counter, 2);
}

// --------------------------------------------------------------------

TEST_F(ReflectionTest, TestMultipleFlagSaversInEnclosedScopes) {
  {
    turbo::FlagSaver s;
    turbo::SetFlag(&FLAGS_test_flag_08, 10);
    EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 10);
    {
      turbo::FlagSaver s;
      turbo::SetFlag(&FLAGS_test_flag_08, 20);
      EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 20);
      {
        turbo::FlagSaver s;
        turbo::SetFlag(&FLAGS_test_flag_08, -200);
        EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), -200);
      }
      EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 20);
    }
    EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 10);
  }
  EXPECT_EQ(turbo::GetFlag(FLAGS_test_flag_08), 9876543);
}

}  // namespace
